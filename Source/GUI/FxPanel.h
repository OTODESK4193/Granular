// ==========================================
// File: FxPanel.h
// FXタブ: 4スロットカード（D&Dで適用順を並べ替え = パラメータ値のスワップ）
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "../PluginProcessor.h"
#include "ValueKnob.h"
#include "ColorPalette.h"

// ------------------------------------------
// 1スロット分のカード
// ------------------------------------------
class FxSlotCard : public juce::Component,
                   public juce::DragAndDropTarget
{
public:
    FxSlotCard(GranularAudioProcessor& processor, int slotIndex,
               std::function<void(int, int)> onSwapCallback);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDrag(const juce::MouseEvent& e) override;

    // --- DragAndDropTarget ---
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails&) override { dragOver = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dragOver = false; repaint(); }
    void itemDropped(const SourceDetails& details) override;

private:
    GranularAudioProcessor& proc;
    const int slot; // 0-based
    std::function<void(int, int)> onSwap;

    juce::ComboBox typeBox;
    ValueKnob amountKnob;
    juce::Label amountLabel;

    bool dragOver = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxSlotCard)
};

// ------------------------------------------
// FXタブ本体
// ------------------------------------------
class FxPanel : public juce::Component,
                public juce::DragAndDropContainer
{
public:
    explicit FxPanel(GranularAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void swapSlots(int a, int b); // 1-based ではなく 0-based

    GranularAudioProcessor& proc;

    std::array<std::unique_ptr<FxSlotCard>, FxChain::kNumSlots> cards;

    juce::ComboBox satAlgoBox;
    juce::Label    satAlgoLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> satAlgoAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPanel)
};
