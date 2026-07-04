// ==========================================
// File: ArpPanel.h
// ARPタブ: アルペジエイターのコントロール
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "GlowToggle.h"
#include "ColorPalette.h"

class ArpPanel : public juce::Component
{
public:
    explicit ArpPanel(GranularAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void setupKnob(ValueKnob& s, juce::Label& l, const juce::String& text,
                   const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& text,
                    const juce::String& paramID);

    GranularAudioProcessor& proc;

    GlowToggle enableButton { "ON",    GranularColors::mint };
    GlowToggle latchButton  { "LATCH", GranularColors::lilac };
    GlowToggle syncButton   { "SYNC",  GranularColors::babyBlue };

    juce::ComboBox patternBox, rateBox, durBox;
    juce::Label    patternLabel, rateLabel, durLabel;

    ValueKnob octavesKnob, rateFreeKnob, gateKnob;
    juce::Label octavesLabel, rateFreeLabel, gateLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpPanel)
};
