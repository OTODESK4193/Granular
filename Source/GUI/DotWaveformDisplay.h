// ==========================================
// File: DotWaveformDisplay.h
// パステル・ドットマトリクス波形ディスプレイ
//  - 波形を小さなドットの縦積みで表現（横方向にパステルグラデーション）
//  - グレイン発火をリアルタイムにドットで可視化（ピッチ→高さ/色、時間で減衰）
//  - Start/End マーカーをドラッグしてサンプル範囲を調整
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

    // --- Position / Start / End 操作 ---
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    enum class DragMode { None, Position, StartMarker, EndMarker };

    struct RegionGeometry
    {
        float autoSN = 0.0f, autoEN = 1.0f;  // 自動トリム (フルバッファ正規化)
        float effSN = 0.0f, effEN = 1.0f;    // 実効範囲   (フルバッファ正規化, 生パラメータ)
        float effSNmod = 0.0f, effENmod = 1.0f; // Start/End変調を反映したマーカー位置
        float waveX = 0.0f, waveW = 1.0f;    // 描画領域
        float effX = 0.0f, effW = 1.0f;      // 実効範囲のピクセル座標
    };

    void timerCallback() override;
    void pullGrainEvents();
    RegionGeometry getRegionGeometry() const;
    void updateDraggedParam(const juce::MouseEvent& e);
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
    DragMode dragMode = DragMode::None;
    juce::RangedAudioParameter* draggedParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DotWaveformDisplay)
};
