// ==========================================
// File: ArpPanel.cpp
// ==========================================
#include "ArpPanel.h"

namespace
{
    constexpr int kKnobW = 78;
    constexpr int kKnobH = 74;
    constexpr int kLabelH = 16;
}

ArpPanel::ArpPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto accent = GranularColors::accentEnv; // peach系

    // トグル
    addAndMakeVisible(enableButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpEnable", enableButton));

    addAndMakeVisible(latchButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpLatch", latchButton));

    addAndMakeVisible(syncButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpSync", syncButton));

    // コンボ
    setupCombo(patternBox, patternLabel, "PATTERN",  "arpPattern");
    setupCombo(rateBox,    rateLabel,    "RATE",     "arpRateSync");
    setupCombo(durBox,     durLabel,     "DURATION", "arpDurMode");

    // ノブ
    setupKnob(octavesKnob,  octavesLabel,  "OCTAVES",   "arpOctaves",  accent);
    setupKnob(rateFreeKnob, rateFreeLabel, "RATE (Hz)", "arpRateFree", accent);
    setupKnob(gateKnob,     gateLabel,     "GATE",      "arpGate",     accent);
}

void ArpPanel::setupKnob(ValueKnob& s, juce::Label& l, const juce::String& text,
                         const juce::String& paramID, juce::Colour accent)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::rotarySliderFillColourId, accent);
    s.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(s);

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    l.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(l);

    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, paramID, s));
}

void ArpPanel::setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& text,
                          const juce::String& paramID)
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(paramID)))
        c.addItemList(choiceParam->choices, 1);

    addAndMakeVisible(c);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, paramID, c));

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centredLeft);
    l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    l.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(l);
}

void ArpPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("ARPEGGIATOR", 20, 6, 1040, GranularColors::accentEnv);

    // 補足説明
    g.setColour(GranularColors::textDim.withAlpha(0.75f));
    g.setFont(juce::Font(juce::FontOptions(11.5f)));
    g.drawText("Output notes are snapped to the QUANTIZE Key / Scale.",
               20, 150, 600, 16, juce::Justification::centredLeft);
    g.drawText("SYNC off = RATE (Hz) knob.  DURATION 'Gate %' = GATE knob.  LATCH holds notes after release.",
               20, 170, 800, 16, juce::Justification::centredLeft);
}

void ArpPanel::resized()
{
    auto placeKnob = [](juce::Slider& s, juce::Label& l, int x, int y)
    {
        s.setBounds(x, y, kKnobW - 8, kKnobH);
        l.setBounds(x - 6, y + kKnobH, kKnobW + 4, kLabelH);
    };

    const int labelY = 34;
    const int ctrlY = 50;

    // ON / LATCH（縦積み）
    enableButton.setBounds(20, ctrlY, 76, 26);
    latchButton.setBounds(20, ctrlY + 32, 76, 26);

    // PATTERN
    patternLabel.setBounds(120, labelY, 100, 14);
    patternBox.setBounds(120, ctrlY, 160, 26);

    // OCTAVES
    placeKnob(octavesKnob, octavesLabel, 306, labelY);

    // SYNC + RATE(コンボ) + RATE(Hz)
    syncButton.setBounds(396, ctrlY, 70, 26);
    rateLabel.setBounds(482, labelY, 80, 14);
    rateBox.setBounds(482, ctrlY, 90, 26);
    placeKnob(rateFreeKnob, rateFreeLabel, 592, labelY);

    // DURATION + GATE
    durLabel.setBounds(686, labelY, 90, 14);
    durBox.setBounds(686, ctrlY, 100, 26);
    placeKnob(gateKnob, gateLabel, 806, labelY);
}
