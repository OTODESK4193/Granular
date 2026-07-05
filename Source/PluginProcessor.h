// ==========================================
// File: PluginProcessor.h
// Granular - 24-voice Granular Synth (OTODESK)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

#include "DSP/GranularEngine.h"
#include "DSP/PitchAnalyzer.h"
#include "DSP/Arpeggiator.h"
#include "DSP/FxChain.h"
#include "DSP/ModMatrix.h"

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
    juce::String getFileName() const { return fileToIngest.getFileName(); }
    bool hasSampleFile() const { return fileToIngest.existsAsFile(); }

    // --- Dryサンプル・プレビュー再生 (取り込んだ原音を試聴) ---
    void startDryPreview() { dryPreviewPos = -1.0; dryPreviewActive.store(true); }
    void stopDryPreview()  { dryPreviewActive.store(false); }
    std::atomic<bool> dryPreviewActive { false };

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> sharedAudioBuffer;

    // --- GUI 連携 ---
    std::array<float, 1000> waveformPeaks;
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
    std::atomic<double> bufferSampleRate { 44100.0 }; // sharedAudioBufferが作られたSR

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
    juce::File fileToIngest;
    double currentSampleRate = 44100.0;
    PitchInfo currentPitch;
    juce::AudioBuffer<float> fxDryBuffer;   // FX Dry/Wet 用のプリFXコピー
    double dryPreviewPos = -1.0;            // Dryプレビューの再生位置 (-1=未初期化)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularAudioProcessor)
};
