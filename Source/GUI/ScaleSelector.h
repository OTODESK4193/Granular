// ==============================================================================
// Source/GUI/ScaleSelector.h
//
// スケール選択用ツリー方式コントロール（ModDestSelector を応用）
// 全82種のスケールをカテゴリ別（Basic / Church / Jazz / Ethnic / Synthetic / Chords）
// にサブメニュー展開し、瞬時に目的のスケールを選択可能にする。
// ==============================================================================
#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>
#include "ColorPalette.h"
#include "../DSP/ScaleQuantizer.h"

class ScaleSelector : public juce::Component
{
public:
    ScaleSelector()
    {
        buildMenu = [](int currentScale) {
            return ScaleQuantizer::buildScaleMenu(currentScale);
        };
    }
    ~ScaleSelector() override = default;

    std::function<juce::PopupMenu(int currentScaleIndex)> buildMenu;

    void bindTo(juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
    {
        attachment.reset();
        param = state.getParameter(paramID);
        if (param == nullptr) { jassertfalse; return; }

        attachment = std::make_unique<juce::ParameterAttachment>(
            *param,
            [this](float newDenormalisedValue) { updateFromValue(newDenormalisedValue); },
            nullptr);

        attachment->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        g.setColour(GranularColors::knobTrack);
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(isMouseOver() ? GranularColors::accentPitch : GranularColors::panelLine);
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);

        auto textArea = b.reduced(8.0f, 0.0f);
        textArea.removeFromRight(14.0f);

        g.setColour(GranularColors::text);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(currentText, textArea, juce::Justification::centred, true);

        // 下向き矢印アイコン
        g.setColour(GranularColors::accentPitch);
        const float ax = b.getRight() - 12.0f;
        const float ay = b.getCentreY();
        juce::Path arrow;
        arrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.fillPath(arrow);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseDown(const juce::MouseEvent&) override
    {
        if (buildMenu == nullptr || attachment == nullptr) return;

        juce::PopupMenu menu = buildMenu(currentScaleIndex);
        juce::Component::SafePointer<ScaleSelector> safeThis(this);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [safeThis](int result)
            {
                if (safeThis == nullptr || result <= 0) return;
                if (safeThis->attachment == nullptr) return;
                // itemID = scaleIndex + 1 (1-indexed for PopupMenu)
                safeThis->attachment->setValueAsCompleteGesture((float)(result - 1));
            });
    }

private:
    void updateFromValue(float denormalisedValue)
    {
        currentScaleIndex = juce::roundToInt(denormalisedValue);
        refreshText();
    }

    void refreshText()
    {
        currentText = ScaleQuantizer::scaleName(currentScaleIndex);
        repaint();
    }

    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;

    int currentScaleIndex = 0;
    juce::String currentText { "Chromatic" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSelector)
};
