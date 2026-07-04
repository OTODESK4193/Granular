// ==========================================
// File: MainPanel.h
// MAINタブ: グレイン/ピッチ/エンベロープ/サンプル範囲/マスター
// (Quantize/TextureはARPタブ、Material/CeilingはCONFIGタブへ移動)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
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

    // --- SAMPLE (Position/Start/End) ---
    ValueKnob positionKnob, startKnob, endKnob;
    juce::Label positionLabel, startLabel, endLabel;

    // --- MASTER ---
    ValueKnob hpfKnob, lpfKnob, gainKnob;
    juce::Label hpfLabel, lpfLabel, gainLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainPanel)
};
