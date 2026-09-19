// ==========================================
// File: ModPanel.h
// MODタブ: ソース段 (LFO/Env サブタブ) + 16スロットマトリクス (Page1/Page2)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "GlowToggle.h"
#include "ModDestSelector.h"
#include "ColorPalette.h"

class ModPanel : public juce::Component,
                 private juce::Timer
{
public:
    explicit ModPanel(GranularAudioProcessor& processor);
    ~ModPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    enum ModKind { LinearAdd, ExpOct };
    void timerCallback() override;
    void applyModRing(juce::Slider& s, int dst, ModKind kind, float scale);

    void setupKnob(ValueKnob& s, const juce::String& paramID, juce::Colour accent);
    void setupCombo(juce::ComboBox& c, const juce::String& paramID, juce::Component& parent);
    void setSourceTab(int t);          // 0=LFO, 1=Env
    void setMatrixPage(int p);         // 0=slot1-8, 1=slot9-16
    void styleTab(juce::TextButton& b, bool active);
    juce::PopupMenu buildDestMenu(int currentDst);

    GranularAudioProcessor& proc;

    // --- ソース・サブタブ ---
    juce::TextButton lfoTabBtn   { "LFO" };
    juce::TextButton envTabBtn   { "Env" };
    int activeSrcTab = 0;

    // --- LFO ×4 ---
    std::array<juce::ComboBox, ModMatrix::kNumLfos> lfoWaveBox;
    std::array<std::unique_ptr<GlowToggle>, ModMatrix::kNumLfos> lfoSyncButton;
    std::array<ValueKnob, ModMatrix::kNumLfos> lfoRateKnob;
    std::array<juce::ComboBox, ModMatrix::kNumLfos> lfoSyncRateBox;

    // --- ENV ×3 (ADSR) ---
    std::array<ValueKnob, ModMatrix::kNumEnvs> envAttackKnob, envDecayKnob, envSustainKnob, envReleaseKnob;
    std::array<std::unique_ptr<GlowToggle>, ModMatrix::kNumEnvs> envLoopButton;

    // --- マトリクス16行 (2ページ) ---
    juce::TextButton page1Btn { "PAGE 1" };
    juce::TextButton page2Btn { "PAGE 2" };
    int matrixPage = 0;
    std::array<juce::Label, ModMatrix::kNumSlots> rowLabel;
    std::array<juce::ComboBox, ModMatrix::kNumSlots> srcBox;
    std::array<ModDestSelector, ModMatrix::kNumSlots> dstBox;
    std::array<ValueKnob, ModMatrix::kNumSlots> amtSlider;
    std::array<std::unique_ptr<GlowToggle>, ModMatrix::kNumSlots> uniButton;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>   sliderAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>   buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModPanel)
};
