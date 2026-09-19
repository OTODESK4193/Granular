// ==========================================
// File: PluginEditor.h
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <memory>

#include "PluginProcessor.h"
#include "GUI/ArcDial.h"
#include "GUI/DotWaveformDisplay.h"
#include "GUI/MainPanel.h"
#include "GUI/ArpPanel.h"
#include "GUI/ConfigPanel.h"
#include "GUI/FxPanel.h"
#include "GUI/ModPanel.h"
#include "GUI/PresetBrowser.h"
#include "GUI/ColorPalette.h"

class GranularAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit GranularAudioProcessorEditor(GranularAudioProcessor&);
    ~GranularAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum class Tab { Main, Arp, Mod, Fx, Config };

    void timerCallback() override;
    void setActiveTab(Tab t);
    void styleTabButton(juce::TextButton& b, bool active);

    GranularAudioProcessor& proc;
    ArcDialLookAndFeel lnf;

    // --- リサイズ対応 (Wavetable 準拠: アスペクト比固定スケーリング 75%〜200%) ---
    static constexpr int kBaseW = 1080;
    static constexpr int kBaseH = 640;

    struct ContentComponent : public juce::Component
    {
        std::function<void(juce::Graphics&)> onPaint;
        std::function<void()> onLayout;
        void paint(juce::Graphics& g) override { if (onPaint) onPaint(g); }
        void resized() override { if (onLayout) onLayout(); }
    };

    ContentComponent content;
    juce::ComponentBoundsConstrainer constrainer;

    void paintContent(juce::Graphics& g);
    void layoutContent();

    DotWaveformDisplay waveDisplay;

    // --- タブ ---
    juce::TextButton mainTabButton   { "MAIN" };
    juce::TextButton arpTabButton    { "ARP" };
    juce::TextButton modTabButton    { "MOD" };
    juce::TextButton fxTabButton     { "FX" };
    juce::TextButton configTabButton { "CONFIG" };
    Tab activeTab = Tab::Main;

    MainPanel mainPanel;
    ArpPanel arpPanel;
    ConfigPanel configPanel;
    FxPanel fxPanel;
    ModPanel modPanel;
    PresetBrowser presetBrowser;
    void toggleBrowser();

    // --- ヘッダー ---
    juce::TextButton loadButton    { "LOAD" };
    juce::TextButton analyzeButton { "ANALYZE" };
    juce::TextButton deleteButton  { "DELETE" };
    juce::TextButton resetButton   { "RESET" };
    juce::TextButton dryPlayButton { "PLAY DRY" };
    juce::TextButton panicButton   { "PANIC" };
    juce::TextButton browseButton  { "BROWSE" };
    juce::TextButton randomButton  { "RANDOM" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String hudFile, hudTask, hudOrig, hudCorr;
    int lastThemeIndex = -1;    // カラーテーマ変更検知
    int themeMsgFrames = 0;     // テーマ変更後の再起動バナー残フレーム
    bool dryPlaying = false;    // PLAY DRYボタン表示の状態
    juce::Label themeBanner;    // テーマ変更→再起動促しバナー

    void confirmThen(const juce::String& title, const juce::String& message,
                     std::function<void()> action);
    void resetAllParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularAudioProcessorEditor)
};
