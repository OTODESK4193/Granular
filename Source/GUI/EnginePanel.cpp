// ==========================================
// File: EnginePanel.cpp
// ==========================================
#include "EnginePanel.h"

namespace
{
    constexpr int kKnobW  = 78;
    constexpr int kKnobH  = 74;
    constexpr int kLabelH = 16;
    constexpr int kRow1Y  = 30;
    constexpr int kRow2Y  = 172;
}

EnginePanel::EnginePanel(GranularAudioProcessor& p) : proc(p)
{
    auto setupKnob = [this](ValueKnob& k, juce::Label& l, const juce::String& text,
                            const juce::String& id, juce::Colour accent)
    {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k.setColour(juce::Slider::rotarySliderFillColourId, accent);
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

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text,
                             const juce::String& id)
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)))
            c.addItemList(choiceParam->getAllValueStrings(), 1);
        addAndMakeVisible(c);
        comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            proc.apvts, id, c));

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        l.setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(l);
    };

    // Row 1: ENGINE & SWARM
    setupCombo(engineBox, engineLabel, "ENGINE MODE", "engineMode");
    setupKnob(formantKnob,   formantLabel,   "FORMANT",     "formantShift",   GranularColors::mint);
    setupKnob(swarmKnob,     swarmLabel,     "SWARM MIX",   "swarmMix",       GranularColors::mint);
    setupKnob(pitchLockKnob, pitchLockLabel, "ATLAS PITCH", "atlasPitchLock", GranularColors::mint);

    // Row 1: CONCATENATIVE ATLAS
    setupKnob(timbreScrubKnob, timbreScrubLabel, "TIMBRE",    "timbreScrub", GranularColors::babyBlue);
    setupKnob(timbreAmtKnob,   timbreAmtLabel,   "ATLAS AMT", "timbreAmt",   GranularColors::babyBlue);

    // Row 1: GRAIN SHAPE
    setupKnob(skewKnob,  skewLabel,  "SKEW",  "grainSkew",  GranularColors::peach);
    setupKnob(chirpKnob, chirpLabel, "CHIRP", "grainChirp", GranularColors::peach);

    // Row 2: PER-GRAIN SVF FILTER
    setupCombo(filterModeBox, filterModeLabel, "FILTER MODE", "grainFilterMode");
    setupKnob(filterFreqKnob,   filterFreqLabel,   "CUTOFF",     "grainFilterFreq",   GranularColors::lavender);
    setupKnob(filterResKnob,    filterResLabel,    "RESONANCE",  "grainFilterRes",    GranularColors::lavender);
    setupKnob(filterJitterKnob, filterJitterLabel, "FLT JITTER", "grainFilterJitter", GranularColors::lavender);

    // Row 2: SCALE-TUNED MODAL RESONATOR BANK
    setupCombo(modalModeBox, modalModeLabel, "RATIO MODE", "modalRatioMode");
    setupKnob(modalMixKnob,    modalMixLabel,    "MODAL MIX", "modalMix",    GranularColors::pink);
    setupKnob(modalDecayKnob,  modalDecayLabel,  "DECAY",     "modalDecay",  GranularColors::pink);
    setupKnob(modalDampKnob,   modalDampLabel,   "DAMPING",   "modalDamp",   GranularColors::pink);
    setupKnob(modalInharmKnob, modalInharmLabel, "INHARM",    "modalInharm", GranularColors::pink);

    startTimerHz(30);
}

EnginePanel::~EnginePanel() { stopTimer(); }

void EnginePanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    // Row 1 headers
    drawSection("ENGINE & SWARM",      20,  6, 376, GranularColors::mint);
    drawSection("CONCATENATIVE ATLAS", 416, 6, 164, GranularColors::babyBlue);
    drawSection("GRAIN SHAPE",         602, 6, 164, GranularColors::peach);
    drawSection("SIGNAL NOTES",        788, 6, 272, GranularColors::lavender);

    // Subtle status / signal hints card on top-right
    g.setColour(GranularColors::textDim.withAlpha(0.75f));
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText("Granular / WSOLA / TD-PSOLA: Pitch-synchronous formant shift.",
               788, 34, 272, 24, juce::Justification::topLeft, true);
    g.drawText("Atlas: O(log N) timbre lookup + local F0 lock + ZC snap.",
               788, 58, 272, 24, juce::Justification::topLeft, true);
    g.drawText("ModMatrix runs polyphonic per-voice Env, Vel, Note & Aftertouch.",
               788, 82, 272, 26, juce::Justification::topLeft, true);

    // Row 2 headers
    drawSection("PER-GRAIN SVF FILTER",            20,  148, 376, GranularColors::lavender);
    drawSection("SCALE-TUNED MODAL RESONATOR BANK", 416, 148, 496, GranularColors::pink);
}

void EnginePanel::resized()
{
    auto placeKnob = [](ValueKnob& k, juce::Label& l, int x, int y)
    {
        k.setBounds(x, y, kKnobW, kKnobH);
        l.setBounds(x, y + kKnobH, kKnobW, kLabelH);
    };

    // Row 1: ENGINE & SWARM
    engineLabel.setBounds(20, 38, 108, 14);
    engineBox.setBounds(20, 54, 108, 26);
    placeKnob(formantKnob,   formantLabel,   140, kRow1Y);
    placeKnob(swarmKnob,     swarmLabel,     224, kRow1Y);
    placeKnob(pitchLockKnob, pitchLockLabel, 308, kRow1Y);

    // Row 1: CONCATENATIVE ATLAS
    placeKnob(timbreScrubKnob, timbreScrubLabel, 416, kRow1Y);
    placeKnob(timbreAmtKnob,   timbreAmtLabel,   500, kRow1Y);

    // Row 1: GRAIN SHAPE
    placeKnob(skewKnob,  skewLabel,  602, kRow1Y);
    placeKnob(chirpKnob, chirpLabel, 686, kRow1Y);

    // Row 2: PER-GRAIN SVF
    filterModeLabel.setBounds(20, 180, 104, 14);
    filterModeBox.setBounds(20, 196, 104, 26);
    placeKnob(filterFreqKnob,   filterFreqLabel,   136, kRow2Y);
    placeKnob(filterResKnob,    filterResLabel,    220, kRow2Y);
    placeKnob(filterJitterKnob, filterJitterLabel, 304, kRow2Y);

    // Row 2: MODAL RESONATOR BANK
    modalModeLabel.setBounds(416, 180, 112, 14);
    modalModeBox.setBounds(416, 196, 112, 26);
    placeKnob(modalMixKnob,    modalMixLabel,    540, kRow2Y);
    placeKnob(modalDecayKnob,  modalDecayLabel,  624, kRow2Y);
    placeKnob(modalDampKnob,   modalDampLabel,   708, kRow2Y);
    placeKnob(modalInharmKnob, modalInharmLabel, 792, kRow2Y);
}

void EnginePanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
{
    const float rMin = proc.modMatrix.getRangeMin(dst);
    const float rMax = proc.modMatrix.getRangeMax(dst);
    const float live = proc.modMatrix.getGui(dst);

    auto& props = s.getProperties();
    const bool active = std::abs(rMax - rMin) > 1.0e-4f;

    if (!active)
    {
        if ((bool)props.getWithDefault("mod_active", false))
        {
            props.set("mod_active", false);
            s.repaint();
        }
        return;
    }

    const auto range = s.getNormalisableRange();
    const float base = (float)s.getValue();

    auto toNorm = [&](float modUnit)
    {
        float real = (kind == ExpOct) ? base * std::pow(2.0f, modUnit * scale)
                                      : base + modUnit * scale;
        real = juce::jlimit((float)range.start, (float)range.end, real);
        return (float)range.convertTo0to1(real);
    };

    const float nMin  = toNorm(rMin);
    const float nMax  = toNorm(rMax);
    const float nLive = toNorm(live);

    const float oldMin  = (float)props.getWithDefault("mod_min", -1.0f);
    const float oldMax  = (float)props.getWithDefault("mod_max", -1.0f);
    const float oldLive = (float)props.getWithDefault("mod_live", -1.0f);

    if (!(bool)props.getWithDefault("mod_active", false)
        || std::abs(nMin - oldMin) > 0.002f
        || std::abs(nMax - oldMax) > 0.002f
        || std::abs(nLive - oldLive) > 0.004f)
    {
        props.set("mod_active", true);
        props.set("mod_min", nMin);
        props.set("mod_max", nMax);
        props.set("mod_live", nLive);
        s.repaint();
    }
}

void EnginePanel::timerCallback()
{
    using D = ModMatrix;
    applyModRing(formantKnob,      D::DstFormantShift,      LinearAdd, 12.0f);
    applyModRing(swarmKnob,        D::DstSwarmMix,          LinearAdd, 1.0f);
    applyModRing(pitchLockKnob,    D::DstAtlasPitchLock,    LinearAdd, 1.0f);
    applyModRing(timbreScrubKnob,  D::DstTimbreScrub,       LinearAdd, 1.0f);
    applyModRing(timbreAmtKnob,    D::DstTimbreAmt,         LinearAdd, 1.0f);
    applyModRing(skewKnob,         D::DstGrainSkew,         LinearAdd, 1.0f);
    applyModRing(chirpKnob,        D::DstGrainChirp,        LinearAdd, 12.0f);
    applyModRing(filterFreqKnob,   D::DstGrainFilterFreq,   ExpOct,    5.0f);
    applyModRing(filterResKnob,    D::DstGrainFilterRes,    LinearAdd, 5.0f);
    applyModRing(filterJitterKnob, D::DstGrainFilterJitter, LinearAdd, 1.0f);
    applyModRing(modalMixKnob,     D::DstModalMix,          LinearAdd, 1.0f);
    applyModRing(modalDecayKnob,   D::DstModalDecay,        LinearAdd, 2.5f);
    applyModRing(modalDampKnob,    D::DstModalDamp,         LinearAdd, 1.0f);
    applyModRing(modalInharmKnob,  D::DstModalInharm,       LinearAdd, 1.0f);
}
