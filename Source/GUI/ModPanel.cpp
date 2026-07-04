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
    inline int macroKnobX(int i){ return 28 + i * 90; }
    constexpr int kMacroKnobY = 96;
    constexpr int kMatrixTop  = 56;  // マトリクス行の開始Y
}

ModPanel::ModPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto lav   = GranularColors::accentPitch;
    const auto peach = GranularColors::accentEnv;
    const auto pink  = GranularColors::accentMaster;

    // --- ソース・サブタブ ---
    for (auto* b : { &lfoTabBtn, &envTabBtn, &macroTabBtn, &page1Btn, &page2Btn })
    {
        b->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*b);
    }
    lfoTabBtn.onClick   = [this] { setSourceTab(0); };
    envTabBtn.onClick   = [this] { setSourceTab(1); };
    macroTabBtn.onClick = [this] { setSourceTab(2); };
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

    // --- MACRO ×4 ---
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        setupKnob(macroKnob[(size_t)i], "macro" + juce::String(i + 1), pink);

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
        setupCombo(dstBox[(size_t)i], "mod" + idx + "Dst", *this);

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
    activeSrcTab = t;
    const bool L = (t == 0), E = (t == 1), M = (t == 2);

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
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        macroKnob[(size_t)i].setVisible(M);

    styleTab(lfoTabBtn,   L);
    styleTab(envTabBtn,   E);
    styleTab(macroTabBtn, M);
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
    underline(macroTabBtn, activeSrcTab == 2, GranularColors::lavender);
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
    else if (activeSrcTab == 1)  // ENV (ADSR) ×3
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
    else  // MACRO
    {
        for (int i = 0; i < ModMatrix::kNumMacros; ++i)
            g.drawText("MACRO " + juce::String(i + 1), macroKnobX(i) - 8, kMacroKnobY + 66, 80, 14,
                       juce::Justification::centred);
        g.setColour(GranularColors::textDim.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(10.5f)));
        g.drawText("Tip: assign one Macro to several matrix rows, set +/- amount per destination.",
                   20, kMacroKnobY + 92, kSrcW, 14, juce::Justification::centredLeft);
    }
}

void ModPanel::resized()
{
    // --- ソース・サブタブ ---
    lfoTabBtn.setBounds(20, 26, 72, 24);
    envTabBtn.setBounds(96, 26, 72, 24);
    macroTabBtn.setBounds(172, 26, 84, 24);

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

    // --- MACRO: 大きめノブ×4 ---
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        macroKnob[(size_t)i].setBounds(macroKnobX(i), kMacroKnobY, 64, 64);

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
