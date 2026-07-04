// ==========================================
// File: ModMatrix.h
// モジュレーションマトリクス（ブロックレート処理）
//
//  Sources : LFO×4 (テンポ同期/フリー, Sine/Tri/Saw/Sqr/S&H/Chaos)
//            ENV×2 (ループ可能AD), Velocity, Note, ModWheel,
//            Random(ノート毎S&H), Macro×4
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
    static constexpr int kNumEnvs = 2;
    static constexpr int kNumMacros = 4;
    static constexpr int kNumSlots = 16;

    enum Src
    {
        SrcNone = 0,
        SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4,
        SrcEnv1, SrcEnv2,
        SrcVelocity, SrcNote, SrcModWheel, SrcRandom,
        SrcMacro1, SrcMacro2, SrcMacro3, SrcMacro4,
        NumSrcs
    };

    enum Dst
    {
        DstNone = 0,
        DstPosition, DstSize, DstDensity, DstSpray, DstJitter,
        DstPanSpread, DstSpeed, DstPitch, DstFine, DstTone,
        DstHpf, DstLpf,
        DstFx1, DstFx2, DstFx3, DstFx4, DstFx5,
        NumDsts
    };

    static juce::StringArray getSourceNames()
    {
        return { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 1", "ENV 2",
                 "Velocity", "Note", "Mod Wheel", "Random",
                 "Macro 1", "Macro 2", "Macro 3", "Macro 4" };
    }
    static juce::StringArray getDestNames()
    {
        return { "None", "Position", "Grain Size", "Density", "Spray", "Pitch Jitter",
                 "Pan Spread", "Speed", "Pitch (st)", "Fine", "Tone",
                 "HPF", "LPF",
                 "FX1 Amount", "FX2 Amount", "FX3 Amount", "FX4 Amount", "FX5 Amount" };
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
        struct Env { float attack = 0.05f; float decay = 0.5f; bool loop = false; };
        struct Slot { int src = 0; int dst = 0; float amt = 0.0f; };

        std::array<Lfo, kNumLfos> lfo;
        std::array<Env, kNumEnvs> env;
        std::array<float, kNumMacros> macro { 0.5f, 0.5f, 0.5f, 0.5f };
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
        destAccum.fill(0.0f);
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
                for (auto& e : envState) e.stage = EnvState::Attack; // リトリガー(現在値から)
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

        // --- ENV (ループ可能AD / ブロックレート) ---
        for (int i = 0; i < kNumEnvs; ++i)
        {
            const auto& ep = p.env[(size_t)i];
            auto& st = envState[(size_t)i];

            src[SrcEnv1 + i] = st.value;

            const float blockSec = (float)((double)numSamples / sampleRate);
            if (st.stage == EnvState::Attack)
            {
                st.value += blockSec / juce::jmax(0.001f, ep.attack);
                if (st.value >= 1.0f) { st.value = 1.0f; st.stage = EnvState::Decay; }
            }
            else if (st.stage == EnvState::Decay)
            {
                st.value -= blockSec / juce::jmax(0.001f, ep.decay);
                if (st.value <= 0.0f)
                {
                    st.value = 0.0f;
                    st.stage = ep.loop ? EnvState::Attack : EnvState::Idle;
                }
            }
        }

        // --- MIDI / Macro ---
        src[SrcVelocity] = velocity;
        src[SrcNote] = noteNorm;
        src[SrcModWheel] = modWheel;
        src[SrcRandom] = randomSH;
        for (int i = 0; i < kNumMacros; ++i)
            src[SrcMacro1 + i] = p.macro[(size_t)i];

        // --- スロット合成 ---
        destAccum.fill(0.0f);
        for (const auto& s : p.slot)
        {
            if (s.src <= 0 || s.src >= NumSrcs || s.dst <= 0 || s.dst >= NumDsts) continue;
            if (std::abs(s.amt) < 0.0001f) continue;
            destAccum[(size_t)s.dst] += src[s.src] * s.amt;
        }
    }

    // 合成済み変調値 (概ね -1..+1、複数スロットで超えることもある)
    float get(int dst) const noexcept
    {
        return destAccum[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)];
    }

private:
    struct LfoState
    {
        double phase = 0.0;
        double phase2 = 0.0;   // Chaos用の非整数比セカンド位相
        float shValue = 0.0f;
    };
    struct EnvState
    {
        enum Stage { Idle, Attack, Decay };
        int stage = Idle;
        float value = 0.0f;
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

    std::array<float, NumDsts> destAccum {};
    juce::Random rng;
};
