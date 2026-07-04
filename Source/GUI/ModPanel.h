// ==========================================
// File: ModPanel.h
// MODタブ: ソース段 (LFO×4 / ENV×2 / MACRO×4) + 16スロットマトリクス
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "GlowToggle.h"
#include "ColorPalette.h"

class ModPanel : public juce::Component
{
public:
    explicit ModPanel(GranularAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void setupKnob(ValueKnob& s, const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, const juce::String& paramID, juce::Component& parent);

    GranularAudioProcessor& proc;

    // --- LFO ×4 ---
    std::array<juce::ComboBox, ModMatrix::kNumLfos> lfoWaveBox;
    std::array<std::unique_ptr<GlowToggle>, ModMatrix::kNumLfos> lfoSyncButton;
    std::array<ValueKnob, ModMatrix::kNumLfos> lfoRateKnob;
    std::array<juce::ComboBox, ModMatrix::kNumLfos> lfoSyncRateBox;

    // --- ENV ×2 ---
    std::array<ValueKnob, ModMatrix::kNumEnvs> envAttackKnob, envDecayKnob;
    std::array<std::unique_ptr<GlowToggle>, ModMatrix::kNumEnvs> envLoopButton;

    // --- MACRO ×4 ---
    std::array<ValueKnob, ModMatrix::kNumMacros> macroKnob;

    // --- マトリクス16行 (Viewport内) ---
    juce::Viewport matrixViewport;
    juce::Component matrixContent;
    std::array<juce::Label, ModMatrix::kNumSlots> rowLabel;
    std::array<juce::ComboBox, ModMatrix::kNumSlots> srcBox, dstBox;
    std::array<ValueKnob, ModMatrix::kNumSlots> amtSlider;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModPanel)
};
