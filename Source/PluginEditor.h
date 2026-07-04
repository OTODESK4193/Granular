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

    // --- ヘッダー ---
    juce::TextButton loadButton    { "LOAD" };
    juce::TextButton analyzeButton { "ANALYZE" };
    juce::TextButton deleteButton  { "DELETE" };
    juce::TextButton resetButton   { "RESET" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String hudFile, hudTask, hudOrig, hudCorr;

    void confirmThen(const juce::String& title, const juce::String& message,
                     std::function<void()> action);
    void resetAllParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularAudioProcessorEditor)
};
