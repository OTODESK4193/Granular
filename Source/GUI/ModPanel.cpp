// ==========================================
// File: ModPanel.cpp
// ==========================================
#include "ModPanel.h"

namespace
{
    constexpr int kRowH    = 28;    // マトリクス行の高さ
    constexpr int kMatrixX = 410;   // マトリクス開始X
    constexpr int kSrcX    = 16;    // ソース領域の左端
    constexpr int kSrcW    = 380;   // ソース領域の幅
    constexpr int kContentTop = 56; // サブタブ下のコンテンツ開始Y
    constexpr int kRowsPerPage = 8;

    inline int lfoRowY(int i)   { return kContentTop + 8 + i * 56; }
    inline int envBlockY(int i) { return kContentTop + 2 + i * 76; }
    inline int envKnobX(int k)  { return 22 + k * 58; }
    constexpr int kMatrixTop  = 56;  // マトリクス行の開始Y
}

ModPanel::ModPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto lav   = GranularColors::accentPitch;
    const auto peach = GranularColors::accentEnv;

    // --- ソース・サブタブ ---
    for (auto* b : { &lfoTabBtn, &envTabBtn, &page1Btn, &page2Btn })
    {
        b->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*b);
    }
    lfoTabBtn.onClick   = [this] { setSourceTab(0); };
    envTabBtn.onClick   = [this] { setSourceTab(1); };
    page1Btn.onClick    = [this] { setMatrixPage(0); };
    page2Btn.onClick    = [this] { setMatrixPage(1); };

    // --- LFO ×4 ---
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const juce::String idx(i + 1);
        setupCombo(lfoWaveBox[(size_t)i], "lfo" + idx + "Wave", *this);
        setupCombo(lfoSyncRateBox[(size_t)i], "lfo" + idx + "SyncRate", *this);
        setupKnob(lfoRateKnob[(size_t)i], "lfo" + idx + "Rate", lav);

        lfoSyncButton[(size_t)i] = std::make_unique<GlowToggle>("SYNC", GranularColors::babyBlue);
        addAndMakeVisible(*lfoSyncButton[(size_t)i]);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.apvts, "lfo" + idx + "Sync", *lfoSyncButton[(size_t)i]));
    }

    // --- ENV ×3 (ADSR) ---
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i + 1);
        setupKnob(envAttackKnob[(size_t)i],  "env" + idx + "Attack",  peach);
        setupKnob(envDecayKnob[(size_t)i],   "env" + idx + "Decay",   peach);
        setupKnob(envSustainKnob[(size_t)i], "env" + idx + "Sustain", peach);
        setupKnob(envReleaseKnob[(size_t)i], "env" + idx + "Release", peach);

        envLoopButton[(size_t)i] = std::make_unique<GlowToggle>("LOOP", GranularColors::lilac);
        addAndMakeVisible(*envLoopButton[(size_t)i]);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.apvts, "env" + idx + "Loop", *envLoopButton[(size_t)i]));
    }

    // --- マトリクス16行 (直接配置 / ページ切替) ---
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);

        rowLabel[(size_t)i].setText(juce::String(i + 1), juce::dontSendNotification);
        rowLabel[(size_t)i].setJustificationType(juce::Justification::centred);
        rowLabel[(size_t)i].setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        rowLabel[(size_t)i].setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(rowLabel[(size_t)i]);

        setupCombo(srcBox[(size_t)i], "mod" + idx + "Src", *this);

        addAndMakeVisible(dstBox[(size_t)i]);
        dstBox[(size_t)i].bindTo(proc.apvts, "mod" + idx + "Dst");
        dstBox[(size_t)i].buildMenu = [this](int currentDst) { return buildDestMenu(currentDst); };

        // Uni/Bipolar トグル (点灯=Unipolar / 消灯=Bipolar)
        uniButton[(size_t)i] = std::make_unique<GlowToggle>("UNI", GranularColors::babyBlue);
        addAndMakeVisible(*uniButton[(size_t)i]);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.apvts, "mod" + idx + "Uni", *uniButton[(size_t)i]));

        auto& amt = amtSlider[(size_t)i];
        amt.setSliderStyle(juce::Slider::LinearHorizontal);
        amt.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amt.setColour(juce::Slider::trackColourId, GranularColors::lavender.withAlpha(0.55f));
        amt.setColour(juce::Slider::backgroundColourId, GranularColors::knobTrack);
        amt.setColour(juce::Slider::thumbColourId, GranularColors::text);
        amt.setPopupDisplayEnabled(true, true, this);
        amt.setDoubleClickReturnValue(true, 0.0);
        addAndMakeVisible(amt);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, "mod" + idx + "Amt", amt));
    }

    setSourceTab(0);
    setMatrixPage(0);

    startTimerHz(30);
}

ModPanel::~ModPanel() { stopTimer(); }

void ModPanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
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

void ModPanel::timerCallback()
{
    using D = ModMatrix;
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        applyModRing(lfoRateKnob[(size_t)i], D::DstLfo1Rate + i, LinearAdd, 10.0f);

    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        applyModRing(envAttackKnob[(size_t)i],  D::DstEnv1Attack + i * 4,  LinearAdd, 2.5f);
        applyModRing(envDecayKnob[(size_t)i],   D::DstEnv1Decay + i * 4,   LinearAdd, 2.5f);
        applyModRing(envSustainKnob[(size_t)i], D::DstEnv1Sustain + i * 4, LinearAdd, 1.0f);
        applyModRing(envReleaseKnob[(size_t)i], D::DstEnv1Release + i * 4, LinearAdd, 2.5f);
    }
}

juce::PopupMenu ModPanel::buildDestMenu(int currentDst)
{
    juce::PopupMenu menu;
    auto addItem = [&menu, currentDst](juce::PopupMenu& targetMenu, int id)
    {
        const juce::String name = ModMatrix::getDestNames()[id];
        targetMenu.addItem(id + 1, name, true, currentDst == id);
    };

    addItem(menu, (int)ModMatrix::DstNone);
    menu.addSeparator();

    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstPosition; d <= (int)ModMatrix::DstTone; ++d) addItem(sub, d);
        menu.addSubMenu("Grain / Texture", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstOctave; d <= (int)ModMatrix::DstFine; ++d) addItem(sub, d);
        menu.addSubMenu("Pitch", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstMasterHpf; d <= (int)ModMatrix::DstMasterLpf; ++d) addItem(sub, d);
        menu.addSubMenu("Filter", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstSampleStart; d <= (int)ModMatrix::DstSampleEnd; ++d) addItem(sub, d);
        menu.addSubMenu("Sample", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstArpOct; d <= (int)ModMatrix::DstArpGate; ++d) addItem(sub, d);
        menu.addSubMenu("ARP", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstOutGain; d <= (int)ModMatrix::DstLimRelease; ++d) addItem(sub, d);
        menu.addSubMenu("Master", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstFx1Amount; d <= (int)ModMatrix::DstFx5Amount; ++d) addItem(sub, d);
        menu.addSubMenu("FX General", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstSatDrive; d <= (int)ModMatrix::DstSatTrim; ++d) addItem(sub, d);
        menu.addSubMenu("Saturation", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstChoRate; d <= (int)ModMatrix::DstChoWidth; ++d) addItem(sub, d);
        menu.addSubMenu("Chorus", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstDlyFeedback; d <= (int)ModMatrix::DstDlyDamp; ++d) addItem(sub, d);
        menu.addSubMenu("Delay", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstFrzSize; d <= (int)ModMatrix::DstFrzDamp; ++d) addItem(sub, d);
        menu.addSubMenu("Freeze", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstRevDecay; d <= (int)ModMatrix::DstRevMod; ++d) addItem(sub, d);
        menu.addSubMenu("Reverb", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstLfo1Rate; d <= (int)ModMatrix::DstLfo4Rate; ++d) addItem(sub, d);
        menu.addSubMenu("LFO", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstEnv1Attack; d <= (int)ModMatrix::DstEnv3Release; ++d) addItem(sub, d);
        menu.addSubMenu("Envelope", sub);
    }
    {
        juce::PopupMenu sub;
        for (int d = (int)ModMatrix::DstGateDepth; d <= (int)ModMatrix::DstGateVowel; ++d) addItem(sub, d);
        menu.addSubMenu("Gate", sub);
    }

    return menu;
}

void ModPanel::setupKnob(ValueKnob& s, const juce::String& paramID, juce::Colour accent)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setColour(juce::Slider::rotarySliderFillColourId, accent);
    s.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(s);
    sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, paramID, s));
}

void ModPanel::setupCombo(juce::ComboBox& c, const juce::String& paramID, juce::Component& parent)
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(paramID)))
        c.addItemList(choiceParam->choices, 1);
    parent.addAndMakeVisible(c);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, paramID, c));
}

void ModPanel::styleTab(juce::TextButton& b, bool active)
{
    b.setColour(juce::TextButton::textColourOffId,
                active ? GranularColors::text : GranularColors::textDim);
}

void ModPanel::setSourceTab(int t)
{
    activeSrcTab = juce::jlimit(0, 1, t);
    const bool L = (activeSrcTab == 0), E = (activeSrcTab == 1);

    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        lfoWaveBox[(size_t)i].setVisible(L);
        lfoSyncButton[(size_t)i]->setVisible(L);
        lfoRateKnob[(size_t)i].setVisible(L);
        lfoSyncRateBox[(size_t)i].setVisible(L);
    }
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        envAttackKnob[(size_t)i].setVisible(E);
        envDecayKnob[(size_t)i].setVisible(E);
        envSustainKnob[(size_t)i].setVisible(E);
        envReleaseKnob[(size_t)i].setVisible(E);
        envLoopButton[(size_t)i]->setVisible(E);
    }
    styleTab(lfoTabBtn, L);
    styleTab(envTabBtn, E);
    repaint();
}

void ModPanel::setMatrixPage(int pg)
{
    matrixPage = pg;
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const bool vis = (i / kRowsPerPage) == pg;
        rowLabel[(size_t)i].setVisible(vis);
        srcBox[(size_t)i].setVisible(vis);
        dstBox[(size_t)i].setVisible(vis);
        uniButton[(size_t)i]->setVisible(vis);
        amtSlider[(size_t)i].setVisible(vis);
    }
    styleTab(page1Btn, pg == 0);
    styleTab(page2Btn, pg == 1);
    repaint();
}

void ModPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("SOURCES", 20, 6, kSrcW, GranularColors::accentPitch);
    drawSection("MATRIX  (Source x Amount -> Destination, dbl-click=0, UNI=unipolar)",
                kMatrixX, 6, getWidth() - kMatrixX - 16, GranularColors::accentGrain);

    auto underline = [&g](const juce::TextButton& b, bool active, juce::Colour c)
    {
        if (!active) return;
        const auto r = b.getBounds();
        g.setColour(c);
        g.fillRoundedRectangle((float)r.getX() + 6.0f, (float)r.getBottom() - 1.0f,
                               (float)r.getWidth() - 12.0f, 2.0f, 1.0f);
    };
    underline(lfoTabBtn,   activeSrcTab == 0, GranularColors::lavender);
    underline(envTabBtn,   activeSrcTab == 1, GranularColors::lavender);
    underline(page1Btn,    matrixPage == 0,   GranularColors::mint);
    underline(page2Btn,    matrixPage == 1,   GranularColors::mint);

    g.setColour(GranularColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));

    if (activeSrcTab == 0)  // LFO
    {
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
            g.drawText("LFO" + juce::String(i + 1), kSrcX + 4, lfoRowY(i) + 4, 42, 24,
                       juce::Justification::centredLeft);
    }
    else  // ENV (ADSR) ×3
    {
        static const char* adsr[4] = { "A", "D", "S", "R" };
        for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
        {
            const int yb = envBlockY(i);
            g.setColour(GranularColors::accentEnv);
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText("ENV" + juce::String(i + 1), 20, yb, 80, 14, juce::Justification::centredLeft);

            g.setColour(GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
            for (int k = 0; k < 4; ++k)
                g.drawText(adsr[k], envKnobX(k), yb + 16 + 44, 44, 11, juce::Justification::centred);
        }
    }
}

void ModPanel::resized()
{
    // --- ソース・サブタブ ---
    lfoTabBtn.setBounds(20, 26, 72, 24);
    envTabBtn.setBounds(96, 26, 72, 24);

    // --- マトリクス・ページタブ ---
    page1Btn.setBounds(kMatrixX, 28, 72, 22);
    page2Btn.setBounds(kMatrixX + 78, 28, 72, 22);

    // --- LFO行 ---
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const int y = lfoRowY(i);
        lfoWaveBox[(size_t)i].setBounds(64, y, 96, 26);
        lfoSyncButton[(size_t)i]->setBounds(168, y, 58, 26);
        lfoRateKnob[(size_t)i].setBounds(236, y - 9, 44, 44);
        lfoSyncRateBox[(size_t)i].setBounds(288, y, 100, 26);
    }

    // --- ENV: A/D/S/R ノブ(44) + LOOP ---
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const int yb = envBlockY(i);
        const int ky = yb + 16;
        envAttackKnob[(size_t)i].setBounds(envKnobX(0), ky, 44, 44);
        envDecayKnob[(size_t)i].setBounds(envKnobX(1), ky, 44, 44);
        envSustainKnob[(size_t)i].setBounds(envKnobX(2), ky, 44, 44);
        envReleaseKnob[(size_t)i].setBounds(envKnobX(3), ky, 44, 44);
        envLoopButton[(size_t)i]->setBounds(envKnobX(3) + 52, ky + 9, 70, 26);
    }

    // --- マトリクス16行 (ページごとに同じ8スロット位置へ配置) ---
    const int amtX = kMatrixX + 308;
    const int amtW = getWidth() - 16 - amtX;
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const int row = i % kRowsPerPage;
        const int y = kMatrixTop + row * kRowH;
        rowLabel[(size_t)i].setBounds(kMatrixX, y, 20, 24);
        srcBox[(size_t)i].setBounds(kMatrixX + 24, y, 110, 24);
        dstBox[(size_t)i].setBounds(kMatrixX + 138, y, 116, 24);
        uniButton[(size_t)i]->setBounds(kMatrixX + 258, y, 46, 24);
        amtSlider[(size_t)i].setBounds(amtX, y, amtW, 24);
    }
}
