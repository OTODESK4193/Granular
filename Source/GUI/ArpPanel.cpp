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
    const auto arpAccent = GranularColors::accentEnv;   // peach系
    const auto grain = GranularColors::accentGrain;

    // --- ARP トグル ---
    addAndMakeVisible(enableButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpEnable", enableButton));

    addAndMakeVisible(latchButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpLatch", latchButton));

    addAndMakeVisible(syncButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpSync", syncButton));

    // --- ARP コンボ / ノブ ---
    setupCombo(patternBox, patternLabel, "PATTERN",  "arpPattern");
    setupCombo(rateBox,    rateLabel,    "RATE",     "arpRateSync");
    setupCombo(durBox,     durLabel,     "DURATION", "arpDurMode");

    setupKnob(octavesKnob,  octavesLabel,  "OCTAVES",   "arpOctaves",  arpAccent);
    setupKnob(rateFreeKnob, rateFreeLabel, "RATE (Hz)", "arpRateFree", arpAccent);
    setupKnob(gateKnob,     gateLabel,     "GATE",      "arpGate",     arpAccent);

    // --- QUANTIZE ---
    setupCombo(keyBox,   keyLabel,   "KEY",   "key");
    setupCombo(scaleBox, scaleLabel, "SCALE", "scale");

    // --- TEXTURE ---
    setupCombo(windowBox, windowLabel, "WINDOW", "windowShape");
    setupKnob(toneKnob, toneLabel, "TONE", "tone", grain);
    setupKnob(syncSpeedKnob, syncSpeedLabel, "SYNC SPD", "syncSpeed", grain);

    addAndMakeVisible(pitchSyncButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "pitchSync", pitchSyncButton));

    addAndMakeVisible(revButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "reverseGrain", revButton));
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

    drawSection("ARPEGGIATOR", 20,  6,   860, GranularColors::accentEnv);
    drawSection("QUANTIZE",    20,  150, 240, GranularColors::accentPitch);
    drawSection("TEXTURE",     300, 150, 360, GranularColors::accentGrain);

    // 補足説明（TEXTURE / SyncSpeedノブに被らないよう右寄せ・4行）
    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("KEY / SCALE snaps both ARP notes and grain pitches.",
               700, 166, 370, 14, juce::Justification::centredLeft);
    g.drawText("P-SYNC: grain firing locked to note pitch.",
               700, 182, 370, 14, juce::Justification::centredLeft);
    g.drawText("SYNC SPD: playhead speed while P-SYNC is on.",
               700, 198, 370, 14, juce::Justification::centredLeft);
    g.drawText("TONE: comb resonator.  LATCH holds notes after release.",
               700, 214, 370, 14, juce::Justification::centredLeft);
}

void ArpPanel::resized()
{
    auto placeKnob = [](juce::Slider& s, juce::Label& l, int x, int y)
    {
        s.setBounds(x, y, kKnobW - 8, kKnobH);
        l.setBounds(x - 6, y + kKnobH, kKnobW + 4, kLabelH);
    };

    // --- Row 1: ARP ---
    const int labelY = 34;
    const int ctrlY = 50;

    enableButton.setBounds(20, ctrlY, 76, 26);
    latchButton.setBounds(20, ctrlY + 32, 76, 26);

    patternLabel.setBounds(120, labelY, 100, 14);
    patternBox.setBounds(120, ctrlY, 160, 26);

    placeKnob(octavesKnob, octavesLabel, 306, labelY);

    syncButton.setBounds(396, ctrlY, 70, 26);
    rateLabel.setBounds(482, labelY, 80, 14);
    rateBox.setBounds(482, ctrlY, 90, 26);
    placeKnob(rateFreeKnob, rateFreeLabel, 592, labelY);

    durLabel.setBounds(686, labelY, 90, 14);
    durBox.setBounds(686, ctrlY, 100, 26);
    placeKnob(gateKnob, gateLabel, 806, labelY);

    // --- Row 2: QUANTIZE + TEXTURE ---
    const int subLabelY = 172;
    const int comboY = 188;

    keyLabel.setBounds(20, subLabelY, 60, 14);
    keyBox.setBounds(20, comboY, 80, 26);
    scaleLabel.setBounds(110, subLabelY, 80, 14);
    scaleBox.setBounds(110, comboY, 150, 26);

    windowLabel.setBounds(300, subLabelY, 80, 14);
    windowBox.setBounds(300, comboY, 110, 26);
    placeKnob(toneKnob, toneLabel, 424, subLabelY);
    placeKnob(syncSpeedKnob, syncSpeedLabel, 510, subLabelY);
    pitchSyncButton.setBounds(596, subLabelY + 8, 72, 24);
    revButton.setBounds(596, subLabelY + 38, 72, 24);
}
