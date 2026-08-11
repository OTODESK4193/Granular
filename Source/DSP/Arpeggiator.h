// ==========================================
// File: Arpeggiator.h
// 抑えた鍵盤をばらすアルペジエイター
//  - パターン13種（基本〜音楽的）
//  - TempoSync（付点・三連対応）/ Free(Hz)
//  - Duration: Gate% または 音符指定
//  - Latch（鍵盤を離しても保持、新しいコードで入れ替え）
//  - 出力ノートは Quantize の Key/Scale にスナップ
//  - サンプル精度のステップ/ゲートスケジューリング
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

#include "ScaleQuantizer.h"

struct ArpParams
{
    bool  enable = false;
    int   pattern = 0;       // 下記Pattern列挙と対応
    int   octaves = 1;       // 1..4
    bool  sync = true;
    int   rateSync = 6;      // レート表 (デフォルト 1/8)
    float rateFree = 8.0f;   // Hz
    int   durMode = 0;       // 0=Gate%, 1=1/32, 2=1/16, 3=1/8, 4=1/4
    float gate = 0.8f;       // 0.05..1.0
    bool  latch = false;

    // Quantize連携
    int key = 0;             // 0=Auto, 1..12=C..B
    int scale = 0;           // 0=Free
    int detectedRoot = 0;

    double bpm = 120.0;
};

class Arpeggiator
{
public:
    enum Pattern
    {
        Up = 0, Down, UpDown, DownUp, UpDownIncl,
        Converge, Diverge, PedalLow, PedalHigh,
        AsPlayed, Random, RandomWalk, Chord,
        NumPatterns
    };

    static juce::StringArray getPatternNames()
    {
        return { "Up", "Down", "Up-Down", "Down-Up", "Up-Down Incl",
                 "Converge", "Diverge", "Pedal Low", "Pedal High",
                 "As Played", "Random", "Random Walk", "Chord" };
    }

    static juce::StringArray getRateNames()
    {
        return { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4", "1/4.", "1/4T",
                 "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T", "1/32" };
    }

    static juce::StringArray getDurationNames()
    {
        return { "Gate %", "1/32", "1/16", "1/8", "1/4" };
    }

    void prepare(double sr)
    {
        sampleRate = sr;
        // オーディオスレッドでの再アロケーションを避けるため事前確保
        held.reserve(32);
        seq.reserve(256);
        active.reserve(256);
        scratchSorted.reserve(32);
        scratchAsc.reserve(160);
        scratchConv.reserve(160);
        reset();
    }

    void reset();

    // midi を消費し、アルペジオ化した MIDI に置き換える
    void process(juce::MidiBuffer& midi, int numSamples, const ArpParams& p);

private:
    struct PoolNote { int note = 60; float vel = 1.0f; };
    struct ActiveNote { int note = 60; int samplesLeft = 0; };

    void handleInputEvent(const juce::MidiMessage& msg, const ArpParams& p);
    void buildSequence(const ArpParams& p);       // held → seq (パターン展開)
    int  quantizeNote(int note, const ArpParams& p) const;
    void fireStep(juce::MidiBuffer& out, int offset, int gateSamples, const ArpParams& p);
    double getStepSamples(const ArpParams& p) const;
    int    getGateSamples(double stepSamples, const ArpParams& p) const;

    double sampleRate = 44100.0;

    struct HeldNote { int note = 60; float vel = 1.0f; int order = 0; };
    std::vector<HeldNote> held;              // アルペジオ対象（Latch中は離鍵後も保持）
    std::array<bool, 128> physical {};       // 物理的に押されている鍵盤
    int physicalCount = 0;
    int orderCounter = 0;

    std::vector<PoolNote> seq;               // パターン展開済みシーケンス
    int seqPos = 0;
    int walkPos = 0;

    double samplesToNextStep = 0.0;
    bool running = false;
    bool wasEnabled = false;

    std::vector<ActiveNote> active;          // noteOff待ち
    juce::Random random;

    // buildSequence用スクラッチ（アロケーション回避）
    std::vector<HeldNote> scratchSorted;
    std::vector<PoolNote> scratchAsc;
    std::vector<PoolNote> scratchConv;
};
