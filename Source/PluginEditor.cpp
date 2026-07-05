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
    : AudioProcessorEditor(&p), proc(p), waveDisplay(p), mainPanel(p), arpPanel(p),
      configPanel(p), fxPanel(p), modPanel(p), presetBrowser(p.getUserPresetDir())
{
    setLookAndFeel(&lnf);

    addAndMakeVisible(waveDisplay);
    addAndMakeVisible(mainPanel);
    addChildComponent(arpPanel);  // 非表示で追加
    addChildComponent(modPanel);
    addChildComponent(fxPanel);
    addChildComponent(configPanel);

    // --- タブボタン ---
    for (auto* b : { &mainTabButton, &arpTabButton, &modTabButton, &fxTabButton, &configTabButton })
        addAndMakeVisible(*b);

    mainTabButton.onClick   = [this] { setActiveTab(Tab::Main); };
    arpTabButton.onClick    = [this] { setActiveTab(Tab::Arp); };
    modTabButton.onClick    = [this] { setActiveTab(Tab::Mod); };
    fxTabButton.onClick     = [this] { setActiveTab(Tab::Fx); };
    configTabButton.onClick = [this] { setActiveTab(Tab::Config); };
    setActiveTab(Tab::Main);

    // --- ヘッダーボタン (LOAD以外はY/N確認付き) ---
    auto styleHeaderButton = [this](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
        b.setColour(juce::TextButton::textColourOffId, GranularColors::text);
        addAndMakeVisible(b);
    };
    styleHeaderButton(loadButton);
    styleHeaderButton(analyzeButton);
    styleHeaderButton(deleteButton);
    styleHeaderButton(resetButton);
    styleHeaderButton(dryPlayButton);
    styleHeaderButton(browseButton);
    styleHeaderButton(randomButton);
    randomButton.onClick = [this] { proc.randomizePreset(); };

    dryPlayButton.onClick = [this]
    {
        if (!proc.hasSampleFile()) return;
        if (proc.dryPreviewActive.load()) proc.stopDryPreview();  // もう一度押すと停止
        else                              proc.startDryPreview();
    };

    // --- プリセットブラウザ ---
    browseButton.onClick = [this] { toggleBrowser(); };
    addChildComponent(presetBrowser);
    presetBrowser.onLoadFactory = [this](int i)          { proc.applyFactoryPreset(i); };
    presetBrowser.onLoadUser    = [this](const juce::File& f) { proc.loadUserPresetFile(f); };
    presetBrowser.onSave        = [this](const juce::String& n) { proc.saveUserPreset(n); };
    presetBrowser.onInit        = [this] { proc.resetAllParametersToDefault(); };
    presetBrowser.onRandom      = [this] { proc.randomizePreset(); };
    presetBrowser.onClose       = [this] { presetBrowser.setVisible(false);
                                           browseButton.setColour(juce::TextButton::textColourOffId, GranularColors::text); };

    // カラーテーマの初期反映
    lastThemeIndex = (int)proc.apvts.getRawParameterValue("colorTheme")->load();
    GranularColors::setTheme(lastThemeIndex);

    // テーマ変更→再起動促しバナー（最前面・クリック透過、初期は非表示）
    themeBanner.setText("Color theme changed - reopen the plugin window for full effect",
                        juce::dontSendNotification);
    themeBanner.setJustificationType(juce::Justification::centred);
    themeBanner.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::bold)));
    themeBanner.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.78f));
    themeBanner.setColour(juce::Label::textColourId, juce::Colours::white);
    themeBanner.setColour(juce::Label::outlineColourId, GranularColors::mint.withAlpha(0.9f));
    themeBanner.setInterceptsMouseClicks(false, false);
    addChildComponent(themeBanner);  // 最後に追加=最前面

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

    analyzeButton.onClick = [this]
    {
        if (!proc.hasSampleFile()) return;
        confirmThen("Re-Analyze",
                    juce::String::fromUTF8("\xE7\x8F\xBE\xE5\x9C\xA8\xE3\x81\xAE\xE3\x82\xB5\xE3\x83\xB3\xE3\x83\x97\xE3\x83\xAB\xE3\x82\x92\xE5\x86\x8D\xE8\xA7\xA3\xE6\x9E\x90\xE3\x81\x97\xE3\x81\xBE\xE3\x81\x99\xE3\x81\x8B\xEF\xBC\x9F"),
                    [this] { proc.reingestCurrentFile(); });
    };

    deleteButton.onClick = [this]
    {
        confirmThen("Delete Sample",
                    juce::String::fromUTF8("\xE8\xAA\xAD\xE3\x81\xBF\xE8\xBE\xBC\xE3\x82\x93\xE3\x81\xA0\xE3\x82\xB5\xE3\x83\xB3\xE3\x83\x97\xE3\x83\xAB\xE3\x82\x92\xE7\xA0\xB4\xE6\xA3\x84\xE3\x81\x97\xE3\x81\xBE\xE3\x81\x99\xE3\x81\x8B\xEF\xBC\x9F"),
                    [this] { proc.clearSample(); });
    };

    resetButton.onClick = [this]
    {
        confirmThen("Reset Parameters",
                    juce::String::fromUTF8("\xE3\x81\x99\xE3\x81\xB9\xE3\x81\xA6\xE3\x81\xAE\xE3\x83\x8E\xE3\x83\x96/\xE8\xA8\xAD\xE5\xAE\x9A\xE3\x82\x92\xE3\x83\x87\xE3\x83\x95\xE3\x82\xA9\xE3\x83\xAB\xE3\x83\x88\xE3\x81\xAB\xE6\x88\xBB\xE3\x81\x97\xE3\x81\xBE\xE3\x81\x99\xE3\x81\x8B\xEF\xBC\x9F"),
                    [this] { resetAllParameters(); });
    };

    startTimerHz(10); // HUD更新
    setSize(1080, 640);
}

GranularAudioProcessorEditor::~GranularAudioProcessorEditor()
{
    stopTimer();               // 破棄中にHUDタイマーが発火しないよう明示停止
    setLookAndFeel(nullptr);
}

// ------------------------------------------
// 確認ダイアログ (Y/N)
// ------------------------------------------
void GranularAudioProcessorEditor::confirmThen(const juce::String& title, const juce::String& message,
                                               std::function<void()> action)
{
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon, title, message, "Yes", "No", this,
        juce::ModalCallbackFunction::create([action](int result)
        {
            if (result == 1 && action != nullptr) action();
        }));
}

void GranularAudioProcessorEditor::resetAllParameters()
{
    for (auto* param : proc.getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());
}

void GranularAudioProcessorEditor::toggleBrowser()
{
    const bool show = !presetBrowser.isVisible();
    if (show)
    {
        presetBrowser.refresh();       // User一覧を再スキャン
        presetBrowser.toFront(false);  // 最前面へ
    }
    presetBrowser.setVisible(show);
    browseButton.setColour(juce::TextButton::textColourOffId,
                           show ? GranularColors::mint : GranularColors::text);
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

    // タブを押したらブラウザを閉じて該当タブを表示
    if (presetBrowser.isVisible())
    {
        presetBrowser.setVisible(false);
        browseButton.setColour(juce::TextButton::textColourOffId, GranularColors::text);
    }
    mainPanel.setVisible(t == Tab::Main);
    arpPanel.setVisible(t == Tab::Arp);
    modPanel.setVisible(t == Tab::Mod);
    fxPanel.setVisible(t == Tab::Fx);
    configPanel.setVisible(t == Tab::Config);

    styleTabButton(mainTabButton,   t == Tab::Main);
    styleTabButton(arpTabButton,    t == Tab::Arp);
    styleTabButton(modTabButton,    t == Tab::Mod);
    styleTabButton(fxTabButton,     t == Tab::Fx);
    styleTabButton(configTabButton, t == Tab::Config);
    repaint(0, kTabY, getWidth(), kTabH + 8);
}

// ------------------------------------------
// HUD 更新
// ------------------------------------------
void GranularAudioProcessorEditor::timerCallback()
{
    // カラーテーマ切替の検知 → パレット更新 & 全再描画 + 再起動バナー
    const int themeIdx = (int)proc.apvts.getRawParameterValue("colorTheme")->load();
    if (themeIdx != lastThemeIndex)
    {
        const bool firstTime = (lastThemeIndex < 0);
        lastThemeIndex = themeIdx;
        GranularColors::setTheme(themeIdx);
        if (!firstTime) themeMsgFrames = 45; // 約4.5秒バナー表示（初回起動時は出さない）
        repaint();
        for (auto* child : getChildren()) child->repaint();
    }
    if (themeMsgFrames > 0)
    {
        themeBanner.setVisible(true);
        if (--themeMsgFrames == 0) themeBanner.setVisible(false);
    }

    // PLAY DRY ボタンの表示をトグル状態に追従
    const bool nowPlaying = proc.dryPreviewActive.load();
    if (nowPlaying != dryPlaying)
    {
        dryPlaying = nowPlaying;
        dryPlayButton.setButtonText(dryPlaying ? "STOP DRY" : "PLAY DRY");
    }

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

    // --- HUD (ロゴの隣・解析結果4行、左揃え) ---
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.setColour(GranularColors::text.withAlpha(0.85f));
    g.drawText(hudFile, 360, 8,  316, 13, juce::Justification::centredLeft);
    g.setColour(GranularColors::textDim);
    g.drawText(hudTask, 360, 22, 316, 13, juce::Justification::centredLeft);
    g.drawText(hudOrig, 360, 36, 316, 13, juce::Justification::centredLeft);
    g.drawText(hudCorr, 360, 50, 316, 13, juce::Justification::centredLeft);

    // --- アクティブタブのアンダーライン ---
    auto underline = [&g](const juce::TextButton& b, juce::Colour c)
    {
        const auto r = b.getBounds();
        g.setColour(c);
        g.fillRoundedRectangle((float)r.getX() + 6.0f, (float)r.getBottom() + 2.0f,
                               (float)r.getWidth() - 12.0f, 2.5f, 1.25f);
    };

    if (activeTab == Tab::Main)   underline(mainTabButton,   GranularColors::mint);
    if (activeTab == Tab::Arp)    underline(arpTabButton,    GranularColors::peach);
    if (activeTab == Tab::Mod)    underline(modTabButton,    GranularColors::lavender);
    if (activeTab == Tab::Fx)     underline(fxTabButton,     GranularColors::pink);
    if (activeTab == Tab::Config) underline(configTabButton, GranularColors::sage);
}

// ------------------------------------------
// レイアウト
// ------------------------------------------
void GranularAudioProcessorEditor::resized()
{
    waveDisplay.setBounds(16, 68, getWidth() - 32, 236);

    // ヘッダー右: 2×2 ボタングリッド
    loadButton.setBounds(888, 8, 80, 22);
    analyzeButton.setBounds(976, 8, 88, 22);
    deleteButton.setBounds(888, 34, 80, 22);
    resetButton.setBounds(976, 34, 88, 22);

    // Dryプレビュー再生ボタン + BROWSE (ボタングリッドの左に縦積み)
    dryPlayButton.setBounds(792, 8, 88, 22);
    browseButton.setBounds(792, 34, 88, 22);
    // RANDOM (BROWSEの左)
    randomButton.setBounds(696, 34, 88, 22);

    // プリセットブラウザ (タブパネルと同じ下部領域に展開)
    presetBrowser.setBounds(0, kPanelY, getWidth(), getHeight() - kPanelY);

    // テーマ変更バナー (波形上部に重ねて表示)
    themeBanner.setBounds(getWidth() / 2 - 260, 80, 520, 28);

    mainTabButton.setBounds(20,  kTabY, 92, kTabH);
    arpTabButton.setBounds(118, kTabY, 92, kTabH);
    modTabButton.setBounds(216, kTabY, 92, kTabH);
    fxTabButton.setBounds(314, kTabY, 92, kTabH);
    configTabButton.setBounds(412, kTabY, 92, kTabH);

    const auto panelBounds = juce::Rectangle<int>(0, kPanelY, getWidth(), getHeight() - kPanelY);
    mainPanel.setBounds(panelBounds);
    arpPanel.setBounds(panelBounds);
    modPanel.setBounds(panelBounds);
    fxPanel.setBounds(panelBounds);
    configPanel.setBounds(panelBounds);
}
