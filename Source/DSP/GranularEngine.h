// ==========================================
// File: GranularEngine.h
// 24ボイス・グラニュラーエンジン
//
// Tessellator からの主な改良点:
//  - グレイン読み出しを 4点 Hermite 補間に変更（最近傍→高音質化）
//  - 密度を「オーバーラップ量」として扱う決定論的スケジューラ
//    （確率スポーンよりテクスチャが安定し、低密度でも粒が途切れない）
//  - ボイススチーリング（最古ノートを奪う）
//  - ベロシティ対応
//  - グレインピッチ量子化: ノート+オクターブ+セミ+ファイン+ジッターの
//    「変調後の最終ピッチ」を Key/Scale にスナップ (ScaleQuantizer)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include "ScaleQuantizer.h"
#include "GrainVisualizerData.h"

// IEEE-754 ビットパターン判定で /fp:fast 環境下でも確実に NaN / Inf を検出
inline bool isFiniteFloat(float x) noexcept
{
    uint32_t bits = 0;
    std::memcpy(&bits, &x, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}

// ==========================================
// CPU Optimization: Fast Xorshift PRNG
// ==========================================
class FastRandom
{
    uint32_t state = 123456789;
public:
    inline void setSeed(uint32_t seed) noexcept { state = seed ? seed : 123456789; }
    inline uint32_t nextInt() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    inline float nextFloat() noexcept
    {
        return (float)(nextInt() & 0x00FFFFFF) / (float)0x01000000;
    }
    inline int nextInt(int maxVal) noexcept
    {
        if (maxVal <= 0) return 0;
        return (int)(nextInt() % (uint32_t)maxVal);
    }
};

// ==========================================
// パラメータスナップショット（processBlock毎にAPVTSから詰める）
// ==========================================
struct GranularParams
{
    float attack = 0.2f;
    float decay = 0.1f;
    float sustain = 1.0f;
    float release = 0.5f;

    float grainSize = 0.15f;      // 秒
    float density = 0.5f;         // 0..1 (オーバーラップ量)
    float position = 0.0f;        // 0..1
    float positionSpray = 0.0f;   // 0..1
    float pitchJitter = 0.0f;     // セント 0..2400
    float panSpread = 1.0f;       // 0..1
    float speed = 1.0f;           // 0..2 プレイヘッド速度（倍率、方向はplayMode）

    int   playMode = 0;           // 0=Forward 1=Backward 2=Ping-Pong 3=Random
    bool  speedSync = false;      // DAWテンポ追随（領域を指定音価でスキャン）
    int   speedSyncDiv = 2;       // 0=1Beat 1=2Beats 2=1Bar 3=2Bars 4=4Bars 5=8Bars
    double bpm = 120.0;

    int   octave = 0;
    int   pitchSt = 0;
    float fineTune = 0.0f;        // セント
    float pitchMod = 0.0f;        // MODマトリクスからのピッチ変調(半音) ※量子化前に加算
    float outGainDb = -12.0f;
    bool  reverseGrain = false;
    int   windowShape = 0;        // 0..3

    int   key = 0;                // 0=Auto(解析結果), 1..12 = C..B
    int   scale = 0;              // ScaleQuantizer のインデックス (0..81)
    bool  scaleOn = false;        // スケール量子化の有効/無効

    bool  pitchSync = false;      // グレイン発火をノート周波数に同期（音程感強調）
    float spraySpeed = 1.0f;      // 0.01..1 スプレー値の移動速度
    float tone = 0.0f;            // 0..1 ノート同調コムレゾネーター量（音程感強調）

    float masterHPF = 20.0f;
    float masterLPF = 20000.0f;
    int   filterSlope = 0;        // 0=12dB/oct(1段) 1=24dB/oct(2段)
    float ceiling = -0.1f;        // マスターリミッターの天井 (dB)
    float limRelease = 50.0f;     // リミッターのリリース (ms)
    int   poly = 24;              // 同時発音数 (1..24)

    int   detectedRoot = 0;       // 解析で得たルート (key==Auto時に使用)
    int   rootNote = 60;          // Root Note Tracking: サンプルの元ピッチ(MIDI)
                                  // このノートを弾くと原音の高さで再生される

    // SR補償: バッファ作成時SR / ホストSR。
    // DAWのSR変更時(44.1k-192k)でもピッチと速度を不変に保つ
    float srComp = 1.0f;
};

// ==========================================
// PadEnvelope — PAD対応カスタムADSR
//   Attack : べき乗カーブ(ゆっくり立ち上がる) 最大10秒 / 最短0.5msで歯切れよく
//   Decay/Release : 指数カーブ(アナログ的な自然な減衰、初動が速い)
//   リトリガーは現在値から継続（ボイススチール時のクリック防止）
// ==========================================
class PadEnvelope
{
public:
    void setSampleRate(double sr) noexcept
    {
        sampleRate = juce::jmax(1.0, sr);
        setParameters(atkTime, decTime, sus, relTime);
    }

    void setParameters(float a, float d, float s, float r) noexcept
    {
        atkTime = juce::jmax(0.0005f, a);
        decTime = juce::jmax(0.001f, d);
        sus     = juce::jlimit(0.0f, 1.0f, s);
        relTime = juce::jmax(0.001f, r);

        attackInc   = (float)(1.0 / ((double)atkTime * sampleRate));
        decayCoef   = 1.0f - std::exp((float)(-4.6 / ((double)decTime * sampleRate)));
        releaseCoef = 1.0f - std::exp((float)(-6.9 / ((double)relTime * sampleRate)));
    }

    void noteOn() noexcept
    {
        // 現在値からアタックを再開（位相を逆算、極小値は0でクランプ）
        const float safeVal = juce::jlimit(0.0f, 1.0f, value);
        attackPhase = (safeVal > 1.0e-5f) ? std::pow(safeVal, 1.0f / kAttackCurve) : 0.0f;
        attackInc   = (float)(1.0 / ((double)atkTime * sampleRate));
        decayCoef   = 1.0f - std::exp((float)(-4.6 / ((double)decTime * sampleRate)));
        releaseCoef = 1.0f - std::exp((float)(-6.9 / ((double)relTime * sampleRate)));
        stage = Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle) stage = Stage::Release;
    }

    void reset() noexcept { stage = Stage::Idle; value = 0.0f; attackPhase = 0.0f; }
    bool isActive() const noexcept { return stage != Stage::Idle; }

    inline float getNextSample() noexcept
    {
        switch (stage)
        {
        case Stage::Attack:
            attackPhase += attackInc;
            if (attackPhase >= 1.0f) { attackPhase = 1.0f; value = 1.0f; stage = Stage::Decay; }
            else value = std::pow(attackPhase, kAttackCurve); // 凹カーブ: ゆっくり→加速
            break;
        case Stage::Decay:
            value += decayCoef * (sus - value);
            if (std::abs(value - sus) < 1.0e-4f) { value = sus; stage = Stage::Sustain; }
            break;
        case Stage::Sustain:
            if (value > sus)
            {
                value += decayCoef * (sus - value);
                if (value - sus < 1.0e-4f) value = sus;
            }
            else
            {
                value = sus;
            }
            break;
        case Stage::Release:
            value += releaseCoef * (0.0f - value);
            if (value < 2.0e-4f) { value = 0.0f; stage = Stage::Idle; }
            break;
        case Stage::Idle:
        default:
            value = 0.0f;
            break;
        }
        return value;
    }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    static constexpr float kAttackCurve = 2.5f;

    Stage stage = Stage::Idle;
    double sampleRate = 44100.0;
    float value = 0.0f;
    float attackPhase = 0.0f, attackInc = 0.001f;
    float atkTime = 0.02f, decTime = 0.1f, sus = 1.0f, relTime = 0.5f;
    float decayCoef = 0.001f, releaseCoef = 0.001f;
};

// ==========================================
// TptSvf — TPT(トポロジー保存)ステートバリアブルフィルタ (Zavalishin)
//   カットオフをサンプル単位でスムージングするため、変調しても
//   ザラつき/クリックが出ない。1段=12dB/oct。2段直列で24dB/oct。
// ==========================================
struct TptSvf
{
    void prepare(double sr) noexcept
    {
        sampleRate = sr > 1.0 ? sr : 44100.0;
        s1 = s2 = 0.0f;
        smoothCoef = 1.0f - std::exp(-1.0f / (float)(0.012 * sampleRate)); // ~12ms対数グライド
        logFreq = logFreqTarget = std::log(1000.0f);
        g = gTarget = std::tan(juce::MathConstants<float>::pi * 1000.0f / (float)sampleRate);
        coeffInit = false;
    }
    void reset() noexcept
    {
        s1 = s2 = 0.0f;
        coeffInit = false;
    }

    // 目標カットオフ(Hz)/Qを設定（対数周波数ドメインでper-sampleスムージング）
    void setTarget(float freqHz, float q) noexcept
    {
        const float nyq = (float)(sampleRate * 0.5);
        freqHz = juce::jlimit(10.0f, nyq * 0.99f, freqHz);
        logFreqTarget = std::log(freqHz);
        gTarget = std::tan(juce::MathConstants<float>::pi * freqHz / (float)sampleRate);
        R2 = 1.0f / juce::jmax(0.05f, q);

        if (!coeffInit)
        {
            logFreq = logFreqTarget;
            g = gTarget;
            coeffInit = true;
        }
    }

    inline void tickCoeff() noexcept
    {
        const float diff = logFreqTarget - logFreq;
        if (std::abs(diff) > 1.0e-5f)
        {
            logFreq += smoothCoef * diff;
            g = std::tan(juce::MathConstants<float>::pi * std::exp(logFreq) / (float)sampleRate);
        }
        else if (g != gTarget)
        {
            logFreq = logFreqTarget;
            g = gTarget;
        }
    }

    inline float processLP(float x) noexcept
    {
        tickCoeff();
        const float hp = (x - (R2 + g) * s1 - s2) / (1.0f + R2 * g + g * g);
        const float bp = g * hp + s1; s1 = g * hp + bp;
        const float lp = g * bp + s2; s2 = g * bp + lp;
        return lp;
    }
    inline float processHP(float x) noexcept
    {
        tickCoeff();
        const float hp = (x - (R2 + g) * s1 - s2) / (1.0f + R2 * g + g * g);
        const float bp = g * hp + s1; s1 = g * hp + bp;
        const float lp = g * bp + s2; s2 = g * bp + lp;
        return hp;
    }

    double sampleRate = 44100.0;
    float g = 0.05f, gTarget = 0.05f, R2 = 1.41421356f, s1 = 0.0f, s2 = 0.0f;
    float logFreq = 6.907755f, logFreqTarget = 6.907755f, smoothCoef = 0.002f;
    bool coeffInit = false;
};

// ==========================================
// BrickLimiter — マスター用ブリックウォールリミッター
//   アタック即時（天井を絶対に超えない）/ リリース可変
// ==========================================
class BrickLimiter
{
public:
    void prepare(double sr) noexcept
    {
        sampleRate = sr;
        setReleaseMs(50.0f);
        gain = 1.0f;
    }
    void reset() noexcept { gain = 1.0f; }
    void setReleaseMs(float ms) noexcept
    {
        const double t = juce::jmax(0.5f, ms) * 0.001;
        releaseCoef = 1.0f - std::exp((float)(-1.0 / (t * sampleRate)));
    }
    inline void processMono(float& s, float ceilingLin) noexcept
    {
        const float peak = juce::jmax(std::abs(s), 1.0e-9f);
        const float target = peak > ceilingLin ? ceilingLin / peak : 1.0f;
        if (target < gain) gain = target;                    // 即時アタック
        else gain += releaseCoef * (target - gain);          // スムーズリリース
        gain = juce::jmax(1.0e-6f, gain);                    // デノーマル防止
        s *= gain;
    }
    inline void process(float& l, float& r, float ceilingLin) noexcept
    {
        if (&l == &r) { processMono(l, ceilingLin); return; }
        const float peak = juce::jmax(std::abs(l), std::abs(r), 1.0e-9f);
        const float target = peak > ceilingLin ? ceilingLin / peak : 1.0f;
        if (target < gain) gain = target;                    // 即時アタック
        else gain += releaseCoef * (target - gain);          // スムーズリリース
        gain = juce::jmax(1.0e-6f, gain);                    // デノーマル防止
        l *= gain;
        r *= gain;
    }
private:
    double sampleRate = 44100.0;
    float gain = 1.0f;
    float releaseCoef = 0.0005f;
};

// ==========================================
// グレイン
// ==========================================
struct Grain
{
    bool   isActive = false;
    double position = 0.0;        // ソース内の読み出し位置（サンプル）
    double step = 1.0;            // 1出力サンプルあたりの前進量（符号込み）
    int    age = 0;
    int    life = 0;
    float  panL = 0.7f;
    float  panR = 0.7f;
    float  amp = 1.0f;
    double trimStart = 0.0;       // スポーン時のStart位置
    double trimEnd = 0.0;         // スポーン時のEnd位置
};

// ==========================================
// ボイス
// ==========================================
class GranularVoice
{
public:
    GranularVoice() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void startNote(int midiNote, float velocity, uint64_t noteStamp, const GranularParams& p);
    void stopNote(bool allowTailOff);
    bool isActive() const noexcept { return playing; }
    void clearCurrentNote();
    void reset();

    int getCurrentlyPlayingNote() const noexcept { return currentMidiNote; }
    uint64_t getNoteStamp() const noexcept { return noteStamp; }
    bool isReleasing() const noexcept { return playing && !heldDown; }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         const juce::AudioBuffer<float>* sharedSourceBuffer,
                         int trimStart, int trimEnd,
                         const std::array<std::array<float, 4096>, 4>& windowTables,
                         const GranularParams& p,
                         GrainVisualizerData* visualizer,
                         int& spawnBudget);

    int getActiveGrainCount() const noexcept
    {
        int n = 0;
        for (const auto& g : grains) if (g.isActive) ++n;
        return n;
    }

    // 直近ブロック末の実プレイヘッド位置 (領域内正規化 0..1、無効=-1)
    float getPlayheadNorm() const noexcept { return lastRelNorm; }

private:
    void spawnGrain(int trimStart, int trimEnd,
                    double currentPlayhead, const GranularParams& p,
                    float overlap, int baseLife,
                    GrainVisualizerData* visualizer, int& spawnBudget);

    double currentSampleRate = 44100.0;
    int    currentMidiNote = -1;
    bool   playing = false;
    bool   heldDown = false;
    uint64_t noteStamp = 0;

    float  velocityGain = 1.0f;
    double playheadOffset = 0.0;    // 領域正規化位相 (0..1 / PingPong 0..2)
    double randomTargetRel = 0.0;   // Randomモード: 現在のジャンプ先（領域内正規化 0..1）
    double randomJumpCountdown = 0.0; // Randomモード: 次のジャンプまでの残りサンプル (0以下=即抽選)
    float  panSide = 1.0f;          // Pan交互配置用（グレイン毎にL/R反転）
    float  lastRelNorm = -1.0f;     // GUI表示用の実プレイヘッド位置
    double samplesToNextSpawn = 0.0;
    double smoothedPos01 = 0.0;     // Position のサンプル単位グライド（急ジャンプ→バースト防止）
    double currentSprayTarget = 0.0;
    double currentSprayValue = 0.0;

    PadEnvelope env;

    static constexpr int MAX_GRAINS = 100;   // Quanta 2 同等の1ボイス上限
    std::array<Grain, MAX_GRAINS> grains;

    FastRandom random;
    juce::AudioBuffer<float> voiceInternalBuffer;

    // --- TONE: ノート同調コムレゾネーター (Thiran 1次AP補間 + SR補償LPF + DCブロッカー) ---
    std::vector<float> combBufL, combBufR;
    int   combWriteIdx = 0;
    float combLpL = 0.0f, combLpR = 0.0f;
    float combApXPrevL = 0.0f, combApYPrevL = 0.0f;
    float combApXPrevR = 0.0f, combApYPrevR = 0.0f;
    float combDcXPrevL = 0.0f, combDcYPrevL = 0.0f;
    float combDcXPrevR = 0.0f, combDcYPrevR = 0.0f;
    float currentCombDelay = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularVoice)
};

// ==========================================
// シンセ本体
// ==========================================
class GranularSynth
{
public:
    GranularSynth();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float>& outputBuffer,
                      juce::MidiBuffer& midiMessages,
                      const juce::AudioBuffer<float>* sharedSourceBuffer,
                      int trimStart, int trimEnd,
                      const GranularParams& p);

    void allNotesOff()
    {
        for (auto& v : voices) v.stopNote(false);
    }

    void reset() noexcept
    {
        for (auto& v : voices) v.reset();
        for (int ch = 0; ch < 2; ++ch)
            for (int st = 0; st < 2; ++st)
            {
                svfHP[ch][st].reset();
                svfLP[ch][st].reset();
            }
        prevOutGain = 0.0f;
        hasPrevParams = false;
        playheadDisplay.store(-1.0f);
    }

    GrainVisualizerData visualizer;
    std::atomic<float> playheadDisplay { -1.0f }; // 最新ボイスの実プレイヘッド (GUI用)

    static constexpr int NUM_VOICES = 24;
    static constexpr int MAX_TOTAL_GRAINS = 1200; // 全ボイス合計のCPUバジェット

private:
    void handleMidiMessage(const juce::MidiMessage& msg, const GranularParams& p);
    void renderVoicesRange(juce::AudioBuffer<float>& subBuf,
                           const juce::AudioBuffer<float>* src,
                           int trimStart, int trimEnd,
                           const GranularParams& p);
    void generateWindowTables();

    std::array<GranularVoice, NUM_VOICES> voices;
    std::array<std::array<float, 4096>, 4> windowTables;

    // マスターフィルタ: [ch][stage] (stage1は24dB/oct Butterworth Q2=1.30656296)
    TptSvf svfHP[2][2];
    TptSvf svfLP[2][2];

    GranularParams prevParams;
    bool  hasPrevParams = false;
    float prevOutGain = 0.0f;   // 出力ゲインのランプ用

    double currentSampleRate = 44100.0;
    uint64_t noteCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularSynth)
};
