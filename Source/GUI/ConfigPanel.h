// ==========================================
// File: ConfigPanel.h
// CONFIGタブ: 解析モード / リミッター / システム / サードパーティ表記
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "ColorPalette.h"

class ConfigPanel : public juce::Component,
                    private juce::Timer
{
public:
    explicit ConfigPanel(GranularAudioProcessor& processor);
    ~ConfigPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum ModKind { LinearAdd, ExpOct };
    void timerCallback() override;
    void applyModRing(juce::Slider& s, int dst, ModKind kind, float scale);

private:
    GranularAudioProcessor& proc;

    juce::ComboBox materialBox;
    juce::Label    materialLabel;

    // LIMITER 詳細: Release
    ValueKnob   limReleaseKnob;
    juce::Label limReleaseLabel;

    // SYSTEM: Polyphony / Filter Slope
    ValueKnob   polyKnob;
    juce::Label polyLabel;
    juce::ComboBox slopeBox;
    juce::Label    slopeLabel;
    juce::ComboBox themeBox;
    juce::Label    themeLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigPanel)
};
