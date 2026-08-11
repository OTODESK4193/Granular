// ==========================================
// File: ModMatrix.h
// モジュレーションマトリクス（ブロックレート処理）
//
//  Sources : LFO×4 (テンポ同期/フリー, Sine/Tri/Saw/Sqr/S&H/Chaos)
//            ENV×2 (ループ可能AD), Velocity, Note, ModWheel,
//            Random(ノート毎S&H)
//  Slots   : 16 ( Source × Amount(-1..+1) → Destination )
//  Dests   : 主要パラメータ + FX Amount×5
//
//  ※ Pitch系の変調は「量子化前」に加算されるため、
//    どれだけ暴れさせてもQuantizeのKey/Scaleに乗る（本機の核）
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

class ModMatrix
{
public:
    static constexpr int kNumLfos = 4;
    static constexpr int kNumEnvs = 3;
    static constexpr int kNumSlots = 16;

    enum Src
    {
        SrcNone = 0,
        SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4,
        SrcEnv1, SrcEnv2, SrcEnv3,
        SrcVelocity, SrcNote, SrcModWheel, SrcRandom,
        NumSrcs
    };

    enum Dst
    {
        DstNone = 0,
        // Grain
        DstPosition, DstSize, DstDensity, DstSpray, DstSpraySpeed, DstJitter, DstPanSpread, DstSpeed,
        // Pitch
        DstPitch, DstFine, DstTone,
        // Filter
        DstMasterHpf, DstMasterLpf,
        // Sample
        DstSampleStart, DstSampleEnd,
        // Arp
        DstArpOct, DstArpRateFree, DstArpGate,
        // Master
        DstOutGain, DstFxMix, DstLimCeiling, DstLimRelease,
        // FX General
        DstFx1Amount, DstFx2Amount, DstFx3Amount, DstFx4Amount, DstFx5Amount,
        // Saturation
        DstSatDrive, DstSatPreHpf, DstSatTrim,
        // Chorus
        DstChoRate, DstChoDepth, DstChoWidth,
        // Delay
        DstDlyFeedback, DstDlyDuck, DstDlyDamp,
        // Freeze
        DstFrzSize, DstFrzFeedback, DstFrzDamp,
        // Reverb
        DstRevDecay, DstRevShimmer, DstRevDamp, DstRevMod,
        // LFO
        DstLfo1Rate, DstLfo2Rate, DstLfo3Rate, DstLfo4Rate,
        // ENV
        DstEnv1Attack, DstEnv1Decay, DstEnv1Sustain, DstEnv1Release,
        DstEnv2Attack, DstEnv2Decay, DstEnv2Sustain, DstEnv2Release,
        DstEnv3Attack, DstEnv3Decay, DstEnv3Sustain, DstEnv3Release,
        NumDsts
    };

    static juce::StringArray getSourceNames()
    {
        return { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 1", "ENV 2", "ENV 3",
                 "Velocity", "Note", "Mod Wheel", "Random" };
    }
    static juce::StringArray getDestNames()
    {
        return {
            "None",
            "Position", "Grain Size", "Density", "Spray", "Spray Speed", "Pitch Jitter", "Pan Spread", "Speed",
            "Pitch (st)", "Fine", "Tone",
            "Master HPF", "Master LPF",
            "Sample Start", "Sample End",
            "Arp Octaves", "Arp Rate (Hz)", "Arp Gate",
            "Out Gain", "FX Mix", "Limiter Ceiling", "Limiter Release",
            "FX1 Amount", "FX2 Amount", "FX3 Amount", "FX4 Amount", "FX5 Amount",
            "Sat Drive", "Sat Pre-HPF", "Sat Trim",
            "Chorus Rate", "Chorus Depth", "Chorus Width",
            "Delay Feedback", "Delay Duck", "Delay Damp",
            "Freeze Size", "Freeze Feedback", "Freeze Damp",
            "Reverb Decay", "Reverb Shimmer", "Reverb Damp", "Reverb Mod",
            "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate", "LFO 4 Rate",
            "ENV 1 Attack", "ENV 1 Decay", "ENV 1 Sustain", "ENV 1 Release",
            "ENV 2 Attack", "ENV 2 Decay", "ENV 2 Sustain", "ENV 2 Release",
            "ENV 3 Attack", "ENV 3 Decay", "ENV 3 Sustain", "ENV 3 Release"
        };
    }
    static juce::StringArray getWaveNames()
    {
        return { "Sine", "Triangle", "Saw", "Square", "S&H", "Chaos" };
    }
    static juce::StringArray getSyncRateNames()
    {
        return { "1/1", "1/2", "1/2T", "1/4", "1/4.", "1/4T",
                 "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T", "1/32" };
    }

    struct Params
    {
        struct Lfo { float rateHz = 1.0f; bool sync = false; int rateSync = 6; int wave = 0; };
        struct Env { float attack = 0.05f; float decay = 0.5f; float sustain = 1.0f; float release = 0.3f; bool loop = false; };
        struct Slot { int src = 0; int dst = 0; float amt = 0.0f; bool uni = false; };

        std::array<Lfo, kNumLfos> lfo;
        std::array<Env, kNumEnvs> env;
        std::array<Slot, kNumSlots> slot;
        double bpm = 120.0;
    };

    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
    }

    void reset()
    {
        for (auto& l : lfoState) l = {};
        for (auto& e : envState) e = {};
        velocity = 0.8f;
        noteNorm = 0.5f;
        modWheel = 0.0f;
        randomSH = 0.5f;
        heldNotes = 0;
        gate = false;
        destAccum.fill(0.0f);
        rangeMin.fill(0.0f);
        rangeMax.fill(0.0f);
    }

    // 入力MIDI(アルペジオ後)からソース値を更新
    void handleMidi(const juce::MidiBuffer& midi)
    {
        for (const auto& m : midi)
        {
            const auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                velocity = msg.getFloatVelocity();
                noteNorm = (float)msg.getNoteNumber() / 127.0f;
                randomSH = rng.nextFloat();
                ++heldNotes;
                gate = true;
                for (auto& e : envState) e.stage = EnvState::Attack; // リトリガー(現在値から)
            }
            else if (msg.isNoteOff())
            {
                if (--heldNotes <= 0) { heldNotes = 0; gate = false; } // 全ノート離鍵→ゲートOFF
            }
            else if (msg.isControllerOfType(1))
            {
                modWheel = (float)msg.getControllerValue() / 127.0f;
            }
        }
    }

    // ブロック毎: ソース値を確定し、スロットを合成して destAccum に集計
    void processBlock(int numSamples, const Params& p)
    {
        static const double beatsTable[13] = {
            4.0, 2.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
            0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };

        float src[NumSrcs] = {};

        // --- LFO ---
        for (int i = 0; i < kNumLfos; ++i)
        {
            const auto& lp = p.lfo[(size_t)i];
            auto& st = lfoState[(size_t)i];

            double freq = (double)lp.rateHz;
            if (lp.sync)
            {
                const double bpm = p.bpm > 1.0 ? p.bpm : 120.0;
                freq = bpm / (60.0 * beatsTable[juce::jlimit(0, 12, lp.rateSync)]);
            }

            src[SrcLfo1 + i] = lfoValue(st, lp.wave);

            // 位相を1ブロック分進める
            const double inc = freq * (double)numSamples / sampleRate;
            st.phase += inc;
            st.phase2 += inc * 1.41421356; // Chaos用
            while (st.phase >= 1.0)
            {
                st.phase -= 1.0;
                st.shValue = rng.nextFloat() * 2.0f - 1.0f; // S&H更新
            }
            while (st.phase2 >= 1.0) st.phase2 -= 1.0;
        }

        // --- ENV (ADSR / ブロックレート、Loop時はA-D循環) ---
        for (int i = 0; i < kNumEnvs; ++i)
        {
            const auto& ep = p.env[(size_t)i];
            auto& st = envState[(size_t)i];
            const float sus = juce::jlimit(0.0f, 1.0f, ep.sustain);

            src[SrcEnv1 + i] = st.value;

            const float blockSec = (float)((double)numSamples / sampleRate);

            // ゲートOFFかつ非Loop → Releaseへ移行（Idle/Release以外から）
            if (!ep.loop && !gate
                && st.stage != EnvState::Idle && st.stage != EnvState::Release)
            {
                st.stage = EnvState::Release;
                st.releaseStart = juce::jmax(0.0001f, st.value);
            }

            switch (st.stage)
            {
            case EnvState::Attack:
                st.value += blockSec / juce::jmax(0.001f, ep.attack);
                if (st.value >= 1.0f) { st.value = 1.0f; st.stage = EnvState::Decay; }
                break;

            case EnvState::Decay:
                if (ep.loop)
                {
                    // Loop: 1→0 まで下げてAttackへ循環（複雑LFO）
                    st.value -= blockSec / juce::jmax(0.001f, ep.decay);
                    if (st.value <= 0.0f) { st.value = 0.0f; st.stage = EnvState::Attack; }
                }
                else
                {
                    // ADSR: 1→Sustainレベルまで下げてSustainへ
                    st.value -= (1.0f - sus) * blockSec / juce::jmax(0.001f, ep.decay);
                    if (st.value <= sus) { st.value = sus; st.stage = EnvState::Sustain; }
                }
                break;

            case EnvState::Sustain:
                st.value = sus;   // ゲート保持中はSustainレベルを維持
                break;

            case EnvState::Release:
                st.value -= st.releaseStart * blockSec / juce::jmax(0.001f, ep.release);
                if (st.value <= 0.0f) { st.value = 0.0f; st.stage = EnvState::Idle; }
                break;

            default: // Idle
                st.value = 0.0f;
                break;
            }
        }

        // --- MIDI ---
        src[SrcVelocity] = velocity;
        src[SrcNote] = noteNorm;
        src[SrcModWheel] = modWheel;
        src[SrcRandom] = randomSH;

        // --- スロット合成 (Uni/Bipolar極性変換 + レンジ算出) ---
        destAccum.fill(0.0f);
        rangeMin.fill(0.0f);
        rangeMax.fill(0.0f);
        for (const auto& s : p.slot)
        {
            if (s.src <= 0 || s.src >= NumSrcs || s.dst <= 0 || s.dst >= NumDsts) continue;
            if (std::abs(s.amt) < 0.0001f) continue;

            const bool srcBip = isBipolarSource(s.src);

            // ソース値を、スロットが選んだ出力極性に合わせて変換
            float v = src[s.src];
            if (s.uni) { if (srcBip) v = (v + 1.0f) * 0.5f; }        // 出力 0..1
            else       { if (!srcBip) v = v * 2.0f - 1.0f; }         // 出力 -1..+1
            destAccum[(size_t)s.dst] += v * s.amt;

            // GUIアーク用: この行き先が取りうるオフセット範囲を集計
            const float lo = s.uni ? 0.0f : -1.0f;
            const float hi = 1.0f;
            const float c1 = lo * s.amt, c2 = hi * s.amt;
            rangeMin[(size_t)s.dst] += juce::jmin(c1, c2);
            rangeMax[(size_t)s.dst] += juce::jmax(c1, c2);
        }
    }

    // 合成済み変調値 (概ね -1..+1、複数スロットで超えることもある)
    float get(int dst) const noexcept
    {
        return destAccum[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)];
    }

    // GUIアーク用: 行き先が取りうる最小/最大オフセット (mod単位)
    float getRangeMin(int dst) const noexcept { return rangeMin[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)]; }
    float getRangeMax(int dst) const noexcept { return rangeMax[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)]; }

    // ソースが本来バイポーラ(±1)か: LFOのみ。ENV/Vel/Note/Wheel/Randomはユニポーラ(0..1)
    static bool isBipolarSource(int s) noexcept { return s >= SrcLfo1 && s <= SrcLfo4; }

private:
    struct LfoState
    {
        double phase = 0.0;
        double phase2 = 0.0;   // Chaos用の非整数比セカンド位相
        float shValue = 0.0f;
    };
    struct EnvState
    {
        enum Stage { Idle, Attack, Decay, Sustain, Release };
        int stage = Idle;
        float value = 0.0f;
        float releaseStart = 1.0f;   // Release開始時のレベル
    };

    float lfoValue(const LfoState& st, int wave) const noexcept
    {
        const float ph = (float)st.phase;
        switch (wave)
        {
        case 0: return std::sin(ph * juce::MathConstants<float>::twoPi);              // Sine
        case 1: return 1.0f - 4.0f * std::abs(ph - 0.5f);                             // Triangle
        case 2: return 2.0f * ph - 1.0f;                                              // Saw
        case 3: return ph < 0.5f ? 1.0f : -1.0f;                                      // Square
        case 4: return st.shValue;                                                    // S&H
        case 5: return (std::sin(ph * juce::MathConstants<float>::twoPi)              // Chaos
                      + std::sin((float)st.phase2 * juce::MathConstants<float>::twoPi)) * 0.5f;
        default: return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    std::array<LfoState, kNumLfos> lfoState;
    std::array<EnvState, kNumEnvs> envState;

    float velocity = 0.8f;
    float noteNorm = 0.5f;
    float modWheel = 0.0f;
    float randomSH = 0.5f;
    int   heldNotes = 0;      // 押下中ノート数 (ゲート判定)
    bool  gate = false;       // heldNotes > 0

    std::array<float, NumDsts> destAccum {};
    std::array<float, NumDsts> rangeMin {};
    std::array<float, NumDsts> rangeMax {};
    juce::Random rng;
};
