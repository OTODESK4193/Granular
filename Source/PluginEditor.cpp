// ==========================================
// File: PluginEditor.cpp
// ==========================================
#include "PluginEditor.h"

namespace
{
    constexpr int kTabY = 312;
    constexpr int kTabH = 26;
    constexpr int kPanelY = 344;
}

GranularAudioProcessorEditor::GranularAudioProcessorEditor(GranularAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p), waveDisplay(p), mainPanel(p)
{
    setLookAndFeel(&lnf);

    addAndMakeVisible(waveDisplay);
    addAndMakeVisible(mainPanel);
    addChildComponent(modPanel);  // 非表示で追加
    addChildComponent(fxPanel);

    // --- タブボタン ---
    for (auto* b : { &mainTabButton, &modTabButton, &fxTabButton })
        addAndMakeVisible(*b);

    mainTabButton.onClick = [this] { setActiveTab(Tab::Main); };
    modTabButton.onClick  = [this] { setActiveTab(Tab::Mod); };
    fxTabButton.onClick   = [this] { setActiveTab(Tab::Fx); };
    setActiveTab(Tab::Main);

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
// タブ切り替え
// ------------------------------------------
void GranularAudioProcessorEditor::styleTabButton(juce::TextButton& b, bool active)
{
    b.setColour(juce::TextButton::buttonColourId,
                active ? GranularColors::panel : juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOffId,
                active ? GranularColors::text : GranularColors::textDim);
}

void GranularAudioProcessorEditor::setActiveTab(Tab t)
{
    activeTab = t;
    mainPanel.setVisible(t == Tab::Main);
    modPanel.setVisible(t == Tab::Mod);
    fxPanel.setVisible(t == Tab::Fx);

    styleTabButton(mainTabButton, t == Tab::Main);
    styleTabButton(modTabButton,  t == Tab::Mod);
    styleTabButton(fxTabButton,   t == Tab::Fx);
    repaint(0, kTabY, getWidth(), kTabH + 8);
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

    // --- アクティブタブのアンダーライン ---
    auto underline = [&g](const juce::TextButton& b, juce::Colour c)
    {
        const auto r = b.getBounds();
        g.setColour(c);
        g.fillRoundedRectangle((float)r.getX() + 6.0f, (float)r.getBottom() + 2.0f,
                               (float)r.getWidth() - 12.0f, 2.5f, 1.25f);
    };

    if (activeTab == Tab::Main) underline(mainTabButton, GranularColors::mint);
    if (activeTab == Tab::Mod)  underline(modTabButton,  GranularColors::lavender);
    if (activeTab == Tab::Fx)   underline(fxTabButton,   GranularColors::pink);
}

// ------------------------------------------
// レイアウト
// ------------------------------------------
void GranularAudioProcessorEditor::resized()
{
    waveDisplay.setBounds(16, 68, getWidth() - 32, 236);
    loadButton.setBounds(getWidth() - 76, 40, 60, 22);

    mainTabButton.setBounds(20,  kTabY, 100, kTabH);
    modTabButton.setBounds(126, kTabY, 100, kTabH);
    fxTabButton.setBounds(232, kTabY, 100, kTabH);

    const auto panelBounds = juce::Rectangle<int>(0, kPanelY, getWidth(), getHeight() - kPanelY);
    mainPanel.setBounds(panelBounds);
    modPanel.setBounds(panelBounds);
    fxPanel.setBounds(panelBounds);
}
