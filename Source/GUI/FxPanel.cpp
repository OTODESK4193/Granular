// ==========================================
// File: FxPanel.cpp
// ==========================================
#include "FxPanel.h"

namespace
{
    constexpr int kCardW = 196;
    constexpr int kCardH = 136;
    constexpr int kCardGap = 211; // カード間隔 (5枚: 20 + i*211)
    constexpr int kCardY = 32;
    constexpr int kDetailY = 180;
}

// ------------------------------------------
// FxSlotCard
// ------------------------------------------
FxSlotCard::FxSlotCard(GranularAudioProcessor& processor, int slotIndex,
                       std::function<void(int, int)> onSwapCallback,
                       std::function<void(int)> onSelectCallback,
                       std::function<void()> onTypeChangedCallback)
    : proc(processor), slot(slotIndex),
      onSwap(std::move(onSwapCallback)),
      onSelect(std::move(onSelectCallback)),
      onTypeChanged(std::move(onTypeChangedCallback))
{
    const juce::String idx(slot + 1);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("fx" + idx + "Type")))
        typeBox.addItemList(choiceParam->choices, 1);

    // 他のスロットですでに使用されているFXタイプは非活性化 (Spectra8準拠・重複禁止)
    typeBox.setItemEnabledPredicate([this](int itemId) -> bool
    {
        const int fxType = itemId - 1; // 1-based (addItemList starts at 1) to 0-based FxType
        if (fxType <= 0) return true;  // None (0) は常にどのスロットでも選択可能

        // 自分のスロットで現在選択中のタイプなら選択可能
        auto* myParam = proc.apvts.getRawParameterValue("fx" + juce::String(slot + 1) + "Type");
        const int myType = (myParam != nullptr) ? (int)myParam->load() : 0;
        if (fxType == myType) return true;

        // 他のスロットで既に選択されているFXタイプは非活性化 (重複禁止)
        for (int s = 0; s < FxChain::kNumSlots; ++s)
        {
            if (s == slot) continue;
            auto* p = proc.apvts.getRawParameterValue("fx" + juce::String(s + 1) + "Type");
            if (p != nullptr && (int)p->load() == fxType)
                return false; // 他のスロットで使用中のため非活性化
        }
        return true;
    });

    addAndMakeVisible(typeBox);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "fx" + idx + "Type", typeBox);
    typeBox.onChange = [this]
    {
        if (onSelect != nullptr) onSelect(slot);        // 触ったスロットを選択
        if (onTypeChanged != nullptr) onTypeChanged();  // 詳細エリアを更新
    };

    amountKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    amountKnob.setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentMaster);
    amountKnob.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(amountKnob);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "fx" + idx + "Amount", amountKnob);

    amountLabel.setText("AMOUNT", juce::dontSendNotification);
    amountLabel.setJustificationType(juce::Justification::centred);
    amountLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    amountLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
    addAndMakeVisible(amountLabel);

    startTimerHz(30);
}

FxSlotCard::~FxSlotCard() { stopTimer(); }

void FxSlotCard::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour(selected ? GranularColors::panel.brighter(0.08f) : GranularColors::panel);
    g.fillRoundedRectangle(bounds, 10.0f);

    juce::Colour border = GranularColors::panelLine;
    float borderW = 1.0f;
    if (dragOver)      { border = GranularColors::mint.withAlpha(0.9f); borderW = 2.0f; }
    else if (selected) { border = GranularColors::accentMaster.withAlpha(0.85f); borderW = 1.6f; }
    g.setColour(border);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 10.0f, borderW);

    // ヘッダー (ドラッグハンドル)
    g.setColour(GranularColors::knobTrack.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.withHeight(24.0f), 10.0f);

    g.setColour(selected ? GranularColors::accentMaster : GranularColors::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    g.drawText("SLOT " + juce::String(slot + 1), 10, 3, 90, 18, juce::Justification::centredLeft);

    // ドラッグハンドルアイコン (≡)
    g.setColour(GranularColors::textDim);
    const float hx = bounds.getWidth() - 26.0f;
    for (int i = 0; i < 3; ++i)
        g.fillRoundedRectangle(hx, 7.0f + (float)i * 4.0f, 14.0f, 2.0f, 1.0f);
}

void FxSlotCard::resized()
{
    typeBox.setBounds(10, 30, getWidth() - 20, 24);
    amountKnob.setBounds((getWidth() - 54) / 2, 60, 54, 52);
    amountLabel.setBounds(0, 114, getWidth(), 13);
}

void FxSlotCard::mouseDown(const juce::MouseEvent&)
{
    if (onSelect != nullptr) onSelect(slot);
}

void FxSlotCard::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mouseDownPosition.getY() <= 24.0f)
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
    if (onSwap != nullptr && details.description.isInt())
        onSwap((int)details.description, slot);
}

void FxSlotCard::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
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

void FxSlotCard::timerCallback()
{
    applyModRing(amountKnob, ModMatrix::DstFx1Amount + slot, LinearAdd, 100.0f);
}

// ------------------------------------------
// FxPanel
// ------------------------------------------
FxPanel::FxPanel(GranularAudioProcessor& p) : proc(p)
{
    for (int i = 0; i < FxChain::kNumSlots; ++i)
    {
        cards[(size_t)i] = std::make_unique<FxSlotCard>(proc, i,
            [this](int a, int b) { swapSlots(a, b); },
            [this](int s) { selectSlot(s); },
            [this] { rebuildDetails(); });
        addAndMakeVisible(*cards[(size_t)i]);
    }

    selectSlot(0);
    startTimerHz(30);
}

FxPanel::~FxPanel() { stopTimer(); }

int FxPanel::getSlotType(int slot) const
{
    return (int)proc.apvts.getRawParameterValue("fx" + juce::String(slot + 1) + "Type")->load();
}

void FxPanel::selectSlot(int slot)
{
    slot = juce::jlimit(0, FxChain::kNumSlots - 1, slot);
    selectedSlot = slot;
    for (int i = 0; i < FxChain::kNumSlots; ++i)
        if (cards[(size_t)i] != nullptr)
            cards[(size_t)i]->setSelected(i == slot);
    rebuildDetails();
    repaint();
}

void FxPanel::swapSlots(int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= FxChain::kNumSlots || b >= FxChain::kNumSlots)
        return;

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

    selectSlot(b); // 移動先を選択状態に
}

// ------------------------------------------
// 詳細エリアの再構築
// ------------------------------------------
void FxPanel::rebuildDetails()
{
    // 破棄 (Attachment → Component の順)
    detailComboAttachments.clear();
    detailKnobAttachments.clear();
    detailCombos.clear();
    detailComboLabels.clear();
    detailKnobs.clear();
    detailKnobLabels.clear();

    struct ComboDef { const char* id; const char* label; bool isSubTree = false; };
    std::vector<ComboDef> comboDefs;
    struct KnobDef { const char* id; const char* label; };
    std::vector<KnobDef> knobDefs;

    switch (getSlotType(selectedSlot))
    {
    case FxChain::Saturation:
        comboDefs = { { "satAlgo", "ALGO", false } };
        knobDefs  = { { "satDrive", "DRIVE" }, { "satPreHz", "PRE-HPF" }, { "satTrim", "TRIM" } };
        break;
    case FxChain::Chorus:
        knobDefs  = { { "choRate", "RATE" }, { "choDepth", "DEPTH" }, { "choWidth", "WIDTH" } };
        break;
    case FxChain::Delay:
        comboDefs = { { "dlyTime", "TIME", false } };
        knobDefs  = { { "dlyFeedback", "FEEDBACK" }, { "dlyDuck", "DUCK" }, { "dlyDamp", "DAMP" } };
        break;
    case FxChain::Freeze:
        knobDefs  = { { "frzSize", "SIZE" }, { "frzFeedback", "FEEDBACK" }, { "frzDamp", "DAMP" } };
        break;
    case FxChain::Reverb:
        knobDefs  = { { "revDecay", "DECAY" }, { "revShimmer", "SHIMMER" }, { "revDamp", "DAMP" }, { "revMod", "MOD" } };
        break;
    case FxChain::Gate:
        comboDefs = { { "gateRate", "RATE", false },
                      { "gatePattern", "PATTERN", true } };
        knobDefs  = { { "gateDepth", "DEPTH" }, { "gateDecay", "DECAY" }, { "gateVowel", "VOWEL" } };
        break;
    default:
        break; // None: 詳細なし
    }

    for (const auto& d : comboDefs)
    {
        auto combo = std::make_unique<juce::ComboBox>();
        if (d.isSubTree)
        {
            // SPECTRA8 完全移植: 5カテゴリ階層ツリーメニュー
            const auto patternNames = gfx::FormantGate::getPatternNames();
            auto* root = combo->getRootMenu();
            root->clear();

            juce::PopupMenu subStraight;
            for (int i = 0; i < 10; ++i) subStraight.addItem(i + 1, patternNames[i]);
            root->addSubMenu("Straight (10)", subStraight);

            juce::PopupMenu subTriplet;
            for (int i = 10; i < 18; ++i) subTriplet.addItem(i + 1, patternNames[i]);
            root->addSubMenu("Triplet & Swing (8)", subTriplet);

            juce::PopupMenu subDotted;
            for (int i = 18; i < 26; ++i) subDotted.addItem(i + 1, patternNames[i]);
            root->addSubMenu("Dotted & Polyrhythm (8)", subDotted);

            juce::PopupMenu subEdm;
            for (int i = 26; i < 38; ++i) subEdm.addItem(i + 1, patternNames[i]);
            root->addSubMenu("ColorBass & EDM (12)", subEdm);

            juce::PopupMenu subGlitch;
            for (int i = 38; i < 50; ++i) subGlitch.addItem(i + 1, patternNames[i]);
            root->addSubMenu("Glitch & Complex (12)", subGlitch);
        }
        else
        {
            if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(d.id)))
                combo->addItemList(choiceParam->choices, 1);
        }

        combo->setColour(juce::ComboBox::backgroundColourId, GranularColors::panel);
        combo->setColour(juce::ComboBox::textColourId, GranularColors::text);
        combo->setColour(juce::ComboBox::outlineColourId, GranularColors::panelLine);
        addAndMakeVisible(*combo);
        detailComboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            proc.apvts, d.id, *combo));

        auto label = std::make_unique<juce::Label>();
        label->setText(d.label, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(*label);

        detailCombos.push_back(std::move(combo));
        detailComboLabels.push_back(std::move(label));
    }

    for (const auto& def : knobDefs)
    {
        auto knob = std::make_unique<ValueKnob>();
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob->setColour(juce::Slider::rotarySliderFillColourId, GranularColors::accentGrain);
        knob->setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(*knob);
        detailKnobAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.apvts, def.id, *knob));
        detailKnobs.push_back(std::move(knob));

        auto label = std::make_unique<juce::Label>();
        label->setText(def.label, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId, GranularColors::textDim);
        addAndMakeVisible(*label);
        detailKnobLabels.push_back(std::move(label));
    }

    layoutDetails();
    repaint();
}

void FxPanel::applyModRing(juce::Slider& s, int dst, ModKind kind, float scale)
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

void FxPanel::timerCallback()
{
    int type = getSlotType(selectedSlot);
    using D = ModMatrix;
    
    // Typeに応じて、対応するknobにModRingを適用する
    if (type == FxChain::Saturation)
    {
        if (detailKnobs.size() >= 3)
        {
            applyModRing(*detailKnobs[0], D::DstSatDrive, LinearAdd, 5.5f);
            applyModRing(*detailKnobs[1], D::DstSatPreHpf, ExpOct, 5.0f);
            applyModRing(*detailKnobs[2], D::DstSatTrim, LinearAdd, 12.0f);
        }
    }
    else if (type == FxChain::Chorus)
    {
        if (detailKnobs.size() >= 3)
        {
            applyModRing(*detailKnobs[0], D::DstChoRate, LinearAdd, 2.0f);
            applyModRing(*detailKnobs[1], D::DstChoDepth, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[2], D::DstChoWidth, LinearAdd, 1.0f);
        }
    }
    else if (type == FxChain::Delay)
    {
        if (detailKnobs.size() >= 3)
        {
            applyModRing(*detailKnobs[0], D::DstDlyFeedback, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[1], D::DstDlyDuck, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[2], D::DstDlyDamp, LinearAdd, 1.0f);
        }
    }
    else if (type == FxChain::Freeze)
    {
        if (detailKnobs.size() >= 3)
        {
            applyModRing(*detailKnobs[0], D::DstFrzSize, LinearAdd, 500.0f);
            applyModRing(*detailKnobs[1], D::DstFrzFeedback, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[2], D::DstFrzDamp, LinearAdd, 1.0f);
        }
    }
    else if (type == FxChain::Reverb)
    {
        if (detailKnobs.size() >= 4)
        {
            applyModRing(*detailKnobs[0], D::DstRevDecay, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[1], D::DstRevShimmer, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[2], D::DstRevDamp, LinearAdd, 1.0f);
            applyModRing(*detailKnobs[3], D::DstRevMod, LinearAdd, 1.0f);
        }
    }
    else if (type == FxChain::Gate)
    {
        if (detailKnobs.size() >= 3)
        {
            applyModRing(*detailKnobs[0], D::DstGateDepth, LinearAdd, 100.0f);
            applyModRing(*detailKnobs[1], D::DstGateDecay, LinearAdd, 100.0f);
            applyModRing(*detailKnobs[2], D::DstGateVowel, LinearAdd, 100.0f);
        }
    }
}

void FxPanel::layoutDetails()
{
    int x = 20;

    for (size_t i = 0; i < detailCombos.size(); ++i)
    {
        const int w = (i == 1 && getSlotType(selectedSlot) == FxChain::Gate) ? 170 : 120;
        detailComboLabels[i]->setBounds(x, kDetailY + 24, w, 14);
        detailCombos[i]->setBounds(x, kDetailY + 40, w, 26);
        x += w + 20;
    }

    for (size_t i = 0; i < detailKnobs.size(); ++i)
    {
        detailKnobs[i]->setBounds(x, kDetailY + 32, 58, 56);
        detailKnobLabels[i]->setBounds(x - 8, kDetailY + 90, 74, 13);
        x += 82;
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

    drawSection("FX RACK  (signal flows Slot 1 -> 5, drag headers to reorder)", 20, 6, 1040,
                GranularColors::accentMaster);

    // 詳細エリアのヘッダー (選択FX名)
    const int t = getSlotType(selectedSlot);
    juce::String sectionTitle;
    if (t == FxChain::Gate)
    {
        sectionTitle = "DETAILS  -  SLOT " + juce::String(selectedSlot + 1) + "  [FORMANT RHYTHM GATE (50 PATTERNS)]";
    }
    else
    {
        const auto typeNames = FxChain::getTypeNames();
        sectionTitle = "DETAILS  -  SLOT " + juce::String(selectedSlot + 1) + "  ["
                     + typeNames[juce::jlimit(0, typeNames.size() - 1, t)] + "]";
    }
    drawSection(sectionTitle, 20, kDetailY, 1040, GranularColors::accentGrain);

    if (t == FxChain::None)
    {
        g.setColour(GranularColors::textDim.withAlpha(0.55f));
        g.setFont(juce::Font(juce::FontOptions(12.0f)));
        g.drawText("Select an FX type on this slot to edit its parameters.",
                   20, kDetailY + 30, 500, 16, juce::Justification::centredLeft);
    }

    // スロット間の矢印
    g.setColour(GranularColors::textDim.withAlpha(0.55f));
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    for (int i = 0; i < FxChain::kNumSlots - 1; ++i)
    {
        const int x = 20 + kCardW + i * kCardGap;
        g.drawText(">", x, kCardY + kCardH / 2 - 10, kCardGap - kCardW, 20, juce::Justification::centred);
    }
}

void FxPanel::resized()
{
    for (int i = 0; i < FxChain::kNumSlots; ++i)
        cards[(size_t)i]->setBounds(20 + i * kCardGap, kCardY, kCardW, kCardH);

    layoutDetails();
}
