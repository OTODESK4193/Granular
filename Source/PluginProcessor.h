// ==========================================
// File: PluginProcessor.h
// Granular - 24-voice Granular Synth (OTODESK)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "DSP/GranularEngine.h"
#include "DSP/PitchAnalyzer.h"
#include "DSP/Arpeggiator.h"
#include "DSP/FxChain.h"
#include "DSP/ModMatrix.h"

struct IngestedSample
{
    juce::AudioBuffer<float> buffer;
    int validSamples = 0;
    int trimStart = 0;
    int trimEnd = 0;
    double sampleRate = 44100.0;
    int detectedRoot = 0;
    bool isMajor = true;
    int rootMidiNote = 60;
};

enum class IngestState
{
    Empty, Loading, Analyzing, Correcting, Normalizing, Ready, Failed
};

class GranularAudioProcessor : public juce::AudioProcessor,
                               public juce::Thread,
                               public juce::ChangeBroadcaster
{
public:
    GranularAudioProcessor();
    ~GranularAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- サンプル読み込み（背景スレッド） ---
    void loadFileForIngest(const juce::File& file);
    void reingestCurrentFile();   // ANALYZE: 現在のファイルを再解析
    void clearSample();           // DELETE: サンプルを破棄
    void run() override;
    static bool readAbletonAif(const juce::File& file, juce::AudioBuffer<float>& destBuffer, double& sampleRate);
    juce::String getFileName() const { const juce::ScopedLock sl(stateLock); return fileToIngest.getFileName(); }
    bool hasSampleFile() const { const juce::ScopedLock sl(stateLock); return fileToIngest.existsAsFile(); }

    // --- Dryサンプル・プレビュー再生 (取り込んだ原音を試聴) ---
    void startDryPreview() { dryPreviewResetPos.store(true, std::memory_order_release); dryPreviewActive.store(true, std::memory_order_release); }
    void stopDryPreview()  { dryPreviewActive.store(false, std::memory_order_release); }
    std::atomic<bool> dryPreviewActive { false };

    // --- PANIC: 全発音・残響・エフェクトの即時消音 ---
    void panic();

    // --- GUI スケーリング (Wavetable 準拠) ---
    int getSavedEditorWidth() const noexcept { return savedEditorWidth; }
    int getSavedEditorHeight() const noexcept { return savedEditorHeight; }
    void setSavedEditorSize(int w, int h) noexcept
    {
        savedEditorWidth = w;
        savedEditorHeight = h;
    }

    // --- プリセット ---
    void applyFactoryPreset(int index);                 // ファクトリー(パラメータ上書き)
    void resetAllParametersToDefault();
    void randomizePreset();                             // ノブ+MOD+FXをランダマイズ
    juce::File getUserPresetDir() const;                // Documents/OTODESK/Granular/Presets
    bool saveUserPreset(const juce::String& name, const juce::String& category = "User", const juce::String& sub = ""); // ユーザープリセット保存
    void loadUserPresetFile(const juce::File& file);    // ユーザープリセット読込
    juce::String currentPresetName = "Init";

    juce::AudioFormatManager formatManager;

    // --- GUI 連携 ---
    std::array<float, 1000> waveformPeaks {};
    std::array<float, 1000> waveformBright {}; // 列毎スペクトル明度(0=低域/1=高域)
    std::atomic<bool> waveformReady { false };
    std::atomic<IngestState> ingestState { IngestState::Empty };
    std::atomic<float> ingestProgress { 0.0f };

    std::atomic<int> validSampleCount { 0 };
    std::atomic<int> trimStartSample { 0 };
    std::atomic<int> trimEndSample { 0 };

    std::atomic<int> currentDetectedRoot { 0 };
    std::atomic<bool> currentIsMajor { true };
    std::atomic<int> rootMidiNote { 60 };   // Root Note Tracking (Ableton: C3=60)
    std::atomic<double> bufferSampleRate { 44100.0 }; // 公開済みサンプルが作られたSR

    GranularSynth synth; // synth.visualizer を GUI が参照する
    Arpeggiator arp;
    FxChain fxChain;
    BrickLimiter masterLimiter;
    ModMatrix modMatrix;

    juce::CriticalSection stateLock;
    juce::String hudLineFile = "File: ---";
    juce::String hudLineTask = "Task: Drop a sample";
    juce::String hudLineOrig = "Original: ---";
    juce::String hudLineCorr = "Corrected: ---";

private:
    struct CachedParams
    {
        std::atomic<float>* attack = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* sustain = nullptr;
        std::atomic<float>* release = nullptr;

        std::atomic<float>* grainSize = nullptr;
        std::atomic<float>* density = nullptr;
        std::atomic<float>* position = nullptr;
        std::atomic<float>* positionSpray = nullptr;
        std::atomic<float>* pitchJitter = nullptr;
        std::atomic<float>* panSpread = nullptr;
        std::atomic<float>* speed = nullptr;

        std::atomic<float>* playMode = nullptr;
        std::atomic<float>* speedSync = nullptr;
        std::atomic<float>* speedSyncDiv = nullptr;
        std::atomic<float>* reverseGrain = nullptr;
        std::atomic<float>* windowShape = nullptr;

        std::atomic<float>* octave = nullptr;
        std::atomic<float>* pitchSt = nullptr;
        std::atomic<float>* fineTune = nullptr;

        std::atomic<float>* scaleOn = nullptr;
        std::atomic<float>* key = nullptr;
        std::atomic<float>* scale = nullptr;

        std::atomic<float>* pitchSync = nullptr;
        std::atomic<float>* spraySpeed = nullptr;
        std::atomic<float>* tone = nullptr;

        std::atomic<float>* materialMode = nullptr;
        std::atomic<float>* sampleStart = nullptr;
        std::atomic<float>* sampleEnd = nullptr;

        std::atomic<float>* ceiling = nullptr;
        std::atomic<float>* limRelease = nullptr;
        std::atomic<float>* filterSlope = nullptr;
        std::atomic<float>* poly = nullptr;
        std::atomic<float>* fxMix = nullptr;
        std::atomic<float>* colorTheme = nullptr;

        std::atomic<float>* masterHPF = nullptr;
        std::atomic<float>* masterLPF = nullptr;
        std::atomic<float>* outGain = nullptr;

        std::array<std::atomic<float>*, FxChain::kNumSlots> fxType {};
        std::array<std::atomic<float>*, FxChain::kNumSlots> fxAmount {};

        std::atomic<float>* satAlgo = nullptr;
        std::atomic<float>* satDrive = nullptr;
        std::atomic<float>* satPreHz = nullptr;
        std::atomic<float>* satTrim = nullptr;

        std::atomic<float>* choRate = nullptr;
        std::atomic<float>* choDepth = nullptr;
        std::atomic<float>* choWidth = nullptr;

        std::atomic<float>* dlyTime = nullptr;
        std::atomic<float>* dlyFeedback = nullptr;
        std::atomic<float>* dlyDuck = nullptr;
        std::atomic<float>* dlyDamp = nullptr;

        std::atomic<float>* frzSize = nullptr;
        std::atomic<float>* frzFeedback = nullptr;
        std::atomic<float>* frzDamp = nullptr;

        std::atomic<float>* revDecay = nullptr;
        std::atomic<float>* revShimmer = nullptr;
        std::atomic<float>* revDamp = nullptr;
        std::atomic<float>* revMod = nullptr;

        std::atomic<float>* gateRate = nullptr;
        std::atomic<float>* gatePattern = nullptr;
        std::atomic<float>* gateDepth = nullptr;
        std::atomic<float>* gateDecay = nullptr;
        std::atomic<float>* gateVowel = nullptr;

        std::array<std::atomic<float>*, ModMatrix::kNumLfos> lfoRate {};
        std::array<std::atomic<float>*, ModMatrix::kNumLfos> lfoSync {};
        std::array<std::atomic<float>*, ModMatrix::kNumLfos> lfoSyncRate {};
        std::array<std::atomic<float>*, ModMatrix::kNumLfos> lfoWave {};

        std::array<std::atomic<float>*, ModMatrix::kNumEnvs> envAttack {};
        std::array<std::atomic<float>*, ModMatrix::kNumEnvs> envDecay {};
        std::array<std::atomic<float>*, ModMatrix::kNumEnvs> envSustain {};
        std::array<std::atomic<float>*, ModMatrix::kNumEnvs> envRelease {};
        std::array<std::atomic<float>*, ModMatrix::kNumEnvs> envLoop {};

        std::array<std::atomic<float>*, ModMatrix::kNumSlots> modSrc {};
        std::array<std::atomic<float>*, ModMatrix::kNumSlots> modDst {};
        std::array<std::atomic<float>*, ModMatrix::kNumSlots> modAmt {};
        std::array<std::atomic<float>*, ModMatrix::kNumSlots> modUni {};

        std::atomic<float>* arpEnable = nullptr;
        std::atomic<float>* arpPattern = nullptr;
        std::atomic<float>* arpOctaves = nullptr;
        std::atomic<float>* arpSync = nullptr;
        std::atomic<float>* arpRateSync = nullptr;
        std::atomic<float>* arpRateFree = nullptr;
        std::atomic<float>* arpDurMode = nullptr;
        std::atomic<float>* arpGate = nullptr;
        std::atomic<float>* arpLatch = nullptr;
    };

    void initParamCache();
    juce::File getFileToIngestCopy() const;
    void retireCurrentSample();
    void publishIngestedSample(std::shared_ptr<const IngestedSample> next);
    void reclaimRetiredSamplesLocked();
    void resetPhysicalToSnapped() noexcept;

    struct RetiredSample
    {
        std::shared_ptr<const IngestedSample> sample;
        uint64_t minSafeEpoch = 0;
    };

    CachedParams cp;
    juce::File fileToIngest;
    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<bool> panicRequested { false };

    // Wait-free オーディオ参照用イミュータブルサンプルスナップショット
    std::atomic<const IngestedSample*> publishedSample { nullptr };
    std::atomic<int> audioCallbacksInFlight { 0 };
    std::atomic<uint64_t> audioCompletedEpoch { 0 };
    juce::CriticalSection sampleOwnerLock;
    std::shared_ptr<const IngestedSample> currentOwnedSample;
    std::vector<RetiredSample> retiredSamples;

    PitchInfo currentPitch;
    juce::AudioBuffer<float> fxDryBuffer;   // FX Dry/Wet 用のプリFXコピー
    juce::MidiBuffer quantizedMidi;         // スケール量子化用プリアロケートバッファ
    std::atomic<bool> dryPreviewResetPos { false };
    double dryPreviewPos = -1.0;            // Dryプレビューの再生位置 (オーディオ専用)

    std::array<std::array<int8_t, 128>, 16> physicalToSnapped {}; // チャネル別NoteOn/NoteOff整合性
    int pendingSnappedCount = 0;
    bool prevArpEnabled = false;

    int savedEditorWidth { 1080 };
    int savedEditorHeight { 640 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularAudioProcessor)
};
