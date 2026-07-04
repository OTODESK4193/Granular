// ==========================================
// File: MainPanel.h
// MAINタブ: グレイン/ピッチ/エンベロープ/量子化/マスターの全コントロール
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"

class MainPanel : public juce::Component
{
public:
    explicit MainPanel(GranularAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text,
                   const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, const juce::String& paramID);
    void setupComboLabel(juce::Label& l, const juce::String& text);

    GranularAudioProcessor& proc;

    // --- GRAIN ---
    juce::Slider sizeKnob, densityKnob, sprayKnob, jitterKnob, panKnob, speedKnob;
    juce::Label  sizeLabel, densityLabel, sprayLabel, jitterLabel, panLabel, speedLabel;

    // --- PITCH ---
    juce::Slider octaveKnob, semiKnob, fineKnob;
    juce::Label  octaveLabel, semiLabel, fineLabel;

    // --- ENV ---
    juce::Slider attackKnob, decayKnob, sustainKnob, releaseKnob;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseLabel;

    // --- TEXTURE (音程感強調) ---
    juce::Slider toneKnob;
    juce::Label  toneLabel;
    juce::ToggleButton syncButton { "SYNC" };
    juce::ToggleButton revButton { "REV" };

    // --- MASTER ---
    juce::Slider hpfKnob, lpfKnob, gainKnob;
    juce::Label  hpfLabel, lpfLabel, gainLabel;

    // --- コンボ ---
    juce::ComboBox keyBox, scaleBox, windowBox, materialBox;
    juce::Label    keyLabel, scaleLabel, windowLabel, materialLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
