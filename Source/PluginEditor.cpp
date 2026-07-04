// ==========================================
// File: PluginEditor.cpp
// ==========================================
#include "PluginEditor.h"
#include "GUI/ColorPalette.h"

namespace
{
    constexpr int kKnobW = 78;
    constexpr int kKnobH = 74;
    constexpr int kLabelH = 16;
}

GranularAudioProcessorEditor::GranularAudioProcessorEditor(GranularAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p), waveDisplay(p)
{
    setLookAndFeel(&lnf);
    addAndMakeVisible(waveDisplay);

    using GC = juce::Colour;
    const GC grain = GranularColors::accentGrain;
    const GC pitch = GranularColors::accentPitch;
    const GC env = GranularColors::accentEnv;
    const GC master = GranularColors::accentMaster;

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

    // --- MASTER ---
    setupKnob(hpfKnob,  hpfLabel,  "HPF",  "masterHPF", master);
    setupKnob(lpfKnob,  lpfLabel,  "LPF",  "masterLPF", master);
    setupKnob(gainKnob, gainLabel, "GAIN", "outGain",   master);

    // --- コンボボックス ---
    setupCombo(keyBox, "key");
    setupCombo(scaleBox, "scale");
    setupCombo(windowBox, "windowShape");
    setupCombo(materialBox, "materialMode");

    auto setupComboLabel = [this](juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centredLeft);
        l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        l.setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(l);
    };
    setupComboLabel(keyLabel,      "KEY");
    setupComboLabel(scaleLabel,    "SCALE");
    setupComboLabel(windowLabel,   "WINDOW");
    setupComboLabel(materialLabel, "MATERIAL");

    // --- REVトグル ---
    addAndMakeVisible(revButton);
    revAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "reverseGrain", revButton);

    // --- LOADボタン ---
    loadButton.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
    loadButton.setColour(juce::TextButton::textColourOffId, GranularColors::text);
    addAndMakeVisible(loadButton);
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load a sample", juce::File(), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                const auto file = fc.getResult();
                if (file.existsAsFile()) proc.loadFileForIngest(file);
            });
    };

    startTimerHz(10); // HUD更新
    setSize(1080, 640);
}

GranularAudioProcessorEditor::~GranularAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

// ------------------------------------------
// セットアップヘルパー
// ------------------------------------------
void GranularAudioProcessorEditor::setupKnob(juce::Slider& s, juce::Label& l, const juce::String& text,
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

void GranularAudioProcessorEditor::setupCombo(juce::ComboBox& c, const juce::String& paramID)
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(paramID)))
        c.addItemList(choiceParam->choices, 1);

    addAndMakeVisible(c);
    comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, paramID, c));
}

// ------------------------------------------
// HUD 更新
// ------------------------------------------
void GranularAudioProcessorEditor::timerCallback()
{
    juce::String f, t, o, c;
    {
        juce::ScopedLock sl(proc.stateLock);
        f = proc.hudLineFile;
        t = proc.hudLineTask;
        o = proc.hudLineOrig;
        c = proc.hudLineCorr;
    }
    if (f != hudFile || t != hudTask || o != hudOrig || c != hudCorr)
    {
        hudFile = f; hudTask = t; hudOrig = o; hudCorr = c;
        repaint(0, 0, getWidth(), 64);
    }
}

// ------------------------------------------
// 描画
// ------------------------------------------
void GranularAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(GranularColors::bg);

    // --- ヘッダー ---
    g.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    juce::ColourGradient titleGrad(GranularColors::mint, 20.0f, 20.0f,
                                   GranularColors::pink, 320.0f, 40.0f, false);
    titleGrad.addColour(0.5, GranularColors::lavender);
    g.setGradientFill(titleGrad);
    g.drawText("G R A N U L A R", 20, 12, 320, 30, juce::Justification::centredLeft);

    g.setColour(GranularColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("OTODESK  |  24-voice Scale-Quantized Granular", 22, 40, 400, 14, juce::Justification::centredLeft);

    // --- HUD (右上) ---
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.setColour(GranularColors::text.withAlpha(0.85f));
    g.drawText(hudFile, 430, 8,  560, 13, juce::Justification::centredRight);
    g.setColour(GranularColors::textDim);
    g.drawText(hudTask, 430, 22, 560, 13, juce::Justification::centredRight);
    g.drawText(hudOrig, 430, 36, 560, 13, juce::Justification::centredRight);
    g.drawText(hudCorr, 430, 50, 560, 13, juce::Justification::centredRight);

    // --- セクションラベル ---
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("GRAIN",    20,  316, 6 * kKnobW - 8, GranularColors::accentGrain);
    drawSection("PITCH",    516, 316, 3 * kKnobW - 8, GranularColors::accentPitch);
    drawSection("ENVELOPE", 772, 316, 4 * kKnobW - 8, GranularColors::accentEnv);
    drawSection("QUANTIZE", 20,  478, 300,            GranularColors::accentPitch);
    drawSection("TEXTURE",  350, 478, 230,            GranularColors::accentGrain);
    drawSection("SAMPLE",   600, 478, 150,            GranularColors::accentMaster);
    drawSection("MASTER",   772, 478, 3 * kKnobW - 8, GranularColors::accentMaster);
}

// ------------------------------------------
// レイアウト
// ------------------------------------------
void GranularAudioProcessorEditor::resized()
{
    // 波形ディスプレイ
    waveDisplay.setBounds(16, 68, getWidth() - 32, 236);

    // LOADボタン（ヘッダー右端）
    loadButton.setBounds(getWidth() - 76, 40, 60, 22);

    auto placeKnob = [this](juce::Slider& s, juce::Label& l, int x, int y)
    {
        s.setBounds(x, y, kKnobW - 8, kKnobH);
        l.setBounds(x, y + kKnobH, kKnobW - 8, kLabelH);
    };

    // --- Row 1 (y=340) ---
    const int row1 = 340;
    // GRAIN
    placeKnob(sizeKnob,    sizeLabel,    20 + 0 * kKnobW, row1);
    placeKnob(densityKnob, densityLabel, 20 + 1 * kKnobW, row1);
    placeKnob(sprayKnob,   sprayLabel,   20 + 2 * kKnobW, row1);
    placeKnob(jitterKnob,  jitterLabel,  20 + 3 * kKnobW, row1);
    placeKnob(panKnob,     panLabel,     20 + 4 * kKnobW, row1);
    placeKnob(speedKnob,   speedLabel,   20 + 5 * kKnobW, row1);
    // PITCH
    placeKnob(octaveKnob, octaveLabel, 516 + 0 * kKnobW, row1);
    placeKnob(semiKnob,   semiLabel,   516 + 1 * kKnobW, row1);
    placeKnob(fineKnob,   fineLabel,   516 + 2 * kKnobW, row1);
    // ENV
    placeKnob(attackKnob,  attackLabel,  772 + 0 * kKnobW, row1);
    placeKnob(decayKnob,   decayLabel,   772 + 1 * kKnobW, row1);
    placeKnob(sustainKnob, sustainLabel, 772 + 2 * kKnobW, row1);
    placeKnob(releaseKnob, releaseLabel, 772 + 3 * kKnobW, row1);

    // --- Row 2 (y=502) ---
    const int row2 = 502;
    // QUANTIZE: Key + Scale
    keyLabel.setBounds(20, row2, 60, 14);
    keyBox.setBounds(20, row2 + 16, 90, 26);
    scaleLabel.setBounds(126, row2, 80, 14);
    scaleBox.setBounds(126, row2 + 16, 190, 26);

    // TEXTURE: Window + Rev
    windowLabel.setBounds(350, row2, 80, 14);
    windowBox.setBounds(350, row2 + 16, 140, 26);
    revButton.setBounds(506, row2 + 16, 70, 26);

    // SAMPLE: Material
    materialLabel.setBounds(600, row2, 90, 14);
    materialBox.setBounds(600, row2 + 16, 150, 26);

    // MASTER
    placeKnob(hpfKnob,  hpfLabel,  772 + 0 * kKnobW, row2 + 4);
    placeKnob(lpfKnob,  lpfLabel,  772 + 1 * kKnobW, row2 + 4);
    placeKnob(gainKnob, gainLabel, 772 + 2 * kKnobW, row2 + 4);
}
