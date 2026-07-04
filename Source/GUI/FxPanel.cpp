// ==========================================
// File: FxPanel.cpp
// ==========================================
#include "FxPanel.h"

// ------------------------------------------
// FxSlotCard
// ------------------------------------------
FxSlotCard::FxSlotCard(GranularAudioProcessor& processor, int slotIndex,
                       std::function<void(int, int)> onSwapCallback)
    : proc(processor), slot(slotIndex), onSwap(std::move(onSwapCallback))
{
    const juce::String idx(slot + 1);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("fx" + idx + "Type")))
        typeBox.addItemList(choiceParam->choices, 1);
    addAndMakeVisible(typeBox);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "fx" + idx + "Type", typeBox);

    amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    amountKnob.setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentMaster);
    amountKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(amountKnob);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "fx" + idx + "Amount", amountKnob);

    amountLabel.setText("AMOUNT", juce::dontSendNotification);
    amountLabel.setJustificationType(juce::Justification::centred);
    amountLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    amountLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(amountLabel);
}

void FxSlotCard::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // カード背景
    g.setColour(GranularColors::panel);
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(dragOver ? GranularColors::mint.withAlpha(0.9f) : GranularColors::panelLine);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 10.0f, dragOver ? 2.0f : 1.0f);

    // ヘッダー (ドラッグハンドル)
    g.setColour(GranularColors::knobTrack.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.withHeight(26.0f), 10.0f);

    g.setColour(GranularColors::accentMaster);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText("SLOT " + juce::String(slot + 1), 12, 4, 100, 18, juce::Justification::centredLeft);

    // ドラッグハンドルアイコン (≡)
    g.setColour(GranularColors::textDim);
    const float hx = bounds.getWidth() - 28.0f;
    for (int i = 0; i < 3; ++i)
        g.fillRoundedRectangle(hx, 8.0f + (float)i * 4.5f, 16.0f, 2.0f, 1.0f);
}

void FxSlotCard::resized()
{
    typeBox.setBounds(12, 34, getWidth() - 24, 26);
    amountKnob.setBounds((getWidth() - 64) / 2, 68, 64, 60);
    amountLabel.setBounds(0, 128, getWidth(), 14);
}

void FxSlotCard::mouseDrag(const juce::MouseEvent& e)
{
    // ヘッダー部(上26px)からのドラッグでスロット移動開始
    if (e.mouseDownPosition.getY() <= 26.0f)
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            if (!container->isDragAndDropActive())
                container->startDragging(juce::var(slot), this);
}

bool FxSlotCard::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.isInt() && (int)details.description != slot;
}

void FxSlotCard::itemDropped(const SourceDetails& details)
{
    dragOver = false;
    repaint();
    if (onSwap != nullptr)
        onSwap((int)details.description, slot);
}

// ------------------------------------------
// FxPanel
// ------------------------------------------
FxPanel::FxPanel(GranularAudioProcessor& p) : proc(p)
{
    for (int i = 0; i < FxChain::kNumSlots; ++i)
    {
        cards[(size_t)i] = std::make_unique<FxSlotCard>(proc, i,
            [this](int a, int b) { swapSlots(a, b); });
        addAndMakeVisible(*cards[(size_t)i]);
    }

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("satAlgo")))
        satAlgoBox.addItemList(choiceParam->choices, 1);
    addAndMakeVisible(satAlgoBox);
    satAlgoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "satAlgo", satAlgoBox);

    satAlgoLabel.setText("SAT ALGO", juce::dontSendNotification);
    satAlgoLabel.setJustificationType(juce::Justification::centredLeft);
    satAlgoLabel.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    satAlgoLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(satAlgoLabel);
}

void FxPanel::swapSlots(int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= FxChain::kNumSlots || b >= FxChain::kNumSlots)
        return;

    // Type と Amount の正規化値を入れ替える（適用順の並べ替えに相当）
    for (const auto* suffix : { "Type", "Amount" })
    {
        auto* pa = proc.apvts.getParameter("fx" + juce::String(a + 1) + suffix);
        auto* pb = proc.apvts.getParameter("fx" + juce::String(b + 1) + suffix);
        if (pa == nullptr || pb == nullptr) continue;

        const float va = pa->getValue();
        const float vb = pb->getValue();

        pa->beginChangeGesture();
        pa->setValueNotifyingHost(vb);
        pa->endChangeGesture();

        pb->beginChangeGesture();
        pb->setValueNotifyingHost(va);
        pb->endChangeGesture();
    }
}

void FxPanel::paint(juce::Graphics& g)
{
    auto drawSection = [&g](const juce::String& name, int x, int y, int w, juce::Colour accent)
    {
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawText(name, x, y, w, 14, juce::Justification::centredLeft);
        g.setColour(accent.withAlpha(0.35f));
        g.fillRect(x, y + 16, w, 1);
    };

    drawSection("FX RACK  (signal flows Slot 1 -> 4)", 20, 6, 1040, GranularColors::accentMaster);

    g.setColour(GranularColors::textDim.withAlpha(0.7f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Drag a slot header to another slot to swap the processing order.",
               210, 216, 500, 14, juce::Justification::centredLeft);
    g.drawText("SAT ALGO applies to all Saturation slots.",
               210, 234, 500, 14, juce::Justification::centredLeft);

    // スロット間の矢印
    g.setColour(GranularColors::textDim.withAlpha(0.55f));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    for (int i = 0; i < FxChain::kNumSlots - 1; ++i)
    {
        const int x = 20 + 248 + i * 262;
        g.drawText(">", x, 100, 14, 20, juce::Justification::centred);
    }
}

void FxPanel::resized()
{
    for (int i = 0; i < FxChain::kNumSlots; ++i)
        cards[(size_t)i]->setBounds(20 + i * 262, 36, 248, 148);

    satAlgoLabel.setBounds(20, 216, 80, 14);
    satAlgoBox.setBounds(20, 232, 160, 26);
}
