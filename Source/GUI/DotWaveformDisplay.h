// ==========================================
// File: DotWaveformDisplay.h
// パステル・ドットマトリクス波形ディスプレイ
//  - 波形を小さなドットの縦積みで表現（横方向にパステルグラデーション）
//  - グレイン発火をリアルタイムにドットで可視化（ピッチ→高さ/色、時間で減衰）
//  - サンプルのドラッグ&ドロップ / クリックで Position 操作
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <vector>

#include "../PluginProcessor.h"

class DotWaveformDisplay : public juce::Component,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    explicit DotWaveformDisplay(GranularAudioProcessor& processor);
    ~DotWaveformDisplay() override;

    void paint(juce::Graphics& g) override;

    // --- D&D ---
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit(const juce::StringArray&) override { dragHover = false; repaint(); }

    // --- Position 操作 ---
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void pullGrainEvents();
    void setPositionFromMouse(const juce::MouseEvent& e, bool isStart);
    juce::Rectangle<float> getWaveArea() const;

    struct LiveDot
    {
        float normPos = 0.0f;
        float pan = 0.5f;
        float semitones = 0.0f;
        double spawnTimeMs = 0.0;
        double durationMs = 100.0;
    };

    GranularAudioProcessor& proc;
    std::vector<LiveDot> liveDots;
    uint32_t lastEventCount = 0;
    bool dragHover = false;
    bool draggingPosition = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotWaveformDisplay)
};
