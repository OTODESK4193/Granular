// ==========================================
// File: EnginePanel.h
// ENGINEタブ: Phase 4 グラニュラー拡張パネル
//  - PLAYBACK ENGINE (Granular / WSOLA / TD-PSOLA, Formant Shift, Diatonic Swarm, Atlas Pitch)
//  - CONCATENATIVE ATLAS (Timbre Scrubbing & Bias)
//  - GRAIN SHAPE & PER-GRAIN SVF (Skew, Chirp, Filter Mode / Cutoff / Res / Jitter)
//  - SCALE-TUNED MODAL RESONATOR BANK (Ratio Mode, Mix, Decay, Damp, Inharmonicity)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "ColorPalette.h"

class EnginePanel : public juce::Component,
                    private juce::Timer
{
public:
    explicit EnginePanel(GranularAudioProcessor& processor);
    ~EnginePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    enum ModKind { LinearAdd, ExpOct };
    void timerCallback() override;
    void applyModRing(juce::Slider& s, int dst, ModKind kind, float scale);

    GranularAudioProcessor& proc;

    // Row 1: ENGINE & SWARM / CONCATENATIVE ATLAS / GRAIN SHAPE
    juce::ComboBox engineBox;
    juce::Label    engineLabel;
    ValueKnob   formantKnob, swarmKnob, pitchLockKnob;
    juce::Label formantLabel, swarmLabel, pitchLockLabel;

    ValueKnob   timbreScrubKnob, timbreAmtKnob;
    juce::Label timbreScrubLabel, timbreAmtLabel;

    ValueKnob   skewKnob, chirpKnob;
    juce::Label skewLabel, chirpLabel;

    // Row 2: PER-GRAIN SVF / SCALE-TUNED MODAL RESONATOR BANK
    juce::ComboBox filterModeBox;
    juce::Label    filterModeLabel;
    ValueKnob   filterFreqKnob, filterResKnob, filterJitterKnob;
    juce::Label filterFreqLabel, filterResLabel, filterJitterLabel;

    juce::ComboBox modalModeBox;
    juce::Label    modalModeLabel;
    ValueKnob   modalMixKnob, modalDecayKnob, modalDampKnob, modalInharmKnob;
    juce::Label modalMixLabel, modalDecayLabel, modalDampLabel, modalInharmLabel;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnginePanel)
};
