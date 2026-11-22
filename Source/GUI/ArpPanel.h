// ==========================================
// File: ArpPanel.h
// ARPタブ: アルペジエイター + QUANTIZE(Key/Scale) + TEXTURE
// (Key/ScaleはARPとグレイン量子化の両方に効くため同居)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "GlowToggle.h"
#include "ColorPalette.h"
#include "ScaleSelector.h"
#include "KnobModMenu.h"

class ArpPanel : public juce::Component,
                 private juce::Timer
{
public:
    explicit ArpPanel(GranularAudioProcessor& processor);
    ~ArpPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override { if (isVisible()) timerCallback(); }

    GranularUI::ModTabNavigator onOpenModTab;

private:
    void setupKnob(ValueKnob& s, juce::Label& l, const juce::String& text,
                   const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& text,
                    const juce::String& paramID);

    // --- MOD Arc ---
    enum ModKind { LinearAdd, ExpOct };
    void timerCallback() override;
    void applyModRing(juce::Slider& s, int dst, ModKind kind, float scale);

    GranularAudioProcessor& proc;

    // --- ARP ---
    GlowToggle enableButton { "ON",    GranularColors::mint };
    GlowToggle latchButton  { "LATCH", GranularColors::lilac };
    GlowToggle syncButton   { "SYNC",  GranularColors::babyBlue };
    GlowToggle euclidButton { "EUCLID", GranularColors::sage };

    juce::ComboBox patternBox, rateBox, durBox;
    juce::Label    patternLabel, rateLabel, durLabel;

    ValueKnob octavesKnob, rateFreeKnob, gateKnob;
    ValueKnob euclidStepsKnob, euclidFillsKnob, euclidRotKnob;
    juce::Label octavesLabel, rateFreeLabel, gateLabel;
    juce::Label euclidStepsLabel, euclidFillsLabel, euclidRotLabel;

    // --- QUANTIZE (3段構成: 1段目=ON/KEY, 2段目=SCALE, 3段目=NOTES) ---
    GlowToggle    scaleOnButton { "ON", GranularColors::accentPitch };
    juce::ComboBox keyBox;
    juce::Label    keyLabel, scaleLabel;
    ScaleSelector scaleSelector;
    juce::String  scaleNotesText;
    int           lastScaleKey = -1;
    int           lastScaleType = -1;

    // --- TEXTURE (MAINタブから移動 / SYNCはARPと区別するためP-SYNC表記) ---
    juce::ComboBox windowBox;
    juce::Label    windowLabel;
    ValueKnob toneKnob, spraySpeedKnob;
    juce::Label toneLabel, spraySpeedLabel;
    GlowToggle pitchSyncButton { "P-SYNC", GranularColors::mint };
    GlowToggle revButton       { "REV",    GranularColors::rose };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpPanel)
};
