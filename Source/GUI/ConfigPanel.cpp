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
    auto setupKnob = [this](ValueKnob& k, juce::Label& l, const juce::String& text, const juce::String& id)
    {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k.setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentMaster);
        k.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(k);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, id, k));

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        l.setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(l);
    };
    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::String& id)
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)))
            c.addItemList(choiceParam->choices, 1);
        addAndMakeVisible(c);
        comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            proc.apvts, id, c));

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        l.setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(l);
    };

    // ANALYSIS: MATERIAL
    setupCombo(materialBox, materialLabel, "MATERIAL", "materialMode");

    // LIMITER: Release (天井CeilingはMainタブへ移設)
    setupKnob(limReleaseKnob, limReleaseLabel, "RELEASE", "limRelease");

    // SYSTEM: Polyphony / Filter Slope / Color Theme
    setupKnob(polyKnob, polyLabel, "POLY", "poly");
    setupCombo(slopeBox, slopeLabel, "FILTER SLOPE", "filterSlope");
    setupCombo(themeBox, themeLabel, "COLOR THEME", "colorTheme");
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

    drawSection("ANALYSIS", 20,  6, 220, GranularColors::accentPitch);
    drawSection("LIMITER",  280, 6, 120, GranularColors::accentMaster);
    drawSection("SYSTEM",   440, 6, 620, GranularColors::accentGrain);

    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Change MATERIAL, then press ANALYZE (header) to re-analyze the sample.",
               20, 120, 240, 28, juce::Justification::topLeft);
    g.drawText("Limiter RELEASE (ms). Ceiling is on the MAIN tab.",
               280, 120, 150, 40, juce::Justification::topLeft);
    g.drawText("POLY: max simultaneous voices.  FILTER SLOPE: master HPF/LPF steepness (12 or 24 dB/oct).",
               440, 120, 620, 14, juce::Justification::topLeft);

    g.setColour(GranularColors::textDim.withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("Color Theme / Analysis options - coming soon",
               440, 138, 620, 18, juce::Justification::topLeft);
}

void ConfigPanel::resized()
{
    // ANALYSIS
    materialLabel.setBounds(20, 34, 100, 14);
    materialBox.setBounds(20, 50, 160, 26);

    // LIMITER: Release ノブ
    limReleaseKnob.setBounds(280, 34, kKnobW - 8, kKnobH);
    limReleaseLabel.setBounds(280, 34 + kKnobH, kKnobW - 8, kLabelH);

    // SYSTEM: Poly ノブ + Filter Slope コンボ
    polyKnob.setBounds(440, 34, kKnobW - 8, kKnobH);
    polyLabel.setBounds(440, 34 + kKnobH, kKnobW - 8, kLabelH);

    slopeLabel.setBounds(540, 40, 110, 14);
    slopeBox.setBounds(540, 56, 130, 26);

    themeLabel.setBounds(700, 40, 110, 14);
    themeBox.setBounds(700, 56, 150, 26);
}
