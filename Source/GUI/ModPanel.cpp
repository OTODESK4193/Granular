// ==========================================
// File: ModPanel.cpp
// ==========================================
#include "ModPanel.h"

namespace
{
    constexpr int kRowH = 28;       // マトリクス行の高さ
    constexpr int kMatrixX = 410;   // マトリクス開始X
}

ModPanel::ModPanel(GranularAudioProcessor& p) : proc(p)
{
    const auto lav = GranularColors::accentPitch;
    const auto peach = GranularColors::accentEnv;
    const auto pink = GranularColors::accentMaster;

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

    // --- ENV ×2 ---
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i + 1);
        setupKnob(envAttackKnob[(size_t)i], "env" + idx + "Attack", peach);
        setupKnob(envDecayKnob[(size_t)i], "env" + idx + "Decay", peach);

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

    drawSection("SOURCES", 20, 6, 370, GranularColors::accentPitch);
    drawSection("MATRIX  (Source x Amount -> Destination, double-click = 0)",
                kMatrixX, 6, 1060 - kMatrixX, GranularColors::accentGrain);

    g.setColour(GranularColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));

    // LFO行ラベル
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
        g.drawText("LFO" + juce::String(i + 1), 20, 30 + i * 34, 38, 24, juce::Justification::centredLeft);

    // ENV行ラベル + A/D
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const int y = 172 + i * 40;
        g.drawText("ENV" + juce::String(i + 1), 20, y, 38, 30, juce::Justification::centredLeft);
        g.drawText("A", 64, y + 30, 30, 10, juce::Justification::centred);
        g.drawText("D", 102, y + 30, 30, 10, juce::Justification::centred);
    }

    // MACROラベル
    g.drawText("MACRO", 20, 254, 44, 30, juce::Justification::centredLeft);
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        g.drawText(juce::String(i + 1), 70 + i * 42, 284, 30, 10, juce::Justification::centred);
}

void ModPanel::resized()
{
    // --- LFO行: [wave 84][SYNC 50][rate 30][syncRate 72] ---
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const int y = 30 + i * 34;
        lfoWaveBox[(size_t)i].setBounds(60, y, 84, 24);
        lfoSyncButton[(size_t)i]->setBounds(150, y, 52, 24);
        lfoRateKnob[(size_t)i].setBounds(208, y - 3, 30, 30);
        lfoSyncRateBox[(size_t)i].setBounds(246, y, 72, 24);
    }

    // --- ENV行: [A 30][D 30][LOOP 54] ---
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const int y = 172 + i * 40;
        envAttackKnob[(size_t)i].setBounds(64, y, 30, 30);
        envDecayKnob[(size_t)i].setBounds(102, y, 30, 30);
        envLoopButton[(size_t)i]->setBounds(142, y + 3, 54, 24);
    }

    // --- MACRO ---
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        macroKnob[(size_t)i].setBounds(70 + i * 42, 252, 30, 30);

    // --- マトリクス ---
    matrixViewport.setBounds(kMatrixX, 26, getWidth() - kMatrixX - 16, getHeight() - 32);

    const int contentW = matrixViewport.getWidth() - 10; // スクロールバー分
    matrixContent.setSize(contentW, ModMatrix::kNumSlots * kRowH + 4);

    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const int y = i * kRowH + 2;
        rowLabel[(size_t)i].setBounds(0, y, 22, 24);
        srcBox[(size_t)i].setBounds(24, y, 118, 24);
        dstBox[(size_t)i].setBounds(148, y, 142, 24);
        amtSlider[(size_t)i].setBounds(296, y, contentW - 300, 24);
    }
}
