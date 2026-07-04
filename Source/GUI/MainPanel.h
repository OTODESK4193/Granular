// ==========================================
// File: MainPanel.h
// MAINタブ: グレイン/ピッチ/エンベロープ/量子化/マスターの全コントロール
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "GlowToggle.h"
#include "ColorPalette.h"

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
    ValueKnob sizeKnob, densityKnob, sprayKnob, jitterKnob, panKnob, speedKnob;
    juce::Label sizeLabel, densityLabel, sprayLabel, jitterLabel, panLabel, speedLabel;

    // --- PITCH ---
    ValueKnob octaveKnob, semiKnob, fineKnob;
    juce::Label octaveLabel, semiLabel, fineLabel;

    // --- ENV ---
    ValueKnob attackKnob, decayKnob, sustainKnob, releaseKnob;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;

    // --- TEXTURE (音程感強調) ---
    ValueKnob toneKnob;
    juce::Label toneLabel;
    GlowToggle syncButton { "SYNC", GranularColors::mint };
    GlowToggle revButton  { "REV",  GranularColors::rose };

    // --- SAMPLE (Start/End) ---
    ValueKnob startKnob, endKnob;
    juce::Label startLabel, endLabel;

    // --- MASTER ---
    ValueKnob hpfKnob, lpfKnob, gainKnob;
    juce::Label hpfLabel, lpfLabel, gainLabel;

    // --- コンボ ---
    juce::ComboBox keyBox, scaleBox, windowBox, materialBox;
    juce::Label    keyLabel, scaleLabel, windowLabel, materialLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
