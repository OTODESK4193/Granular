// ==========================================
// File: ConfigPanel.cpp
// ==========================================
#include "ConfigPanel.h"

namespace
{
    constexpr int kKnobW = 78;
    constexpr int kKnobH = 74;
    constexpr int kLabelH = 16;
}

ConfigPanel::ConfigPanel(GranularAudioProcessor& p) : proc(p)
{
    // MATERIAL (解析モード)
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("materialMode")))
        materialBox.addItemList(choiceParam->choices, 1);
    addAndMakeVisible(materialBox);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "materialMode", materialBox));

    materialLabel.setText("MATERIAL", juce::dontSendNotification);
    materialLabel.setJustificationType(juce::Justification::centredLeft);
    materialLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    materialLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(materialLabel);

    // CEILING (リミッター天井)
    ceilingKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ceilingKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    ceilingKnob.setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentMaster);
    ceilingKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(ceilingKnob);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "ceiling", ceilingKnob));

    ceilingLabel.setText("CEILING", juce::dontSendNotification);
    ceilingLabel.setJustificationType(juce::Justification::centred);
    ceilingLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    ceilingLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(ceilingLabel);
}

void ConfigPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("ANALYSIS", 20,  6, 260, GranularColors::accentPitch);
    drawSection("LIMITER",  320, 6, 160, GranularColors::accentMaster);
    drawSection("SYSTEM",   520, 6, 540, GranularColors::accentGrain);

    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Change MATERIAL, then press ANALYZE (header) to re-analyze the sample.",
               20, 116, 480, 14, juce::Justification::centredLeft);
    g.drawText("CEILING: brickwall limiter output ceiling (default -0.1 dB).",
               320, 134, 480, 14, juce::Justification::centredLeft);

    g.setColour(GranularColors::textDim.withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Color Theme / Polyphony / Analysis options - coming soon",
               520, 40, 540, 20, juce::Justification::centredLeft);
}

void ConfigPanel::resized()
{
    materialLabel.setBounds(20, 34, 100, 14);
    materialBox.setBounds(20, 50, 160, 26);

    ceilingKnob.setBounds(320, 34, kKnobW - 8, kKnobH);
    ceilingLabel.setBounds(320, 34 + kKnobH, kKnobW - 8, kLabelH);
}
