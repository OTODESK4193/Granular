// ==========================================
// File: MainPanel.cpp
// ==========================================
#include "MainPanel.h"
#include <cmath>

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

    // --- MOTION: プレイヘッドの動き ---
    setupCombo(modeBox, modeLabel, "MODE", "playMode");
    setupCombo(scanBox, scanLabel, "SCAN", "speedSyncDiv");

    addAndMakeVisible(speedSyncButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "speedSync", speedSyncButton));

    // --- MASTER ---
    setupKnob(hpfKnob,  hpfLabel,  "HPF",  "masterHPF", master);
    setupKnob(lpfKnob,  lpfLabel,  "LPF",  "masterLPF", master);
    setupKnob(gainKnob, gainLabel, "GAIN", "outGain",   master);
    setupKnob(mixKnob,  mixLabel,  "DRY/WET", "fxMix",  master);

    // --- LIMITER (Configから移設) ---
    setupKnob(ceilingKnob, ceilingLabel, "CEILING", "ceiling", master);

    startTimerHz(30); // MOD Arc 更新
}

MainPanel::~MainPanel() { stopTimer(); }

// ------------------------------------------
// MOD Arc: 変調される宛先ノブに、変調レンジ帯とライブ位置を反映
// ------------------------------------------
void MainPanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
{
    const float rMin = proc.modMatrix.getRangeMin(dst);
    const float rMax = proc.modMatrix.getRangeMax(dst);
    const float live = proc.modMatrix.get(dst);

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

    // 変化があった時だけ再描画
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

void MainPanel::timerCallback()
{
    using D = ModMatrix;
    // 宛先ノブ ← ModMatrix Dst (スケールは PluginProcessor の変調適用式に一致)
    applyModRing(positionKnob, D::DstPosition,  LinearAdd, 1.0f);
    applyModRing(sizeKnob,     D::DstSize,      LinearAdd, 0.48f);
    applyModRing(densityKnob,  D::DstDensity,   LinearAdd, 1.0f);
    applyModRing(sprayKnob,    D::DstSpray,     LinearAdd, 1.0f);
    applyModRing(jitterKnob,   D::DstJitter,    LinearAdd, 2400.0f);
    applyModRing(panKnob,      D::DstPanSpread, LinearAdd, 1.0f);
    applyModRing(speedKnob,    D::DstSpeed,     LinearAdd, 2.0f);
    applyModRing(octaveKnob,   D::DstOctave,     LinearAdd, 2.0f);
    applyModRing(semiKnob,     D::DstSemi,       LinearAdd, 12.0f);
    applyModRing(fineKnob,     D::DstFine,       LinearAdd, 100.0f);
    applyModRing(hpfKnob,      D::DstMasterHpf,  ExpOct,    5.0f);
    applyModRing(lpfKnob,      D::DstMasterLpf,  ExpOct,    6.0f);
    applyModRing(startKnob,    D::DstSampleStart,LinearAdd, 1.0f);
    applyModRing(endKnob,      D::DstSampleEnd,  LinearAdd, 1.0f);
    applyModRing(mixKnob,      D::DstFxMix,      LinearAdd, 1.0f);
    applyModRing(gainKnob,     D::DstOutGain,    LinearAdd, 24.0f);
    applyModRing(ceilingKnob,  D::DstLimCeiling, LinearAdd, 6.0f);
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

void MainPanel::setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& text,
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
    drawSection("MOTION",   256, 150, 250,            GranularColors::accentGrain);
    drawSection("MASTER",   516, 150, 4 * kKnobW - 8, GranularColors::accentMaster);
    drawSection("LIMITER",  844, 150, kKnobW - 8,     GranularColors::accentMaster);

    // MOTION 補足説明（Mode/SCANの下に配置）
    g.setColour(GranularColors::textDim.withAlpha(0.6f));
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText("S-SYNC: scan Start-End over SCAN length (DAW tempo).",
               256, 236, 250, 13, juce::Justification::topLeft);
    g.drawText("Random: SPEED / SCAN set the jump rate.",
               256, 249, 250, 13, juce::Justification::topLeft);
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

    // MOTION (x256..506): MODE / SCAN 上段、S-SYNC 中段、説明文は下段(paint)
    modeLabel.setBounds(256, 168, 80, 14);
    modeBox.setBounds(256, 184, 110, 24);
    scanLabel.setBounds(376, 168, 80, 14);
    scanBox.setBounds(376, 184, 110, 24);
    speedSyncButton.setBounds(256, 212, 110, 24);

    // MASTER (Motionに続けて配置)
    placeKnob(hpfKnob,  hpfLabel,  516 + 0 * kKnobW, row2);
    placeKnob(lpfKnob,  lpfLabel,  516 + 1 * kKnobW, row2);
    placeKnob(gainKnob, gainLabel, 516 + 2 * kKnobW, row2);
    placeKnob(mixKnob,  mixLabel,  516 + 3 * kKnobW, row2);

    // LIMITER (Ceiling, Configから移設)
    placeKnob(ceilingKnob, ceilingLabel, 844, row2);
}
