// ==============================================================================
// Source/GUI/KnobModMenu.h
//
// Wavetable 準拠のノブ右クリック Modulation Matrix クイック設定メニュー。
// 各 ValueKnob の右クリックから以下を即座に完結できる:
//   ・割り当て済みスロットのインライン編集 (Amount / UNI / 削除 / MODタブ移動)
//   ・全 12 ソース (LFO 1-4 / ENV 1-3 / Velocity / Note / Mod Wheel / Random / Aftertouch)
//     のワンクリック・トグルアサイン
//   ・Clear All Modulations / Open MOD Tab
//   ・Reset to Default (Ctrl+Click) / Enter Value... (Double-Click)
//
// 既存の Matrix パラメータ (mod1..16 Src/Dst/Amt/Uni) を APVTS 経由で直接操作
// するため、DSP・プリセット・DAW オートメーション・MODタブと 100% 同期する。
// ==============================================================================
#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include "../DSP/ModMatrix.h"
#include "ColorPalette.h"
#include "ValueKnob.h"

namespace GranularUI
{
    // onOpenModTab(srcSubTab, matrixPage):
    //   srcSubTab  : 0=LFO, 1=Env, -1=変更なし
    //   matrixPage : 0=Page1 (Slot 1-8), 1=Page2 (Slot 9-16), -1=変更なし
    using ModTabNavigator = std::function<void(int srcSubTab, int matrixPage)>;

    // ==========================================================================
    // パラメータ ID -> ModMatrix::Dst インデックス (1..77, 未対応は 0)
    // ModMatrix::Dst および PluginProcessor::processBlock の変調適用先と 1 対 1 対応
    // ==========================================================================
    inline int modDestForParam(const juce::String& id)
    {
        using D = ModMatrix;

        // --- Grain / Texture ---
        if (id == "position")          return D::DstPosition;
        if (id == "grainSize")         return D::DstSize;
        if (id == "density")           return D::DstDensity;
        if (id == "positionSpray")     return D::DstSpray;
        if (id == "spraySpeed")        return D::DstSpraySpeed;
        if (id == "pitchJitter")       return D::DstJitter;
        if (id == "panSpread")         return D::DstPanSpread;
        if (id == "speed")             return D::DstSpeed;
        if (id == "tone")              return D::DstTone;

        // --- Pitch ---
        if (id == "octave")            return D::DstOctave;
        if (id == "pitchSt")           return D::DstSemi;
        if (id == "fineTune")          return D::DstFine;

        // --- Filter ---
        if (id == "masterHPF")         return D::DstMasterHpf;
        if (id == "masterLPF")         return D::DstMasterLpf;

        // --- Sample ---
        if (id == "sampleStart")       return D::DstSampleStart;
        if (id == "sampleEnd")         return D::DstSampleEnd;

        // --- Arp ---
        if (id == "arpOctaves")        return D::DstArpOct;
        if (id == "arpRateFree")       return D::DstArpRateFree;
        if (id == "arpGate")           return D::DstArpGate;

        // --- Master / Limiter ---
        if (id == "outGain")           return D::DstOutGain;
        if (id == "fxMix")             return D::DstFxMix;
        if (id == "ceiling")           return D::DstLimCeiling;
        if (id == "limRelease")        return D::DstLimRelease;

        // --- FX Slots ---
        if (id == "fx1Amount")         return D::DstFx1Amount;
        if (id == "fx2Amount")         return D::DstFx2Amount;
        if (id == "fx3Amount")         return D::DstFx3Amount;
        if (id == "fx4Amount")         return D::DstFx4Amount;
        if (id == "fx5Amount")         return D::DstFx5Amount;

        // --- FX Details ---
        if (id == "satDrive")          return D::DstSatDrive;
        if (id == "satPreHz")          return D::DstSatPreHpf;
        if (id == "satTrim")           return D::DstSatTrim;
        if (id == "choRate")           return D::DstChoRate;
        if (id == "choDepth")          return D::DstChoDepth;
        if (id == "choWidth")          return D::DstChoWidth;
        if (id == "dlyFeedback")       return D::DstDlyFeedback;
        if (id == "dlyDuck")           return D::DstDlyDuck;
        if (id == "dlyDamp")           return D::DstDlyDamp;
        if (id == "frzSize")           return D::DstFrzSize;
        if (id == "frzFeedback")       return D::DstFrzFeedback;
        if (id == "frzDamp")           return D::DstFrzDamp;
        if (id == "revDecay")          return D::DstRevDecay;
        if (id == "revShimmer")        return D::DstRevShimmer;
        if (id == "revDamp")           return D::DstRevDamp;
        if (id == "revMod")            return D::DstRevMod;
        if (id == "gateDepth")         return D::DstGateDepth;
        if (id == "gateDecay")         return D::DstGateDecay;
        if (id == "gateVowel")         return D::DstGateVowel;

        // --- LFO / ENV ---
        if (id == "lfo1Rate")          return D::DstLfo1Rate;
        if (id == "lfo2Rate")          return D::DstLfo2Rate;
        if (id == "lfo3Rate")          return D::DstLfo3Rate;
        if (id == "lfo4Rate")          return D::DstLfo4Rate;
        if (id == "env1Attack")        return D::DstEnv1Attack;
        if (id == "env1Decay")         return D::DstEnv1Decay;
        if (id == "env1Sustain")       return D::DstEnv1Sustain;
        if (id == "env1Release")       return D::DstEnv1Release;
        if (id == "env2Attack")        return D::DstEnv2Attack;
        if (id == "env2Decay")         return D::DstEnv2Decay;
        if (id == "env2Sustain")       return D::DstEnv2Sustain;
        if (id == "env2Release")       return D::DstEnv2Release;
        if (id == "env3Attack")        return D::DstEnv3Attack;
        if (id == "env3Decay")         return D::DstEnv3Decay;
        if (id == "env3Sustain")       return D::DstEnv3Sustain;
        if (id == "env3Release")       return D::DstEnv3Release;

        // --- Phase 4: Engine / Atlas / Swarm / Per-Grain SVF / Modal ---
        if (id == "formantShift")      return D::DstFormantShift;
        if (id == "timbreScrub")       return D::DstTimbreScrub;
        if (id == "timbreAmt")         return D::DstTimbreAmt;
        if (id == "atlasPitchLock")    return D::DstAtlasPitchLock;
        if (id == "swarmMix")          return D::DstSwarmMix;
        if (id == "grainSkew")         return D::DstGrainSkew;
        if (id == "grainChirp")        return D::DstGrainChirp;
        if (id == "grainFilterFreq")   return D::DstGrainFilterFreq;
        if (id == "grainFilterRes")    return D::DstGrainFilterRes;
        if (id == "grainFilterJitter") return D::DstGrainFilterJitter;
        if (id == "modalMix")          return D::DstModalMix;
        if (id == "modalDecay")        return D::DstModalDecay;
        if (id == "modalDamp")         return D::DstModalDamp;
        if (id == "modalInharm")       return D::DstModalInharm;

        return D::DstNone;
    }

    inline juce::String matrixSourceName(int srcIdx)
    {
        static const auto names = ModMatrix::getSourceNames();
        if (srcIdx >= 0 && srcIdx < names.size()) return names[srcIdx];
        return "None";
    }

    inline juce::String matrixDestName(int destIdx)
    {
        static const auto names = ModMatrix::getDestNames();
        if (destIdx >= 0 && destIdx < names.size()) return names[destIdx];
        return "None";
    }

    // ソース番号に対応する MOD パネル内のサブタブ (0=LFO, 1=Env, -1=変更なし)
    inline int modSourceSubTabForSource(int srcIdx) noexcept
    {
        if (srcIdx >= ModMatrix::SrcLfo1 && srcIdx <= ModMatrix::SrcLfo4) return 0;
        if (srcIdx >= ModMatrix::SrcEnv1 && srcIdx <= ModMatrix::SrcEnv3) return 1;
        return -1;
    }

    // ==========================================================================
    // Matrix スロット操作ヘルパー (APVTS 経由・DAW オートメーション整合)
    // ==========================================================================
    namespace ModMatrixOps
    {
        struct AssignedSlot
        {
            int   slot     = -1; // 0-based (0..15 -> mod1..mod16)
            int   srcIdx   = 0;
            float amount   = 0.0f;
            bool  unipolar = false;
        };

        inline void setParamDenorm(juce::AudioProcessorValueTreeState& apvts,
                                   const juce::String& paramID, float denormValue)
        {
            if (auto* p = apvts.getParameter(paramID))
            {
                const float norm = juce::jlimit(0.0f, 1.0f, p->convertTo0to1(denormValue));
                p->beginChangeGesture();
                p->setValueNotifyingHost(norm);
                p->endChangeGesture();
            }
        }

        inline int readIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return juce::roundToInt(raw->load(std::memory_order_relaxed));
            return 0;
        }

        inline float readFloatParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load(std::memory_order_relaxed);
            return 0.0f;
        }

        inline std::vector<AssignedSlot> findAssignedSlots(juce::AudioProcessorValueTreeState& apvts, int destIdx)
        {
            std::vector<AssignedSlot> result;
            if (destIdx <= 0 || destIdx >= ModMatrix::NumDsts) return result;

            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                const int src  = readIntParam(apvts, "mod" + si + "Src");
                const int dest = readIntParam(apvts, "mod" + si + "Dst");
                if (src > 0 && dest == destIdx)
                {
                    AssignedSlot info;
                    info.slot     = s;
                    info.srcIdx   = src;
                    info.amount   = readFloatParam(apvts, "mod" + si + "Amt");
                    info.unipolar = readFloatParam(apvts, "mod" + si + "Uni") > 0.5f;
                    result.push_back(info);
                }
            }
            return result;
        }

        inline int countUsedSlots(juce::AudioProcessorValueTreeState& apvts)
        {
            int count = 0;
            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                if (readIntParam(apvts, "mod" + si + "Src") > 0
                    && readIntParam(apvts, "mod" + si + "Dst") > 0)
                    ++count;
            }
            return count;
        }

        // 空きスロット検索: 完全空き (src==0 && dst==0) を最優先、次点で片側無効
        inline int findFreeSlot(juce::AudioProcessorValueTreeState& apvts)
        {
            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                if (readIntParam(apvts, "mod" + si + "Src") == 0
                    && readIntParam(apvts, "mod" + si + "Dst") == 0)
                    return s;
            }
            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                if (readIntParam(apvts, "mod" + si + "Src") == 0
                    || readIntParam(apvts, "mod" + si + "Dst") == 0)
                    return s;
            }
            return -1;
        }

        inline void clearSlot(juce::AudioProcessorValueTreeState& apvts, int slot)
        {
            if (slot < 0 || slot >= ModMatrix::kNumSlots) return;
            const juce::String si(slot + 1);
            // 先に Src=0 でオーディオ側のレーンを即無効化してから他を初期化
            setParamDenorm(apvts, "mod" + si + "Src", 0.0f);
            setParamDenorm(apvts, "mod" + si + "Dst", 0.0f);
            setParamDenorm(apvts, "mod" + si + "Amt", 0.0f);
            setParamDenorm(apvts, "mod" + si + "Uni", 0.0f);
        }

        inline void clearPair(juce::AudioProcessorValueTreeState& apvts, int srcIdx, int destIdx)
        {
            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                if (readIntParam(apvts, "mod" + si + "Src") == srcIdx
                    && readIntParam(apvts, "mod" + si + "Dst") == destIdx)
                    clearSlot(apvts, s);
            }
        }

        inline void clearAllForDest(juce::AudioProcessorValueTreeState& apvts, int destIdx)
        {
            for (int s = 0; s < ModMatrix::kNumSlots; ++s)
            {
                const juce::String si(s + 1);
                if (readIntParam(apvts, "mod" + si + "Dst") == destIdx)
                    clearSlot(apvts, s);
            }
        }

        // 新規アサイン: ノブ位置に応じたスマート初期方向 + ソース特性に応じた初期 UNI 設定
        inline int assignSource(juce::AudioProcessorValueTreeState& apvts,
                                int srcIdx, int destIdx, float knobNorm01)
        {
            if (srcIdx <= 0 || srcIdx >= ModMatrix::NumSrcs) return -1;
            if (destIdx <= 0 || destIdx >= ModMatrix::NumDsts) return -1;

            const int slot = findFreeSlot(apvts);
            if (slot < 0) return -1;

            // ノブが上限付近（例: LPF 20kHz 全開）なら下向き -0.45、それ以外は +0.45
            const float initialAmt = (knobNorm01 > 0.75f) ? -0.45f : 0.45f;

            // ENV / Velocity / ModWheel / Aftertouch は直感的な Unipolar をデフォルトに
            const bool defaultUni = (srcIdx >= ModMatrix::SrcEnv1 && srcIdx <= ModMatrix::SrcEnv3)
                                 || srcIdx == ModMatrix::SrcVelocity
                                 || srcIdx == ModMatrix::SrcModWheel
                                 || srcIdx == ModMatrix::SrcAftertouch;

            const juce::String si(slot + 1);

            // 先に Amt / Uni / Dst を整えてから最後に Src をセットし一時グリッチを防ぐ
            setParamDenorm(apvts, "mod" + si + "Amt", initialAmt);
            setParamDenorm(apvts, "mod" + si + "Uni", defaultUni ? 1.0f : 0.0f);
            setParamDenorm(apvts, "mod" + si + "Dst", (float)destIdx);
            setParamDenorm(apvts, "mod" + si + "Src", (float)srcIdx);
            return slot;
        }
    }

    // ==========================================================================
    // インライン編集行: 右クリックメニュー内で Amount / UNI / 削除 / タブ移動を完結
    //   CustomComponent(false) により、スライダードラッグや UNI 切替中も
    //   メニューが閉じず、音を聴きながらリアルタイムに調整できる。
    // ==========================================================================
    class ModSlotMenuRow final : public juce::PopupMenu::CustomComponent
    {
    public:
        ModSlotMenuRow(juce::AudioProcessorValueTreeState& vts,
                       int slotIndex,
                       int srcIndex,
                       ModTabNavigator onJumpToMod)
            : juce::PopupMenu::CustomComponent(false),
              apvts(vts),
              slot(slotIndex),
              srcIdx(srcIndex),
              jumpCallback(std::move(onJumpToMod))
        {
            const juce::String si(slot + 1);

            // 1. ソースバッジ（クリックで MOD タブの該当 LFO/ENV サブタブ & ページへ移動）
            srcBtn.setButtonText("#" + si + " " + matrixSourceName(srcIdx));
            srcBtn.setTooltip("Open MOD tab (" + matrixSourceName(srcIdx) + ")");
            srcBtn.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack.withAlpha(0.85f));
            srcBtn.setColour(juce::TextButton::textColourOffId, GranularColors::pink.brighter(0.2f));
            srcBtn.onClick = [this]
            {
                if (cleared) return;
                auto cb = jumpCallback;
                const int subTab = modSourceSubTabForSource(srcIdx);
                const int page   = (slot >= 8) ? 1 : 0;
                triggerMenuItem(); // 先にメニューを安全に閉じてからタブ切替
                if (cb)
                    juce::MessageManager::callAsync([cb, subTab, page] { cb(subTab, page); });
            };
            addAndMakeVisible(srcBtn);

            // 2. Unipolar トグル (MOD タブの UNI ボタンと APVTS 経由で完全同期)
            uniBtn.setButtonText("UNI");
            uniBtn.setClickingTogglesState(true);
            uniBtn.setTooltip("Toggle Unipolar / Bipolar");
            uniBtn.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
            uniBtn.setColour(juce::TextButton::buttonOnColourId, GranularColors::babyBlue.withAlpha(0.35f));
            uniBtn.setColour(juce::TextButton::textColourOffId, GranularColors::textDim);
            uniBtn.setColour(juce::TextButton::textColourOnId, GranularColors::text);
            addAndMakeVisible(uniBtn);

            // 3. Amount 双方向横スライダー (ArcDialLookAndFeel::drawLinearSlider が中央0描画)
            amtSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            amtSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            amtSlider.setPopupDisplayEnabled(false, false, nullptr);
            amtSlider.setDoubleClickReturnValue(true, 0.0);
            amtSlider.setColour(juce::Slider::trackColourId, GranularColors::pink);
            amtSlider.onValueChange = [this] { updateValueText(); };
            addAndMakeVisible(amtSlider);

            // 4. リアルタイム % 表示ラベル
            valLabel.setJustificationType(juce::Justification::centredRight);
            valLabel.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
            valLabel.setColour(juce::Label::textColourId, GranularColors::text);
            addAndMakeVisible(valLabel);

            // 5. 削除ボタン（メニューを閉じずに即解除し、複数スロット連続削除に対応）
            removeBtn.setButtonText(juce::String::fromUTF8("\xc3\x97")); // ×
            removeBtn.setTooltip("Remove this modulation");
            removeBtn.setColour(juce::TextButton::buttonColourId, GranularColors::knobTrack);
            removeBtn.setColour(juce::TextButton::textColourOffId, GranularColors::rose);
            removeBtn.onClick = [this]
            {
                if (cleared) return;
                cleared = true;
                amtAtt.reset();
                uniAtt.reset();
                ModMatrixOps::clearSlot(apvts, slot);

                srcBtn.setEnabled(false);
                uniBtn.setEnabled(false);
                amtSlider.setEnabled(false);
                removeBtn.setEnabled(false);
                srcBtn.setButtonText("#" + juce::String(slot + 1) + " Cleared");
                valLabel.setText("--", juce::dontSendNotification);
                valLabel.setColour(juce::Label::textColourId, GranularColors::textDim);
                repaint();
            };
            addAndMakeVisible(removeBtn);

            // APVTS アタッチメント（メンバ宣言順によりコンポーネントより先に破棄される）
            amtAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "mod" + si + "Amt", amtSlider);
            uniAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, "mod" + si + "Uni", uniBtn);

            updateValueText();
        }

        ~ModSlotMenuRow() override
        {
            amtAtt.reset();
            uniAtt.reset();
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth  = 368;
            idealHeight = 30;
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced(2.0f, 1.0f);
            g.setColour(cleared ? GranularColors::knobTrack.withAlpha(0.18f)
                                : GranularColors::text.withAlpha(0.05f));
            g.fillRoundedRectangle(b, 5.0f);
            g.setColour(GranularColors::panelLine.withAlpha(cleared ? 0.4f : 0.8f));
            g.drawRoundedRectangle(b.reduced(0.5f), 5.0f, 1.0f);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced(6, 4);
            srcBtn.setBounds(r.removeFromLeft(102));
            r.removeFromLeft(5);
            uniBtn.setBounds(r.removeFromLeft(36));
            r.removeFromLeft(6);
            removeBtn.setBounds(r.removeFromRight(22));
            r.removeFromRight(4);
            valLabel.setBounds(r.removeFromRight(42));
            r.removeFromRight(4);
            amtSlider.setBounds(r);
        }

    private:
        void updateValueText()
        {
            if (cleared) return;
            const int pct = juce::roundToInt(amtSlider.getValue() * 100.0);
            const juce::String sign = (pct > 0) ? "+" : "";
            valLabel.setText(sign + juce::String(pct) + "%", juce::dontSendNotification);
        }

        juce::AudioProcessorValueTreeState& apvts;
        int  slot    = -1;
        int  srcIdx  = 0;
        bool cleared = false;
        ModTabNavigator jumpCallback;

        // 注意: Attachment は必ず対象コンポーネントより後に宣言し、先に破棄させる
        juce::TextButton srcBtn;
        juce::TextButton uniBtn;
        juce::Slider     amtSlider;
        juce::Label      valLabel;
        juce::TextButton removeBtn;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amtAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> uniAtt;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSlotMenuRow)
    };

    // ==========================================================================
    // 右クリックメニュー表示エントリポイント
    // ==========================================================================
    namespace KnobModMenu
    {
        enum ActionId
        {
            kAssignSrcBase = 100, // 100 + srcIdx (1..12)
            kClearAll      = 200,
            kOpenModTab    = 201,
            kResetDefault  = 202,
            kEnterValue    = 203,
            kDisabledInfo  = 299,
            kSlotRowBase   = 300  // 300 + slot (0..15) : CustomComponent 用
        };

        inline void showForKnob(ValueKnob& knob,
                                juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& paramID,
                                const juce::String& fallbackLabel,
                                ModTabNavigator onOpenModTab)
        {
            const int destIdx = modDestForParam(paramID);
            const juce::String title = (destIdx > 0)
                ? matrixDestName(destIdx)
                : (fallbackLabel.isNotEmpty() ? fallbackLabel : paramID);

            juce::PopupMenu menu;
            menu.setLookAndFeel(&knob.getLookAndFeel());

            if (destIdx > 0)
            {
                const auto assigned   = ModMatrixOps::findAssignedSlots(apvts, destIdx);
                const int  usedSlots  = ModMatrixOps::countUsedSlots(apvts);
                const bool matrixFull = (ModMatrixOps::findFreeSlot(apvts) < 0);

                // --- ヘッダー ---
                juce::String header = title;
                if (!assigned.empty())
                    header += "  (" + juce::String((int)assigned.size()) + " assigned)";
                menu.addSectionHeader(header);

                // --- 1. 割り当て済みスロットのインライン編集行 ---
                if (!assigned.empty())
                {
                    for (const auto& a : assigned)
                    {
                        auto row = std::make_unique<ModSlotMenuRow>(apvts, a.slot, a.srcIdx, onOpenModTab);
                        const juce::String itemTitle = "Slot " + juce::String(a.slot + 1) + ": " + matrixSourceName(a.srcIdx);
                        menu.addCustomItem(kSlotRowBase + a.slot, std::move(row), nullptr, itemTitle);
                    }
                    menu.addSeparator();
                }

                // ソース -> 割り当て済みスロット番号 の早見表
                int slotOfSrc[ModMatrix::NumSrcs];
                for (int i = 0; i < ModMatrix::NumSrcs; ++i) slotOfSrc[i] = -1;
                for (const auto& a : assigned)
                    if (a.srcIdx > 0 && a.srcIdx < ModMatrix::NumSrcs && slotOfSrc[a.srcIdx] < 0)
                        slotOfSrc[a.srcIdx] = a.slot;

                auto addSourceItem = [&](juce::PopupMenu& target, int srcIdx)
                {
                    const int existingSlot = slotOfSrc[srcIdx];
                    const bool isAssigned = (existingSlot >= 0);
                    juce::String label = matrixSourceName(srcIdx);
                    if (isAssigned)
                        label += "   [#" + juce::String(existingSlot + 1) + "]";

                    // 未割り当て & マトリクス満杯なら無効化、割り当て済みなら解除可能なので有効
                    const bool enabled = isAssigned || !matrixFull;
                    target.addItem(kAssignSrcBase + srcIdx, label, enabled, isAssigned);
                };

                // --- 2. クイックアサイン (LFO 1-4 / ENV 1-3 / Note & Expression) ---
                menu.addSectionHeader(matrixFull
                    ? "Matrix Full (" + juce::String(usedSlots) + "/" + juce::String(ModMatrix::kNumSlots) + " slots)"
                    : "Modulate with");

                for (int s = ModMatrix::SrcLfo1; s <= ModMatrix::SrcLfo4; ++s) addSourceItem(menu, s);
                menu.addSeparator();
                for (int s = ModMatrix::SrcEnv1; s <= ModMatrix::SrcEnv3; ++s) addSourceItem(menu, s);
                menu.addSeparator();

                {
                    juce::PopupMenu noteSub;
                    for (int s = ModMatrix::SrcVelocity; s <= ModMatrix::SrcAftertouch; ++s)
                        addSourceItem(noteSub, s);
                    menu.addSubMenu("Note / Expression", noteSub);
                }

                menu.addSeparator();
                if (!assigned.empty())
                    menu.addItem(kClearAll, "Clear All Modulations");
                menu.addItem(kOpenModTab, "Open MOD Tab");
            }
            else
            {
                menu.addSectionHeader(title);
                menu.addItem(kDisabledInfo, "(Not a Matrix Destination)", false, false);
            }

            // --- 3. 全ノブ共通操作 ---
            menu.addSeparator();
            menu.addItem(kResetDefault, "Reset to Default   (Ctrl+Click)");
            menu.addItem(kEnterValue,   "Enter Value...     (Double-Click)");

            // コールバックは SafePointer でノブの生存を必ず検証
            juce::Component::SafePointer<ValueKnob> safeKnob(&knob);
            auto* apvtsPtr = &apvts;

            menu.showMenuAsync(
                juce::PopupMenu::Options()
                    .withTargetComponent(&knob)
                    .withMinimumWidth(226),
                [safeKnob, apvtsPtr, destIdx, onOpenModTab](int result)
                {
                    if (safeKnob == nullptr || result <= 0) return;

                    if (result > (int)kAssignSrcBase && result < (int)kAssignSrcBase + (int)ModMatrix::NumSrcs)
                    {
                        const int srcIdx = result - kAssignSrcBase;
                        bool alreadyAssigned = false;
                        for (const auto& a : ModMatrixOps::findAssignedSlots(*apvtsPtr, destIdx))
                        {
                            if (a.srcIdx == srcIdx) { alreadyAssigned = true; break; }
                        }

                        if (alreadyAssigned)
                        {
                            ModMatrixOps::clearPair(*apvtsPtr, srcIdx, destIdx);
                        }
                        else
                        {
                            const auto& range = safeKnob->getNormalisableRange();
                            const float norm01 = (float)range.convertTo0to1(safeKnob->getValue());
                            ModMatrixOps::assignSource(*apvtsPtr, srcIdx, destIdx, norm01);
                        }
                    }
                    else if (result == kClearAll)
                    {
                        ModMatrixOps::clearAllForDest(*apvtsPtr, destIdx);
                    }
                    else if (result == kOpenModTab)
                    {
                        if (onOpenModTab) onOpenModTab(-1, -1);
                    }
                    else if (result == kResetDefault)
                    {
                        safeKnob->resetToDefault();
                    }
                    else if (result == kEnterValue)
                    {
                        safeKnob->showTextEntry();
                    }
                });
        }
    }

    // ==========================================================================
    // パネル側ワンライナー接続ヘルパー:
    //   デフォルト値記録 (Ctrl+Click用) + 右クリック Modulation メニュー接続
    // ==========================================================================
    inline void attachKnobModMenu(ValueKnob& knob,
                                  juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramID,
                                  const juce::String& labelText,
                                  ModTabNavigator onOpenModTab)
    {
        if (auto* prm = apvts.getParameter(paramID))
            knob.getProperties().set("default_value",
                                     (double)prm->getNormalisableRange().convertFrom0to1(prm->getDefaultValue()));

        auto* apvtsPtr = &apvts;
        knob.onRightClick = [apvtsPtr, paramID, labelText, onOpenModTab](ValueKnob& k)
        {
            KnobModMenu::showForKnob(k, *apvtsPtr, paramID, labelText, onOpenModTab);
        };
    }
}
