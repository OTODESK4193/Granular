// ==========================================
// File: PluginEditor.h
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "PluginProcessor.h"
#include "GUI/ArcDial.h"
#include "GUI/DotWaveformDisplay.h"

class GranularAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit GranularAudioProcessorEditor(GranularAudioProcessor&);
    ~GranularAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text,
                   const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, const juce::String& paramID);

    GranularAudioProcessor& proc;
    ArcDialLookAndFeel lnf;

    DotWaveformDisplay waveDisplay;

    // --- GRAIN ---
    juce::Slider sizeKnob, densityKnob, sprayKnob, jitterKnob, panKnob, speedKnob;
    juce::Label  sizeLabel, densityLabel, sprayLabel, jitterLabel, panLabel, speedLabel;

    // --- PITCH ---
    juce::Slider octaveKnob, semiKnob, fineKnob;
    juce::Label  octaveLabel, semiLabel, fineLabel;

    // --- ENV ---
    juce::Slider attackKnob, decayKnob, sustainKnob, releaseKnob;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseLabel;

    // --- MASTER ---
    juce::Slider hpfKnob, lpfKnob, gainKnob;
    juce::Label  hpfLabel, lpfLabel, gainLabel;

    // --- コンボ / トグル ---
    juce::ComboBox keyBox, scaleBox, windowBox, materialBox;
    juce::Label    keyLabel, scaleLabel, windowLabel, materialLabel;
    juce::ToggleButton revButton { "REV" };
    juce::TextButton loadButton { "LOAD" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    // --- HUD ---
    juce::String hudFile, hudTask, hudOrig, hudCorr;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>                revAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularAudioProcessorEditor)
};
