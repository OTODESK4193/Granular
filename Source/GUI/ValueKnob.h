// ==========================================
// File: ValueKnob.h
// ロータリースライダー (Wavetable 準拠 UX):
//   - 右クリック        : Modulation Matrix クイック設定メニュー (onRightClick)
//                         ※ 未接続の場合は直接数値入力にフォールバック
//   - Ctrl + 左クリック : デフォルト値へ復帰 (default_value プロパティ)
//   - 左ダブルクリック  : 数値を直接入力 (DoubleClickReturn 有効スライダー除く)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <functional>
#include "ColorPalette.h"

class ValueKnob : public juce::Slider
{
public:
    ValueKnob() = default;

    // 右クリック時のメニュー表示コールバック（KnobModMenu が接続）
    std::function<void(ValueKnob&)> onRightClick;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            if (onRightClick)
                onRightClick(*this);
            else
                showTextEntry();
            return; // 右ボタンではドラッグによる値変更を行わない
        }

        if (e.mods.isLeftButtonDown() && (e.mods.isCommandDown() || e.mods.isCtrlDown()))
        {
            if (getProperties().contains("default_value"))
            {
                ctrlResetPending = true;
                resetToDefault();
                return; // リセット直後のドラッグ誤動作を防止
            }
        }

        ctrlResetPending = false;
        juce::Slider::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || ctrlResetPending) return;
        juce::Slider::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown()) return;
        if (ctrlResetPending)
        {
            ctrlResetPending = false;
            return;
        }
        juce::Slider::mouseUp(e);
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() || e.mods.isCommandDown() || e.mods.isCtrlDown())
            return;

        // ModPanel の amtSlider 等 (double-click=0.0) は標準挙動を維持
        if (isDoubleClickReturnEnabled())
        {
            juce::Slider::mouseDoubleClick(e);
            return;
        }
        showTextEntry();
    }

    // デフォルト値は attachKnobModMenu() が "default_value" プロパティへ書き込む
    void resetToDefault()
    {
        const auto& props = getProperties();
        if (!props.contains("default_value")) return;

        const double d = (double)props["default_value"];
        setValue(d, juce::sendNotificationSync);
    }

    void showTextEntry()
    {
        auto editor = std::make_unique<juce::TextEditor>();
        editor->setSize(96, 26);
        editor->setJustification(juce::Justification::centred);
        editor->setColour(juce::TextEditor::backgroundColourId, GranularColors::panel);
        editor->setColour(juce::TextEditor::textColourId, GranularColors::text);
        editor->setColour(juce::TextEditor::outlineColourId, GranularColors::panelLine);
        editor->setColour(juce::TextEditor::focusedOutlineColourId, GranularColors::mint.withAlpha(0.7f));
        editor->setInputRestrictions(12, "0123456789.-");
        editor->setText(juce::String(getValue(), 3), juce::dontSendNotification);
        editor->setSelectAllWhenFocused(true);
        editor->setWantsKeyboardFocus(true);

        auto* edPtr = editor.get();
        auto& box = juce::CallOutBox::launchAsynchronously(std::move(editor),
                                                           getScreenBounds(), nullptr);

        juce::Component::SafePointer<ValueKnob> safeThis(this);
        juce::Component::SafePointer<juce::CallOutBox> safeBox(&box);
        juce::Component::SafePointer<juce::TextEditor> safeEd(edPtr);

        edPtr->onReturnKey = [safeThis, safeEd, safeBox]
        {
            if (safeThis != nullptr && safeEd != nullptr)
            {
                const double v = safeEd->getText().getDoubleValue();
                safeThis->setValue(v, juce::sendNotificationSync); // Sliderが自動でレンジにクランプ
            }
            if (safeBox != nullptr) safeBox->dismiss();
        };
        edPtr->onEscapeKey = [safeBox] { if (safeBox != nullptr) safeBox->dismiss(); };
        edPtr->onFocusLost = [safeBox] { if (safeBox != nullptr) safeBox->dismiss(); };

        juce::MessageManager::callAsync([safeEd]
        {
            if (safeEd != nullptr) safeEd->grabKeyboardFocus();
        });
    }

private:
    bool ctrlResetPending = false;
};
