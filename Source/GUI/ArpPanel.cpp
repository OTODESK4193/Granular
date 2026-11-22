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

    setupKnob(octavesKnob,     octavesLabel,     "OCTAVES",   "arpOctaves",     arpAccent);
    setupKnob(rateFreeKnob,    rateFreeLabel,    "RATE (Hz)", "arpRateFree",    arpAccent);
    setupKnob(gateKnob,        gateLabel,        "GATE",      "arpGate",        arpAccent);

    // --- EUCLID ---
    addAndMakeVisible(euclidButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "arpEuclidEnable", euclidButton));
    setupKnob(euclidStepsKnob, euclidStepsLabel, "STEPS",     "arpEuclidSteps", GranularColors::sage);
    setupKnob(euclidFillsKnob, euclidFillsLabel, "FILLS",     "arpEuclidFills", GranularColors::sage);
    setupKnob(euclidRotKnob,   euclidRotLabel,   "ROT",       "arpEuclidRot",   GranularColors::sage);

    // --- QUANTIZE (2段構成) ---
    addAndMakeVisible(scaleOnButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "scaleOn", scaleOnButton));

    setupCombo(keyBox, keyLabel, "KEY", "key");

    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    scaleLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    scaleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(scaleLabel);

    scaleSelector.bindTo(proc.apvts, "scale");
    addAndMakeVisible(scaleSelector);

    // --- TEXTURE ---
    setupCombo(windowBox, windowLabel, "WINDOW", "windowShape");
    setupKnob(toneKnob, toneLabel, "TONE", "tone", grain);
    setupKnob(spraySpeedKnob, spraySpeedLabel, "SPRY SPD", "spraySpeed", grain);

    addAndMakeVisible(pitchSyncButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "pitchSync", pitchSyncButton));

    addAndMakeVisible(revButton);
    buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "reverseGrain", revButton));

    const int keyIdx   = (int)proc.apvts.getRawParameterValue("key")->load();
    const int rootNote = (keyIdx == 0) ? proc.currentDetectedRoot.load() : (keyIdx - 1);
    lastScaleKey       = juce::jlimit(0, 11, rootNote);
    lastScaleType      = juce::jlimit(0, ScaleQuantizer::numScales() - 1,
                                 (int)proc.apvts.getRawParameterValue("scale")->load());
    scaleNotesText = ScaleQuantizer::getNoteNames(lastScaleKey, lastScaleType);

    startTimerHz(30);
}

ArpPanel::~ArpPanel() { stopTimer(); }

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

    juce::Component::SafePointer<ArpPanel> safeThis(this);
    GranularUI::attachKnobModMenu(s, proc.apvts, paramID, text,
        [safeThis](int subTab, int page)
        {
            if (safeThis != nullptr && safeThis->onOpenModTab)
                safeThis->onOpenModTab(subTab, page);
        });
}

void ArpPanel::setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& text,
                          const juce::String& paramID)
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(paramID)))
        c.addItemList(choiceParam->getAllValueStrings(), 1);

    addAndMakeVisible(c);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, paramID, c));

    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centredLeft);
    l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    l.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(l);
}

void ArpPanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
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

void ArpPanel::timerCallback()
{
    if (!isVisible()) return;
    using D = ModMatrix;
    applyModRing(octavesKnob,    D::DstArpOct,      LinearAdd, 3.0f);
    applyModRing(rateFreeKnob,   D::DstArpRateFree, ExpOct,    2.0f);
    applyModRing(gateKnob,       D::DstArpGate,     LinearAdd, 1.0f);
    applyModRing(toneKnob,       D::DstTone,        LinearAdd, 1.0f);
    applyModRing(spraySpeedKnob, D::DstSpraySpeed,  LinearAdd, 0.99f);

    // Sync ON時も Rate(Sync) に DstArpRateFree 変調が掛かるためラベル色で可視化
    const bool arpSyncOn = proc.apvts.getRawParameterValue("arpSync")->load() > 0.5f;
    const bool rateModActive = std::abs(proc.modMatrix.getRangeMax(D::DstArpRateFree)
                                        - proc.modMatrix.getRangeMin(D::DstArpRateFree)) > 1.0e-4f;
    rateLabel.setColour(juce::Label::textColourId,
                        (arpSyncOn && rateModActive) ? GranularColors::accentEnv : GranularColors::textDim);

    // 選択中 Key/Scale の構成音表示更新
    const int keyIdx   = (int)proc.apvts.getRawParameterValue("key")->load();
    const int rootNote = (keyIdx == 0) ? proc.currentDetectedRoot.load() : (keyIdx - 1);
    const int safeRoot = juce::jlimit(0, 11, rootNote);
    const int type     = juce::jlimit(0, ScaleQuantizer::numScales() - 1,
                                  (int)proc.apvts.getRawParameterValue("scale")->load());
    if (safeRoot != lastScaleKey || type != lastScaleType)
    {
        lastScaleKey  = safeRoot;
        lastScaleType = type;
        scaleNotesText = ScaleQuantizer::getNoteNames(safeRoot, type);
        repaint(20, 248, 240, 30);
    }
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

    drawSection("ARPEGGIATOR", 20,  6,   712, GranularColors::accentEnv);
    drawSection("EUCLID",      750, 6,   310, GranularColors::sage);
    drawSection("QUANTIZE",    20,  150, 240, GranularColors::accentPitch);
    drawSection("TEXTURE",     300, 150, 360, GranularColors::accentGrain);

    // --- QUANTIZE 3段目: 選択中スケールの構成音 (Scale Notes) ---
    const int notesX = 20;
    const int notesY = 250;
    const int notesW = 240;
    const int notesH = 24;

    g.setColour(GranularColors::panel.withAlpha(0.65f));
    g.fillRoundedRectangle((float)notesX, (float)notesY, (float)notesW, (float)notesH, 3.0f);
    g.setColour(GranularColors::panelLine.withAlpha(0.5f));
    g.drawRoundedRectangle((float)notesX, (float)notesY, (float)notesW, (float)notesH, 3.0f, 1.0f);

    g.setColour(GranularColors::textDim.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawText("NOTES", notesX + 7, notesY, 40, notesH, juce::Justification::centredLeft);

    g.setColour(GranularColors::accentPitch);
    g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    g.drawText(scaleNotesText, notesX + 50, notesY, notesW - 55, notesH, juce::Justification::centredLeft);

    // 補足説明（TEXTURE / SyncSpeedノブに被らないよう右寄せ・4行）
    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("KEY / SCALE snaps both ARP notes and grain pitches.",
               700, 166, 370, 14, juce::Justification::centredLeft);
    g.drawText("P-SYNC: grain firing locked to note pitch.",
               700, 182, 370, 14, juce::Justification::centredLeft);
    g.drawText("SPRY SPD: smoothness of grain scatter.",
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

    // --- Row 1: ARP + EUCLID ---
    const int labelY = 34;
    const int ctrlY = 50;

    enableButton.setBounds(20, ctrlY, 72, 26);
    latchButton.setBounds(20, ctrlY + 32, 72, 26);

    patternLabel.setBounds(104, labelY, 100, 14);
    patternBox.setBounds(104, ctrlY, 144, 26);

    placeKnob(octavesKnob, octavesLabel, 260, labelY);

    syncButton.setBounds(334, ctrlY, 60, 26);
    rateLabel.setBounds(404, labelY, 76, 14);
    rateBox.setBounds(404, ctrlY, 76, 26);
    placeKnob(rateFreeKnob, rateFreeLabel, 490, labelY);

    durLabel.setBounds(566, labelY, 82, 14);
    durBox.setBounds(566, ctrlY, 82, 26);
    placeKnob(gateKnob, gateLabel, 660, labelY);

    euclidButton.setBounds(750, ctrlY + 8, 66, 28);
    placeKnob(euclidStepsKnob, euclidStepsLabel, 826, labelY);
    placeKnob(euclidFillsKnob, euclidFillsLabel, 904, labelY);
    placeKnob(euclidRotKnob,   euclidRotLabel,   982, labelY);

    // --- Row 2: QUANTIZE (3段構成: 1段目=ON/KEY, 2段目=SCALE, 3段目=NOTES) + TEXTURE ---
    // 1段目: ON (幅64) + KEY (X=92, 幅168)
    scaleOnButton.setBounds(20, 178, 64, 24);
    keyLabel.setBounds(92, 164, 50, 14);
    keyBox.setBounds(92, 178, 168, 24);

    // 2段目: SCALE ツリーセレクター (幅240)
    scaleLabel.setBounds(20, 206, 50, 14);
    scaleSelector.setBounds(20, 220, 240, 24);

    // 3段目: NOTES (paint 内で X=20, Y=250, W=240, H=24 に描画)

    const int subLabelY = 172;
    const int comboY    = 188;

    windowLabel.setBounds(300, subLabelY, 80, 14);
    windowBox.setBounds(300, comboY, 110, 26);
    placeKnob(toneKnob, toneLabel, 424, subLabelY);
    placeKnob(spraySpeedKnob, spraySpeedLabel, 510, subLabelY);
    pitchSyncButton.setBounds(596, subLabelY + 8, 72, 24);
    revButton.setBounds(596, subLabelY + 38, 72, 24);
}
