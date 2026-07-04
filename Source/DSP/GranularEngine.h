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
#include <vector>

#include "ScaleQuantizer.h"
#include "GrainVisualizerData.h"

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
    float speed = 1.0f;           // -2..2 プレイヘッド速度

    int   octave = 0;
    int   pitchSt = 0;
    float fineTune = 0.0f;        // セント
    float outGainDb = -12.0f;
    bool  reverseGrain = false;
    int   windowShape = 0;        // 0..3

    int   key = 0;                // 0=Auto(解析結果), 1..12 = C..B
    int   scale = 0;              // ScaleQuantizer のインデックス (0=Free)

    bool  pitchSync = false;      // グレイン発火をノート周波数に同期（音程感強調）
    float tone = 0.0f;            // 0..1 ノート同調コムレゾネーター量（音程感強調）

    float masterHPF = 20.0f;
    float masterLPF = 20000.0f;

    int   detectedRoot = 0;       // 解析で得たルート (key==Auto時に使用)
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
    double playheadOffset = 0.0;
    int    samplesToNextSpawn = 0;

    juce::ADSR adsr;

    static constexpr int MAX_GRAINS = 100;   // Quanta 2 同等の1ボイス上限
    std::array<Grain, MAX_GRAINS> grains;

    FastRandom random;
    juce::AudioBuffer<float> voiceInternalBuffer;

    // --- TONE: ノート同調コムレゾネーター (Karplus-Strong系) ---
    std::vector<float> combBufL, combBufR;
    int   combWriteIdx = 0;
    float combLpL = 0.0f, combLpR = 0.0f;

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

    GrainVisualizerData visualizer;

    static constexpr int NUM_VOICES = 24;
    static constexpr int MAX_TOTAL_GRAINS = 1200; // 全ボイス合計のCPUバジェット

private:
    void handleMidi(const juce::MidiBuffer& midi, const GranularParams& p);
    void generateWindowTables();

    std::array<GranularVoice, NUM_VOICES> voices;
    std::array<std::array<float, 4096>, 4> windowTables;

    juce::IIRFilter filterHPF[2];
    juce::IIRFilter filterLPF[2];

    double currentSampleRate = 44100.0;
    uint64_t noteCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularSynth)
};
