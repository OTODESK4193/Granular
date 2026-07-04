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

    // LFOタブ: i番目の行のY
    inline int lfoRowY(int i) { return kContentTop + 8 + i * 56; }
    // ENVタブ: i番目のブロックのY
    inline int envBlockY(int i) { return kContentTop + 6 + i * 112; }
    // ENV A/D/S/R ノブのX
    inline int envKnobX(int k) { return 22 + k * 62; }
    // MACROノブのX
    inline int macroKnobX(int i) { return 28 + i * 90; }
    constexpr int kMacroKnobY = 108;
}

ModPanel::ModPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto lav   = GranularColors::accentPitch;
    const auto peach = GranularColors::accentEnv;
    const auto pink  = GranularColors::accentMaster;

    // --- ソース・サブタブ ---
    for (auto* b : { &lfoTabBtn, &envTabBtn, &macroTabBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(*b);
    }
    lfoTabBtn.onClick   = [this] { setSourceTab(0); };
    envTabBtn.onClick   = [this] { setSourceTab(1); };
    macroTabBtn.onClick = [this] { setSourceTab(2); };

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

    // --- ENV ×2 (ADSR) ---
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

    // --- マトリクス16行 ---
    addAndMakeVisible(matrixViewport);
    matrixViewport.setViewedComponent(&matrixContent, false);
    matrixViewport.setScrollBarsShown(true, false);

    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);

        rowLabel[(size_t)i].setText(juce::String(i + 1), juce::dontSendNotification);
        rowLabel[(size_t)i].setJustificationType(juce::Justification::centred);
        rowLabel[(size_t)i].setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        rowLabel[(size_t)i].setColour(juce::Label::textColourId, GranularColors::textDim);
        matrixContent.addAndMakeVisible(rowLabel[(size_t)i]);

        setupCombo(srcBox[(size_t)i], "mod" + idx + "Src", matrixContent);
        setupCombo(dstBox[(size_t)i], "mod" + idx + "Dst", matrixContent);

        // Uni/Bipolar トグル (点灯=Unipolar / 消灯=Bipolar)
        uniButton[(size_t)i] = std::make_unique<GlowToggle>("UNI", GranularColors::babyBlue);
        matrixContent.addAndMakeVisible(*uniButton[(size_t)i]);
        buttonAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            proc.apvts, "mod" + idx + "Uni", *uniButton[(size_t)i]));

        auto& amt = amtSlider[(size_t)i];
        amt.setSliderStyle(juce::Slider::LinearHorizontal);
        amt.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amt.setColour(juce::Slider::trackColourId, GranularColors::lavender.withAlpha(0.55f));
        amt.setColour(juce::Slider::backgroundColourId, GranularColors::knobTrack);
        amt.setColour(juce::Slider::thumbColourId, GranularColors::text);
        amt.setPopupDisplayEnabled(true, true, this);
        amt.setDoubleClickReturnValue(true, 0.0); // ダブルクリックで0に戻す
        matrixContent.addAndMakeVisible(amt);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, "mod" + idx + "Amt", amt));
    }

    setSourceTab(0);
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

void ModPanel::styleSrcTab(juce::TextButton& b, bool active)
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

    styleSrcTab(lfoTabBtn,   L);
    styleSrcTab(envTabBtn,   E);
    styleSrcTab(macroTabBtn, M);
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
                kMatrixX, 6, 1060 - kMatrixX, GranularColors::accentGrain);

    // サブタブのアクティブ下線
    auto underline = [&g](const juce::TextButton& b, bool active)
    {
        if (!active) return;
        const auto r = b.getBounds();
        g.setColour(GranularColors::lavender);
        g.fillRoundedRectangle((float)r.getX() + 6.0f, (float)r.getBottom() - 1.0f,
                               (float)r.getWidth() - 12.0f, 2.0f, 1.0f);
    };
    underline(lfoTabBtn,   activeSrcTab == 0);
    underline(envTabBtn,   activeSrcTab == 1);
    underline(macroTabBtn, activeSrcTab == 2);

    g.setColour(GranularColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));

    if (activeSrcTab == 0)  // LFO
    {
        for (int i = 0; i < ModMatrix::kNumLfos; ++i)
            g.drawText("LFO" + juce::String(i + 1), kSrcX + 4, lfoRowY(i) + 4, 42, 24,
                       juce::Justification::centredLeft);
    }
    else if (activeSrcTab == 1)  // ENV (ADSR)
    {
        static const char* adsr[4] = { "A", "D", "S", "R" };
        for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
        {
            const int yb = envBlockY(i);
            g.setColour(GranularColors::accentEnv);
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            g.drawText("ENV" + juce::String(i + 1), 20, yb, 80, 16, juce::Justification::centredLeft);

            g.setColour(GranularColors::textDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            for (int k = 0; k < 4; ++k)
                g.drawText(adsr[k], envKnobX(k), yb + 18 + 52, 52, 12, juce::Justification::centred);
        }
    }
    else  // MACRO
    {
        for (int i = 0; i < ModMatrix::kNumMacros; ++i)
            g.drawText("MACRO " + juce::String(i + 1), macroKnobX(i) - 6, kMacroKnobY + 66, 76, 14,
                       juce::Justification::centred);
    }
}

void ModPanel::resized()
{
    // --- ソース・サブタブ ---
    lfoTabBtn.setBounds(20, 26, 72, 24);
    envTabBtn.setBounds(96, 26, 72, 24);
    macroTabBtn.setBounds(172, 26, 84, 24);

    // --- LFO行: [wave 96][SYNC 58][rate 44][syncRate 100] ---
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const int y = lfoRowY(i);
        lfoWaveBox[(size_t)i].setBounds(64, y, 96, 26);
        lfoSyncButton[(size_t)i]->setBounds(168, y, 58, 26);
        lfoRateKnob[(size_t)i].setBounds(236, y - 9, 44, 44);
        lfoSyncRateBox[(size_t)i].setBounds(288, y, 100, 26);
    }

    // --- ENV: A/D/S/R ノブ(52) + LOOP ---
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const int yb = envBlockY(i);
        const int ky = yb + 18;
        envAttackKnob[(size_t)i].setBounds(envKnobX(0), ky, 52, 52);
        envDecayKnob[(size_t)i].setBounds(envKnobX(1), ky, 52, 52);
        envSustainKnob[(size_t)i].setBounds(envKnobX(2), ky, 52, 52);
        envReleaseKnob[(size_t)i].setBounds(envKnobX(3), ky, 52, 52);
        envLoopButton[(size_t)i]->setBounds(envKnobX(3) + 62, ky + 14, 78, 26);
    }

    // --- MACRO: 大きめノブ×4 ---
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        macroKnob[(size_t)i].setBounds(macroKnobX(i), kMacroKnobY, 64, 64);

    // --- マトリクス ---
    matrixViewport.setBounds(kMatrixX, 26, getWidth() - kMatrixX - 16, getHeight() - 32);

    const int contentW = matrixViewport.getWidth() - 10; // スクロールバー分
    matrixContent.setSize(contentW, ModMatrix::kNumSlots * kRowH + 4);

    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const int y = i * kRowH + 2;
        rowLabel[(size_t)i].setBounds(0, y, 20, 24);
        srcBox[(size_t)i].setBounds(22, y, 110, 24);
        dstBox[(size_t)i].setBounds(136, y, 104, 24);
        uniButton[(size_t)i]->setBounds(244, y, 46, 24);
        amtSlider[(size_t)i].setBounds(296, y, contentW - 300, 24);
    }
}
