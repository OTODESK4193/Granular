// ==========================================
// File: ConfigPanel.cpp
// ==========================================
#include "ConfigPanel.h"

namespace
{
    constexpr int kKnobW = 78;
    constexpr int kKnobH = 74;
    constexpr int kLabelH = 16;
}

ConfigPanel::ConfigPanel(GranularAudioProcessor& p) : proc(p)
{
    auto setupKnob = [this](ValueKnob& k, juce::Label& l, const juce::String& text, const juce::String& id)
    {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k.setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentMaster);
        k.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(k);
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, id, k));

        juce::Component::SafePointer<ConfigPanel> safeThis(this);
        GranularUI::attachKnobModMenu(k, proc.apvts, id, text,
            [safeThis](int subTab, int page)
            {
                if (safeThis != nullptr && safeThis->onOpenModTab)
                    safeThis->onOpenModTab(subTab, page);
            });

        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        l.setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(l);
    };
    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& text, const juce::String& id)
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

    // ANALYSIS: MATERIAL
    setupCombo(materialBox, materialLabel, "MATERIAL", "materialMode");

    // LIMITER: Release (天井CeilingはMainタブへ移設)
    setupKnob(limReleaseKnob, limReleaseLabel, "RELEASE", "limRelease");

    // SYSTEM: Polyphony / Filter Slope / Color Theme
    setupKnob(polyKnob, polyLabel, "POLY", "poly");
    setupCombo(slopeBox, slopeLabel, "FILTER SLOPE", "filterSlope");
    setupCombo(themeBox, themeLabel, "COLOR THEME", "colorTheme");

    // GUI スケーリング
    scaleLabel.setText("GUI SCALE", juce::dontSendNotification);
    scaleLabel.setJustificationType(juce::Justification::centredLeft);
    scaleLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    scaleLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(scaleLabel);

    const float scaleValues[5] = { 0.75f, 1.00f, 1.25f, 1.50f, 2.00f };
    for (size_t i = 0; i < 5; ++i)
    {
        auto& btn = scaleButtons[i];
        btn.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
        btn.setColour(juce::TextButton::textColourOffId, GranularColors::text);
        const float s = scaleValues[i];
        btn.onClick = [this, s] { if (onScaleRequested) onScaleRequested(s); };
        addAndMakeVisible(btn);
    }

    startTimerHz(30);
}

ConfigPanel::~ConfigPanel() { stopTimer(); }

void ConfigPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("ANALYSIS", 20,  6, 220, GranularColors::accentPitch);
    drawSection("LIMITER",  280, 6, 120, GranularColors::accentMaster);
    drawSection("SYSTEM",   440, 6, 620, GranularColors::accentGrain);

    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Change MATERIAL, then press ANALYZE (header) to re-analyze the sample.",
               20, 120, 240, 28, juce::Justification::topLeft);
    g.drawText("Limiter RELEASE (ms). Ceiling is on the MAIN tab.",
               280, 120, 150, 40, juce::Justification::topLeft);
    g.drawText("POLY: max simultaneous voices.  FILTER SLOPE: master HPF/LPF steepness (12 or 24 dB/oct).",
               440, 120, 620, 14, juce::Justification::topLeft);

    g.setColour(GranularColors::textDim.withAlpha(0.55f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("COLOR THEME: palette switches live; reopen window for full redraw.",
               440, 138, 620, 18, juce::Justification::topLeft);

    // ------------------------------------------------------------------
    // サードパーティ表記 (ANALYSIS エリア直下)
    //
    // Signalsmith Stretch / Signalsmith Linear は MIT ライセンス。
    // MIT は「著作権表示と許諾表示を配布物に含めること」を要求するため、
    // ここでの画面表示に加えて、配布パッケージには LICENSE 全文を必ず同梱すること。
    //   Source/third_party/signalsmith-stretch/LICENSE-stretch.txt
    //   Source/third_party/signalsmith-stretch/signalsmith-linear/LICENSE-linear.txt
    // ------------------------------------------------------------------
    {
        const int x = 20;
        const int w = 260;
        int y = 168;

        g.setColour(juce::Colours::white);

        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText("Time-stretching powered by Signalsmith Stretch",
                   x, y, w + 60, 13, juce::Justification::left);
        y += 14;

        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawText(juce::CharPointer_UTF8("Copyright \xc2\xa9 2022 Geraint Luff / Signalsmith Audio Ltd."),
                   x, y, w + 60, 12, juce::Justification::left);
        y += 12;

        g.drawText(juce::CharPointer_UTF8("Signalsmith Linear \xc2\xa9 2025 Signalsmith Audio"),
                   x, y, w + 60, 12, juce::Justification::left);
        y += 12;

        g.drawText("Used under the MIT License. See LICENSE files included with this product.",
                   x, y, w + 120, 12, juce::Justification::left);
    }
}

void ConfigPanel::resized()
{
    // ANALYSIS
    materialLabel.setBounds(20, 34, 100, 14);
    materialBox.setBounds(20, 50, 160, 26);

    // LIMITER: Release ノブ
    limReleaseKnob.setBounds(280, 34, kKnobW - 8, kKnobH);
    limReleaseLabel.setBounds(280, 34 + kKnobH, kKnobW - 8, kLabelH);

    // SYSTEM: Poly ノブ + Filter Slope コンボ + Color Theme + GUI SCALE
    polyKnob.setBounds(430, 34, kKnobW - 8, kKnobH);
    polyLabel.setBounds(430, 34 + kKnobH, kKnobW - 8, kLabelH);

    slopeLabel.setBounds(515, 40, 100, 14);
    slopeBox.setBounds(515, 56, 105, 26);

    themeLabel.setBounds(635, 40, 110, 14);
    themeBox.setBounds(635, 56, 135, 26);

    scaleLabel.setBounds(785, 40, 180, 14);
    const int btnW = 48;
    const int btnH = 26;
    const int gap  = 6;
    for (size_t i = 0; i < 5; ++i)
        scaleButtons[i].setBounds(785 + (int)i * (btnW + gap), 56, btnW, btnH);
}

void ConfigPanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
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

void ConfigPanel::timerCallback()
{
    if (!isVisible()) return;
    using D = ModMatrix;
    applyModRing(limReleaseKnob, D::DstLimRelease, LinearAdd, 250.0f);
}
