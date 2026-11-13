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
#include "ModalResonator.h"

class ModMatrix;

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

    // --- Phase 4: Commercial-Surpassing Engine & Sculpting ---
    int   engineMode = 0;           // 0=Granular, 1=WSOLA, 2=TD-PSOLA
    float formantShift = 0.0f;      // -12..+12 semitones (PSOLA spectral envelope shift)

    float timbreScrub = 0.5f;       // 0..1 Concatenative Atlas target brightness
    float timbreAmt = 0.0f;         // 0..1 Attract position to Atlas timbre match
    float atlasPitchLock = 0.0f;    // 0..1 Per-grain F0 flattening/tracking from Atlas

    float swarmMix = 0.0f;          // 0..1 Diatonic harmonic swarm blend

    float grainSkew = 0.0f;         // -1..+1 Window skew (percussive vs reverse swell)
    float grainChirp = 0.0f;        // -12..+12 semitones pitch glide per grain
    int   grainFilterMode = 0;      // 0=Off, 1=LP, 2=BP, 3=HP, 4=Notch
    float grainFilterFreq = 20000.0f; // Hz
    float grainFilterRes = 0.7f;    // Q 0.1..10
    float grainFilterJitter = 0.0f; // 0..1 per-grain random cutoff spread

    float modalMix = 0.0f;          // 0..1 Modal Resonator Bank mix
    float modalDecay = 0.8f;        // 0.05..5.0 sec T60
    float modalDamp = 0.4f;         // 0..1 High-mode damping
    float modalInharm = 0.0f;       // 0..1 Stiffness dispersion
    int   modalRatioMode = 0;       // 0=Scale, 1=Harmonic, 2=Marimba, 3=Bell, 4=Plate
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
// BrickLimiter — ルックアヘッド型ブリックウォールリミッター (~1.5ms先読み)
//   スライディング最小値キュー + 移動平均FIRによる無歪み先読みアタック
// ==========================================
class BrickLimiter
{
public:
    BrickLimiter() { prepare(44100.0); }

    void prepare(double sr)
    {
        sampleRate = sr > 1000.0 ? sr : 44100.0;
        lookAheadSamples = juce::jlimit(4, 1024, juce::roundToInt(sampleRate * 0.0015));
        windowLen = lookAheadSamples + 1;
        delayL.assign((size_t)windowLen, 0.0f);
        delayR.assign((size_t)windowLen, 0.0f);
        maBuf.assign((size_t)windowLen, 1.0f);
        dq.resize((size_t)windowLen);
        setReleaseMs(50.0f);
        reset();
    }

    int getLatencySamples() const noexcept { return lookAheadSamples; }

    void reset() noexcept
    {
        std::fill(delayL.begin(), delayL.end(), 0.0f);
        std::fill(delayR.begin(), delayR.end(), 0.0f);
        std::fill(maBuf.begin(), maBuf.end(), 1.0f);
        writeSlot = 0;
        maIdx = 0;
        dqHead = 0;
        dqCount = 0;
        sampleIndex = 0;
        relEnv = 1.0f;
        maSum = (double)windowLen;
    }

    void setReleaseMs(float ms) noexcept
    {
        const double t = juce::jmax(0.5f, ms) * 0.001;
        releaseCoef = 1.0f - std::exp((float)(-1.0 / (t * sampleRate)));
    }

    inline void processMono(float& s, float ceilingLin) noexcept
    {
        float dummyR = s;
        processStereoInternal(s, dummyR, ceilingLin, true);
    }

    inline void process(float& l, float& r, float ceilingLin) noexcept
    {
        if (&l == &r) { processMono(l, ceilingLin); return; }
        processStereoInternal(l, r, ceilingLin, false);
    }

private:
    struct DqEntry { uint64_t idx = 0; float val = 1.0f; };

    inline void processStereoInternal(float& l, float& r, float ceilingLin, bool mono) noexcept
    {
        if (windowLen <= 1)
        {
            const float peak = mono ? std::abs(l) : juce::jmax(std::abs(l), std::abs(r));
            const float g = peak > ceilingLin ? ceilingLin / juce::jmax(peak, 1.0e-9f) : 1.0f;
            l *= g;
            if (!mono) r *= g;
            return;
        }

        const float inL = l;
        const float inR = mono ? l : r;
        const float peak = juce::jmax(mono ? std::abs(inL) : juce::jmax(std::abs(inL), std::abs(inR)), 1.0e-9f);
        const float reqGain = peak > ceilingLin ? (ceilingLin / peak) : 1.0f;

        // 先読みウィンドウ幅Wのスライディング最小値（O(1) 均等償却）
        while (dqCount > 0 && dq[(size_t)dqHead].idx + (uint64_t)windowLen <= sampleIndex)
        {
            dqHead = (dqHead + 1) % windowLen;
            --dqCount;
        }
        while (dqCount > 0)
        {
            const int tailIdx = (dqHead + dqCount - 1) % windowLen;
            if (dq[(size_t)tailIdx].val >= reqGain) --dqCount;
            else break;
        }
        dq[(size_t)((dqHead + dqCount) % windowLen)] = { sampleIndex, reqGain };
        ++dqCount;

        const float winMin = dq[(size_t)dqHead].val;
        relEnv = (winMin < relEnv) ? winMin : juce::jmin(winMin, relEnv + releaseCoef * (winMin - relEnv));

        // 移動平均FIRによる先読みランプ平滑化 (W点移動平均でピーク到達時に必ず<=reqGainに収束)
        const float oldMa = maBuf[(size_t)maIdx];
        maBuf[(size_t)maIdx] = relEnv;
        if (maIdx == 0)
        {
            double s = 0.0;
            for (int k = 0; k < windowLen; ++k) s += (double)maBuf[(size_t)k];
            maSum = s;
        }
        else
        {
            maSum += (double)relEnv - (double)oldMa;
        }
        if (++maIdx >= windowLen) maIdx = 0;

        float g = juce::jlimit(1.0e-6f, 1.0f, (float)(maSum / (double)windowLen));

        // 円形遅延バッファ読み書き (ちょうど lookAheadSamples = windowLen - 1 サンプル遅延)
        delayL[(size_t)writeSlot] = inL;
        if (!mono) delayR[(size_t)writeSlot] = inR;

        const int readSlot = (writeSlot + 1) % windowLen;
        const float outL = delayL[(size_t)readSlot];
        const float outR = mono ? outL : delayR[(size_t)readSlot];

        // 浮動小数点丸め誤差への最終ハードガード
        const float outPeak = mono ? std::abs(outL) : juce::jmax(std::abs(outL), std::abs(outR));
        if (outPeak * g > ceilingLin && outPeak > 1.0e-9f)
            g = ceilingLin / outPeak;

        l = outL * g;
        if (!mono) r = outR * g;

        writeSlot = readSlot;
        ++sampleIndex;
    }

    double sampleRate = 44100.0;
    int lookAheadSamples = 66;
    int windowLen = 67;
    std::vector<float> delayL, delayR, maBuf;
    std::vector<DqEntry> dq;
    int writeSlot = 0;
    int maIdx = 0;
    int dqHead = 0, dqCount = 0;
    uint64_t sampleIndex = 0;
    float relEnv = 1.0f;
    double maSum = 67.0;
    float releaseCoef = 0.0005f;
};

// ==========================================
// オクターブ Mip-Map サンプルピラミッド
//   Level 0 (=1x) は親バッファを直接参照し、2x/4x/8x/16x の帯域制限縮小版を事前生成。
//   +1〜+4oct 以上の高速再生でも折り返し雑音(エイリアシング)を抑制する。
// ==========================================
struct SampleMipLevels
{
    static constexpr int kExtraLevels = 4; // 0:2x (+1oct), 1:4x (+2oct), 2:8x (+3oct), 3:16x (+4oct)
    std::array<juce::AudioBuffer<float>, kExtraLevels> downsampled;
    int numExtraLevels = 0;

    void clear()
    {
        for (auto& b : downsampled) b.setSize(0, 0);
        numExtraLevels = 0;
    }

    void buildFrom(const juce::AudioBuffer<float>& base);
};

// ==========================================
// Concatenative Grain Atlas (Breakthrough 1)
//  Ingestスレッドで音色・トランジェント・局所ピッチ・ゼロクロス位置を解析し、
//  Audioスレッドでは O(log N) 二分探索でゼロアロケーション検索する。
// ==========================================
struct AtlasFrame
{
    int   samplePos = 0;      // 解析フレーム中心 (サンプル)
    int   zeroCrossPos = 0;   // 近傍の上向きゼロクロス点
    float timbre = 0.5f;      // 正規化スペクトル明度 [0..1]
    float rms = 0.0f;         // 局所RMS [0..1]
    float onsetScore = 0.0f;  // 過渡スコア [0..1]
    float localMidi = 60.0f;  // 局所F0 (連続MIDI値)
    bool  hasPitch = false;   // 有効な周期性が検出されたか
};

struct GrainAtlas
{
    std::vector<AtlasFrame> frames;
    std::vector<int>        timbreOrder; // `frames` の timbre 昇順インデックス
    int  hopSamples = 512;
    int  trimStartRef = 0;
    bool valid = false;

    void clear()
    {
        frames.clear();
        timbreOrder.clear();
        hopSamples = 512;
        trimStartRef = 0;
        valid = false;
    }

    void buildFrom(const juce::AudioBuffer<float>& buf, double sr, int trimStart, int trimEnd);

    /// O(log N) binary search + bounded local window matching `[trimStart, trimEnd)`.
    const AtlasFrame* findBestFrame(float targetTimbre, int trimStart, int trimEnd, uint32_t seed) const noexcept;

    /// O(1) positional lookup in monotonically ordered `frames`.
    const AtlasFrame* findNearestPosFrame(double pos, int trimStart, int trimEnd) const noexcept;
};

// ==========================================
// Diatonic Swarm & Collision-Avoidance Scheduler (Breakthrough 3)
// ==========================================
class GrainSwarmScheduler
{
public:
    static constexpr int kRingCap = 128;

    void reset() noexcept
    {
        for (auto& e : ring) e = {};
        writeSeq = 0;
    }

    /// Nudges `(normPos)` away from recently spawned identical `(pos, pitch)` grains and records the spawn.
    float resolveCollisionAndRecord(float normPos, float midiPitch, float minDist, bool allowRepel) noexcept
    {
        float resolvedPos = juce::jlimit(0.0f, 1.0f, normPos);
        if (allowRepel && writeSeq > 0)
        {
            const uint32_t window = juce::jmin<uint32_t>(16u, writeSeq);
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                bool collided = false;
                for (uint32_t k = 1; k <= window; ++k)
                {
                    const auto& prev = ring[(size_t)((writeSeq - k) & (uint32_t)(kRingCap - 1))];
                    if (std::abs(resolvedPos - prev.normPos) < minDist
                        && std::abs(midiPitch - prev.midiPitch) < 0.35f)
                    {
                        collided = true;
                        break;
                    }
                }
                if (!collided) break;
                resolvedPos += ((attempt & 1) == 0 ? 1.35f : -2.1f) * minDist;
                resolvedPos -= std::floor(resolvedPos); // wrap cleanly into [0, 1)
            }
        }

        ring[(size_t)(writeSeq & (uint32_t)(kRingCap - 1))] = { resolvedPos, midiPitch, writeSeq };
        ++writeSeq;
        return resolvedPos;
    }

    /// Returns a strictly scale-locked diatonic interval shift (semitones) for swarm grains.
    static float computeSwarmInterval(float swarmMix, int rootNote, int scaleIdx,
                                      float baseSemis, FastRandom& rng, bool& isHarmonic) noexcept
    {
        isHarmonic = false;
        if (swarmMix <= 0.001f) return 0.0f;

        if (rng.nextFloat() < 0.72f * swarmMix)
        {
            static constexpr float kIntervals[6] = { 7.0f, 12.0f, 4.0f, -12.0f, 19.0f, -5.0f };
            const float rawOff = kIntervals[rng.nextInt(6)];
            const float quantized = ScaleQuantizer::quantize(baseSemis + rawOff, rootNote, scaleIdx);
            isHarmonic = true;
            return quantized - baseSemis;
        }
        return 0.0f;
    }

private:
    struct Entry { float normPos = 0.0f; float midiPitch = 60.0f; uint32_t seq = 0; };
    std::array<Entry, kRingCap> ring {};
    uint32_t writeSeq = 0;
};

// ==========================================
// グレイン
// ==========================================
struct Grain
{
    bool   isActive = false;
    double position = 0.0;        // ソース内の読み出し位置（サンプル）
    double step = 1.0;            // 1出力サンプルあたりの前進量（符号込み）
    double stepStart = 1.0;       // Chirp 開始速度
    double stepEnd = 1.0;         // Chirp 終了速度
    int    age = 0;
    int    life = 0;
    float  panL = 0.7f;
    float  panR = 0.7f;
    float  amp = 1.0f;
    double trimStart = 0.0;       // スポーン時のStart位置
    double trimEnd = 0.0;         // スポーン時のEnd位置
    float  mipLevel = 0.0f;       // 再生速度比に応じたMip-Mapレベル (0=1x .. 4=16x)
    float  skew = 0.0f;           // -1..+1 窓スキュー

    // Per-grain TPT SVF (coef precomputed in spawnGrain)
    int   svfMode = 0;
    float svfK = 1.0f, svfA1 = 0.0f, svfA2 = 0.0f, svfA3 = 0.0f;
    float ic1L = 0.0f, ic2L = 0.0f, ic1R = 0.0f, ic2R = 0.0f;
};

// ==========================================
// ボイス
// ==========================================
class GranularVoice
{
public:
    GranularVoice() = default;

    void setVoiceIndex(int idx) noexcept { voiceIndex = idx; }
    int  getVoiceIndex() const noexcept { return voiceIndex; }

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
                         const SampleMipLevels* mipLevels,
                         int trimStart, int trimEnd,
                         const std::array<std::array<float, 4096>, 4>& windowTables,
                         const GranularParams& p,
                         GrainVisualizerData* visualizer,
                         int& spawnBudget,
                         const GrainAtlas* atlas = nullptr,
                         GrainSwarmScheduler* swarm = nullptr);

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
                    float overlap, int baseLife, double expectedInterval,
                    const juce::AudioBuffer<float>* sharedSourceBuffer,
                    GrainVisualizerData* visualizer, int& spawnBudget,
                    const GrainAtlas* atlas, GrainSwarmScheduler* swarm);

    double currentSampleRate = 44100.0;
    int    voiceIndex = 0;
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
    double wsolaExpectedNextPos = -1.0; // WSOLA 位相整列用リファレンス位置

    PadEnvelope env;

    static constexpr int MAX_GRAINS = 100;   // Quanta 2 同等の1ボイス上限
    std::vector<Grain> grains { (size_t) MAX_GRAINS };

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
                      const GranularParams& p,
                      const SampleMipLevels* mipLevels = nullptr,
                      const GrainAtlas* atlas = nullptr,
                      ModMatrix* modMatrix = nullptr);

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
        swarmScheduler.reset();
        modalResonator.reset();
        prevOutGain = 0.0f;
        hasPrevParams = false;
        lastTriggeredNote = 60;
        playheadDisplay.store(-1.0f);
    }

    const ModalResonatorBank& getModalResonator() const noexcept { return modalResonator; }

    GrainVisualizerData visualizer;
    std::atomic<float> playheadDisplay { -1.0f }; // 最新ボイスの実プレイヘッド (GUI用)

    static constexpr int NUM_VOICES = 24;
    static constexpr int MAX_TOTAL_GRAINS = 1200; // 全ボイス合計のCPUバジェット

private:
    void handleMidiMessage(const juce::MidiMessage& msg, const GranularParams& p, ModMatrix* modMatrix);
    void renderVoicesRange(juce::AudioBuffer<float>& subBuf,
                           const juce::AudioBuffer<float>* src,
                           const SampleMipLevels* mipLevels,
                           const GrainAtlas* atlas,
                           ModMatrix* modMatrix,
                           int trimStart, int trimEnd,
                           const GranularParams& baseP,
                           const GranularParams& subP);
    void generateWindowTables();

    std::array<GranularVoice, NUM_VOICES> voices;
    std::array<std::array<float, 4096>, 4> windowTables;

    GrainSwarmScheduler swarmScheduler;
    ModalResonatorBank  modalResonator;

    // マスターフィルタ: [ch][stage] (stage1は24dB/oct Butterworth Q2=1.30656296)
    TptSvf svfHP[2][2];
    TptSvf svfLP[2][2];

    GranularParams prevParams;
    bool  hasPrevParams = false;
    float prevOutGain = 0.0f;   // 出力ゲインのランプ用
    int   lastTriggeredNote = 60;

    double currentSampleRate = 44100.0;
    uint64_t noteCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularSynth)
};
