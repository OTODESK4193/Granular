// ==========================================
// File: MainPanel.cpp
// ==========================================
#include "MainPanel.h"

namespace
{
    constexpr int kKnobW = 78;
    constexpr int kKnobH = 74;
    constexpr int kLabelH = 16;
}

MainPanel::MainPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto grain = GranularColors::accentGrain;
    const auto pitch = GranularColors::accentPitch;
    const auto env = GranularColors::accentEnv;
    const auto master = GranularColors::accentMaster;

    // --- GRAIN ---
    setupKnob(sizeKnob,    sizeLabel,    "SIZE",    "grainSize",     grain);
    setupKnob(densityKnob, densityLabel, "DENSITY", "density",       grain);
    setupKnob(sprayKnob,   sprayLabel,   "SPRAY",   "positionSpray", grain);
    setupKnob(jitterKnob,  jitterLabel,  "JITTER",  "pitchJitter",   grain);
    setupKnob(panKnob,     panLabel,     "PAN",     "panSpread",     grain);
    setupKnob(speedKnob,   speedLabel,   "SPEED",   "speed",         grain);

    // --- PITCH ---
    setupKnob(octaveKnob, octaveLabel, "OCTAVE", "octave",   pitch);
    setupKnob(semiKnob,   semiLabel,   "SEMI",   "pitchSt",  pitch);
    setupKnob(fineKnob,   fineLabel,   "FINE",   "fineTune", pitch);

    // --- ENV ---
    setupKnob(attackKnob,  attackLabel,  "A", "attack",  env);
    setupKnob(decayKnob,   decayLabel,   "D", "decay",   env);
    setupKnob(sustainKnob, sustainLabel, "S", "sustain", env);
    setupKnob(releaseKnob, releaseLabel, "R", "release", env);

    // --- SAMPLE: Position(波形の縦線) / Start / End ---
    setupKnob(positionKnob, positionLabel, "POSITION", "position",    master);
    setupKnob(startKnob,    startLabel,    "START",    "sampleStart", master);
    setupKnob(endKnob,      endLabel,      "END",      "sampleEnd",   master);

    // --- MASTER ---
    setupKnob(hpfKnob,  hpfLabel,  "HPF",  "masterHPF", master);
    setupKnob(lpfKnob,  lpfLabel,  "LPF",  "masterLPF", master);
    setupKnob(gainKnob, gainLabel, "GAIN", "outGain",   master);
}

void MainPanel::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text,
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

void MainPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("GRAIN",    20,  6,   6 * kKnobW - 8, GranularColors::accentGrain);
    drawSection("PITCH",    516, 6,   3 * kKnobW - 8, GranularColors::accentPitch);
    drawSection("ENVELOPE", 772, 6,   4 * kKnobW - 8, GranularColors::accentEnv);
    drawSection("SAMPLE",   20,  150, 3 * kKnobW - 8, GranularColors::accentMaster);
    drawSection("MASTER",   848, 150, 3 * kKnobW - 8, GranularColors::accentMaster);

    // SAMPLEセクションの補足
    g.setColour(GranularColors::textDim.withAlpha(0.6f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("POSITION = pink line / START & END = markers on the waveform",
               280, 196, 540, 14, juce::Justification::centredLeft);
}

void MainPanel::resized()
{
    auto placeKnob = [](juce::Slider& s, juce::Label& l, int x, int y)
    {
        s.setBounds(x, y, kKnobW - 8, kKnobH);
        l.setBounds(x, y + kKnobH, kKnobW - 8, kLabelH);
    };

    // --- Row 1 (y=30) ---
    const int row1 = 30;
    placeKnob(sizeKnob,    sizeLabel,    20 + 0 * kKnobW, row1);
    placeKnob(densityKnob, densityLabel, 20 + 1 * kKnobW, row1);
    placeKnob(sprayKnob,   sprayLabel,   20 + 2 * kKnobW, row1);
    placeKnob(jitterKnob,  jitterLabel,  20 + 3 * kKnobW, row1);
    placeKnob(panKnob,     panLabel,     20 + 4 * kKnobW, row1);
    placeKnob(speedKnob,   speedLabel,   20 + 5 * kKnobW, row1);

    placeKnob(octaveKnob, octaveLabel, 516 + 0 * kKnobW, row1);
    placeKnob(semiKnob,   semiLabel,   516 + 1 * kKnobW, row1);
    placeKnob(fineKnob,   fineLabel,   516 + 2 * kKnobW, row1);

    placeKnob(attackKnob,  attackLabel,  772 + 0 * kKnobW, row1);
    placeKnob(decayKnob,   decayLabel,   772 + 1 * kKnobW, row1);
    placeKnob(sustainKnob, sustainLabel, 772 + 2 * kKnobW, row1);
    placeKnob(releaseKnob, releaseLabel, 772 + 3 * kKnobW, row1);

    // --- Row 2 (y=172) ---
    const int row2 = 172;
    placeKnob(positionKnob, positionLabel, 20 + 0 * kKnobW, row2);
    placeKnob(startKnob,    startLabel,    20 + 1 * kKnobW, row2);
    placeKnob(endKnob,      endLabel,      20 + 2 * kKnobW, row2);

    placeKnob(hpfKnob,  hpfLabel,  848 + 0 * kKnobW, row2);
    placeKnob(lpfKnob,  lpfLabel,  848 + 1 * kKnobW, row2);
    placeKnob(gainKnob, gainLabel, 848 + 2 * kKnobW, row2);
}
