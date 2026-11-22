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
#include <atomic>
#include <cmath>

class ModMatrix
{
public:
    static constexpr int kNumLfos   = 4;
    static constexpr int kNumEnvs   = 3;
    static constexpr int kNumSlots  = 16;
    static constexpr int kMaxVoices = 24;

    enum Src
    {
        SrcNone = 0,
        SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4,
        SrcEnv1, SrcEnv2, SrcEnv3,
        SrcVelocity, SrcNote, SrcModWheel, SrcRandom,
        SrcAftertouch,
        NumSrcs
    };

    enum Dst
    {
        DstNone = 0,
        // Grain
        DstPosition, DstSize, DstDensity, DstSpray, DstSpraySpeed, DstJitter, DstPanSpread, DstSpeed, DstTone,
        // Pitch
        DstOctave, DstSemi, DstFine,
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
        // Gate
        DstGateDepth, DstGateDecay, DstGateVowel,
        // Phase 4: Engine, Atlas, Per-Grain Filter/Shape & Modal Resonator
        DstFormantShift, DstTimbreScrub, DstTimbreAmt, DstAtlasPitchLock, DstSwarmMix,
        DstGrainSkew, DstGrainChirp, DstGrainFilterFreq, DstGrainFilterRes, DstGrainFilterJitter,
        DstModalMix, DstModalDecay, DstModalDamp, DstModalInharm,
        NumDsts
    };

    static juce::StringArray getSourceNames()
    {
        return { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "ENV 1", "ENV 2", "ENV 3",
                 "Velocity", "Note", "Mod Wheel", "Random", "Aftertouch" };
    }
    static juce::StringArray getDestNames()
    {
        return {
            "None",
            "Position", "Grain Size", "Density", "Spray", "Spray Speed", "Pitch Jitter", "Pan Spread", "Speed", "Tone",
            "Octave", "Semi", "Fine",
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
            "ENV 3 Attack", "ENV 3 Decay", "ENV 3 Sustain", "ENV 3 Release",
            "Gate Depth", "Gate Decay", "Gate Vowel",
            "Formant Shift", "Timbre Scrub", "Timbre Bias", "Atlas Pitch", "Swarm Mix",
            "Grain Skew", "Grain Chirp", "Grain Filter Cutoff", "Grain Filter Res", "Grain Filter Jitter",
            "Modal Mix", "Modal Decay", "Modal Damp", "Modal Inharm"
        };
    }
    static juce::StringArray getWaveNames()
    {
        return { "Sine", "Triangle", "Saw", "Square", "S&H", "Chaos" };
    }
    static juce::StringArray getSyncRateNames()
    {
        return { "8/1", "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4", "1/4.", "1/4T",
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
        for (int i = 0; i < kNumLfos; ++i)
        {
            lfoState[(size_t)i] = {};
            lfoState[(size_t)i].lx = 1.0 + 0.35 * (double)i;
            lfoState[(size_t)i].ly = -1.0 - 0.25 * (double)i;
            lfoState[(size_t)i].lz = 20.0 + 0.5 * (double)i;
        }
        for (auto& e : envState) e = {};
        for (auto& v : voiceStates) v = {};
        velocity = 0.8f;
        noteNorm = 0.5f;
        modWheel = 0.0f;
        aftertouch = 0.0f;
        randomSH = 0.5f;
        heldNotes = 0;
        gate = false;
        hasLastParams = false;
        cachedLfoSrc.fill(0.0f);
        destAccum.fill(0.0f);
        rangeMin.fill(0.0f);
        rangeMax.fill(0.0f);
        for (size_t i = 0; i < (size_t)NumDsts; ++i)
        {
            guiAccum[i].store(0.0f, std::memory_order_relaxed);
            guiRangeMin[i].store(0.0f, std::memory_order_relaxed);
            guiRangeMax[i].store(0.0f, std::memory_order_relaxed);
        }
    }

    // 入力MIDI(アルペジオ後)からグローバルソース値を更新
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
            else if (msg.isAftertouch())
            {
                aftertouch = (float)msg.getAfterTouchValue() / 127.0f;
                const int n = msg.getNoteNumber();
                for (auto& v : voiceStates)
                    if (v.active && v.noteNumber == n) v.aftertouch = aftertouch;
            }
            else if (msg.isChannelPressure())
            {
                aftertouch = (float)msg.getChannelPressureValue() / 127.0f;
                for (auto& v : voiceStates)
                    if (v.gate) v.aftertouch = aftertouch;
            }
            else if (msg.isControllerOfType(74)) // MPE Slide / CC74
            {
                aftertouch = (float)msg.getControllerValue() / 127.0f;
                for (auto& v : voiceStates)
                    if (v.gate) v.aftertouch = aftertouch;
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                allNotesOff();
            }
        }
    }

    /// Per-voice note-on notification (called by GranularSynth voice allocator).
    void noteOn(int voiceIdx, int midiNote, float vel) noexcept
    {
        if (voiceIdx < 0 || voiceIdx >= kMaxVoices) return;
        auto& v = voiceStates[(size_t)voiceIdx];
        v.active     = true;
        v.gate       = true;
        v.noteNumber = midiNote;
        v.velocity   = juce::jlimit(0.0f, 1.0f, vel);
        v.noteNorm   = juce::jlimit(0.0f, 1.0f, (float)midiNote / 127.0f);
        v.randomSH   = rng.nextFloat();
        v.aftertouch = aftertouch;
        for (auto& e : v.envState)
            e.stage = EnvState::Attack;

        if (hasLastParams)
            evaluateVoice(v, lastParams);
    }

    /// Per-voice note-off notification (starts release stage of per-voice mod envelopes).
    void noteOff(int voiceIdx) noexcept
    {
        if (voiceIdx < 0 || voiceIdx >= kMaxVoices) return;
        auto& v = voiceStates[(size_t)voiceIdx];
        v.gate = false;
        if (hasLastParams)
            evaluateVoice(v, lastParams);
    }

    /// Marks a voice's modulation state inactive once its amplitude envelope finishes.
    void voiceFinished(int voiceIdx) noexcept
    {
        if (voiceIdx < 0 || voiceIdx >= kMaxVoices) return;
        auto& v = voiceStates[(size_t)voiceIdx];
        v.gate = false;
        v.active = false;
        for (auto& e : v.envState) e = {};
    }

    /// Clears all per-voice modulation gates immediately.
    void allNotesOff() noexcept
    {
        for (auto& v : voiceStates)
            v.gate = false;
        heldNotes = 0;
        gate = false;
    }

    // ブロック毎: ソース値を確定し、グローバル＆各ボイスのスロットを合成
    void processBlock(int numSamples, const Params& p)
    {
        lastParams = p;
        hasLastParams = true;

        static const double beatsTable[16] = {
            32.0, 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
            0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };

        std::array<float, NumSrcs> src {};

        // --- LFO ---
        for (int i = 0; i < kNumLfos; ++i)
        {
            const auto& lp = p.lfo[(size_t)i];
            auto& st = lfoState[(size_t)i];

            double freq = (double)lp.rateHz;
            if (lp.sync)
            {
                const double bpm = p.bpm > 1.0 ? p.bpm : 120.0;
                freq = bpm / (60.0 * beatsTable[juce::jlimit(0, 15, lp.rateSync)]);
            }

            const float lfoOut = lfoValue(st, lp.wave);
            src[(size_t)(SrcLfo1 + i)] = lfoOut;
            cachedLfoSrc[(size_t)i]    = lfoOut;

            // 位相を1ブロック分進める
            const double inc = freq * (double)numSamples / sampleRate;
            st.phase += inc;
            while (st.phase >= 1.0)
            {
                st.phase -= 1.0;
                st.shValue = rng.nextFloat() * 2.0f - 1.0f; // S&H更新
            }

            // Chaos波形 (wave==5) は Lorenz アトラクタを RK4 サブステップ積分
            if (lp.wave == 5)
            {
                constexpr double sigma = 10.0;
                constexpr double rho   = 28.0;
                constexpr double beta  = 8.0 / 3.0;

                const double totalDt = juce::jlimit(0.0, 0.5, inc * 1.25);
                const int steps = juce::jlimit(1, 64, (int)std::ceil(totalDt / 0.005));
                const double h = totalDt / (double)steps;

                auto deriv = [&](double x, double y, double z, double& dx, double& dy, double& dz) noexcept
                {
                    dx = sigma * (y - x);
                    dy = x * (rho - z) - y;
                    dz = x * y - beta * z;
                };

                for (int s = 0; s < steps; ++s)
                {
                    double k1x, k1y, k1z, k2x, k2y, k2z, k3x, k3y, k3z, k4x, k4y, k4z;
                    deriv(st.lx, st.ly, st.lz, k1x, k1y, k1z);
                    deriv(st.lx + 0.5 * h * k1x, st.ly + 0.5 * h * k1y, st.lz + 0.5 * h * k1z, k2x, k2y, k2z);
                    deriv(st.lx + 0.5 * h * k2x, st.ly + 0.5 * h * k2y, st.lz + 0.5 * h * k2z, k3x, k3y, k3z);
                    deriv(st.lx + h * k3x, st.ly + h * k3y, st.lz + h * k3z, k4x, k4y, k4z);

                    st.lx += (h / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
                    st.ly += (h / 6.0) * (k1y + 2.0 * k2y + 2.0 * k3y + k4y);
                    st.lz += (h / 6.0) * (k1z + 2.0 * k2z + 2.0 * k3z + k4z);
                }

                if (!std::isfinite(st.lx) || !std::isfinite(st.ly) || !std::isfinite(st.lz)
                    || std::abs(st.lx) > 60.0 || std::abs(st.ly) > 60.0 || std::abs(st.lz) > 80.0)
                {
                    st.lx = 1.0 + 0.35 * (double)i;
                    st.ly = -1.0 - 0.25 * (double)i;
                    st.lz = 20.0 + 0.5 * (double)i;
                }
            }
        }

        const float blockSec = (float)((double)numSamples / juce::jmax(1.0, sampleRate));

        // --- グローバル ENV ---
        advanceEnvs(blockSec, p, envState, gate, src);

        // --- グローバル MIDI ---
        src[SrcVelocity]   = velocity;
        src[SrcNote]       = noteNorm;
        src[SrcModWheel]   = modWheel;
        src[SrcRandom]     = randomSH;
        src[SrcAftertouch] = aftertouch;

        // --- グローバル スロット合成 + GUIレンジ算出 ---
        evaluateSlots(p, src, destAccum, &rangeMin, &rangeMax);

        // --- ポリフォニック・パーボイス ENV & スロット合成 ---
        for (auto& v : voiceStates)
        {
            if (!v.active) continue;

            std::array<float, NumSrcs> vSrc {};
            for (int i = 0; i < kNumLfos; ++i)
                vSrc[(size_t)(SrcLfo1 + i)] = cachedLfoSrc[(size_t)i];

            advanceEnvs(blockSec, p, v.envState, v.gate, vSrc);
            vSrc[SrcVelocity]   = v.velocity;
            vSrc[SrcNote]       = v.noteNorm;
            vSrc[SrcModWheel]   = modWheel;
            vSrc[SrcRandom]     = v.randomSH;
            vSrc[SrcAftertouch] = v.aftertouch;

            evaluateSlots(p, vSrc, v.voiceAccum, nullptr, nullptr);
        }

        for (size_t i = 0; i < (size_t)NumDsts; ++i)
        {
            guiAccum[i].store(destAccum[i], std::memory_order_relaxed);
            guiRangeMin[i].store(rangeMin[i], std::memory_order_relaxed);
            guiRangeMax[i].store(rangeMax[i], std::memory_order_relaxed);
        }
    }

    // 合成済み変調値 (オーディオスレッド・グローバル用)
    float get(int dst) const noexcept
    {
        return destAccum[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)];
    }

    /// Returns the polyphonic per-voice modulation sum for `voiceIdx`, falling back to global if inactive.
    float getForVoice(int voiceIdx, int dst) const noexcept
    {
        const int d = juce::jlimit(0, (int)NumDsts - 1, dst);
        if (voiceIdx >= 0 && voiceIdx < kMaxVoices)
        {
            const auto& v = voiceStates[(size_t)voiceIdx];
            if (v.active)
                return v.voiceAccum[(size_t)d];
        }
        return destAccum[(size_t)d];
    }

    bool isVoiceActive(int voiceIdx) const noexcept
    {
        return (voiceIdx >= 0 && voiceIdx < kMaxVoices) && voiceStates[(size_t)voiceIdx].active;
    }

    // GUIスレッド用: スレッドセーフなアトミックスナップショット
    float getGui(int dst) const noexcept
    {
        return guiAccum[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)].load(std::memory_order_relaxed);
    }

    // GUIアーク用: 行き先が取りうる最小/最大オフセット (mod単位, アトミック)
    float getRangeMin(int dst) const noexcept
    {
        return guiRangeMin[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)].load(std::memory_order_relaxed);
    }
    float getRangeMax(int dst) const noexcept
    {
        return guiRangeMax[(size_t)juce::jlimit(0, (int)NumDsts - 1, dst)].load(std::memory_order_relaxed);
    }

    // ソースが本来バイポーラ(±1)か: LFOのみ。ENV/Vel/Note/Wheel/Randomはユニポーラ(0..1)
    static bool isBipolarSource(int s) noexcept { return s >= SrcLfo1 && s <= SrcLfo4; }

private:
    struct LfoState
    {
        double phase = 0.0;
        double lx = 1.0, ly = -1.0, lz = 20.0; // Lorenzアトラクタ状態 (Chaos用)
        float shValue = 0.0f;
    };
    struct EnvState
    {
        enum Stage { Idle, Attack, Decay, Sustain, Release };
        int stage = Idle;
        float value = 0.0f;
        float releaseStart = 1.0f;   // Release開始時のレベル
    };

    struct VoiceModState
    {
        bool  active = false;
        bool  gate   = false;
        int   noteNumber = -1;
        float velocity   = 0.8f;
        float noteNorm   = 0.5f;
        float randomSH   = 0.5f;
        float aftertouch = 0.0f;
        std::array<EnvState, kNumEnvs> envState {};
        std::array<float, NumDsts> voiceAccum {};
    };

    static void advanceEnvs(float blockSec, const Params& p,
                            std::array<EnvState, kNumEnvs>& envs,
                            bool gateState,
                            std::array<float, NumSrcs>& outSrc) noexcept
    {
        for (int i = 0; i < kNumEnvs; ++i)
        {
            const auto& ep = p.env[(size_t)i];
            auto& st = envs[(size_t)i];
            const float sus = juce::jlimit(0.0f, 1.0f, ep.sustain);

            outSrc[(size_t)(SrcEnv1 + i)] = st.value;

            if (!ep.loop && !gateState
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
                    st.value -= blockSec / juce::jmax(0.001f, ep.decay);
                    if (st.value <= 0.0f) { st.value = 0.0f; st.stage = EnvState::Attack; }
                }
                else
                {
                    st.value -= (1.0f - sus) * blockSec / juce::jmax(0.001f, ep.decay);
                    if (st.value <= sus) { st.value = sus; st.stage = EnvState::Sustain; }
                }
                break;

            case EnvState::Sustain:
                st.value = sus;
                break;

            case EnvState::Release:
                st.value -= st.releaseStart * blockSec / juce::jmax(0.001f, ep.release);
                if (st.value <= 0.0f) { st.value = 0.0f; st.stage = EnvState::Idle; }
                break;

            default:
                st.value = 0.0f;
                break;
            }
        }
    }

    static void evaluateSlots(const Params& p,
                              const std::array<float, NumSrcs>& src,
                              std::array<float, NumDsts>& outAccum,
                              std::array<float, NumDsts>* outMin,
                              std::array<float, NumDsts>* outMax) noexcept
    {
        outAccum.fill(0.0f);
        if (outMin) outMin->fill(0.0f);
        if (outMax) outMax->fill(0.0f);

        for (const auto& s : p.slot)
        {
            if (s.src <= 0 || s.src >= NumSrcs || s.dst <= 0 || s.dst >= NumDsts) continue;
            if (std::abs(s.amt) < 0.0001f) continue;

            const bool srcBip = isBipolarSource(s.src);
            float v = src[(size_t)s.src];
            if (s.uni) { if (srcBip) v = (v + 1.0f) * 0.5f; }
            else       { if (!srcBip) v = v * 2.0f - 1.0f; }
            outAccum[(size_t)s.dst] += v * s.amt;

            if (outMin && outMax)
            {
                const float lo = s.uni ? 0.0f : -1.0f;
                const float hi = 1.0f;
                const float c1 = lo * s.amt, c2 = hi * s.amt;
                (*outMin)[(size_t)s.dst] += juce::jmin(c1, c2);
                (*outMax)[(size_t)s.dst] += juce::jmax(c1, c2);
            }
        }
    }

    void evaluateVoice(VoiceModState& v, const Params& p) noexcept
    {
        std::array<float, NumSrcs> vSrc {};
        for (int i = 0; i < kNumLfos; ++i)
            vSrc[(size_t)(SrcLfo1 + i)] = cachedLfoSrc[(size_t)i];
        for (int i = 0; i < kNumEnvs; ++i)
            vSrc[(size_t)(SrcEnv1 + i)] = v.envState[(size_t)i].value;
        vSrc[SrcVelocity]   = v.velocity;
        vSrc[SrcNote]       = v.noteNorm;
        vSrc[SrcModWheel]   = modWheel;
        vSrc[SrcRandom]     = v.randomSH;
        vSrc[SrcAftertouch] = v.aftertouch;
        evaluateSlots(p, vSrc, v.voiceAccum, nullptr, nullptr);
    }

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
        case 5: return std::tanh((float)(st.lx / 16.0));                              // Lorenz Chaos (-1..+1)
        default: return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    std::array<LfoState, kNumLfos> lfoState;
    std::array<EnvState, kNumEnvs> envState;
    std::array<VoiceModState, kMaxVoices> voiceStates;
    std::array<float, kNumLfos> cachedLfoSrc {};
    Params lastParams;
    bool hasLastParams = false;

    float velocity = 0.8f;
    float noteNorm = 0.5f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float randomSH = 0.5f;
    int   heldNotes = 0;      // 押下中ノート数 (ゲート判定)
    bool  gate = false;       // heldNotes > 0

    std::array<float, NumDsts> destAccum {};
    std::array<float, NumDsts> rangeMin {};
    std::array<float, NumDsts> rangeMax {};
    std::array<std::atomic<float>, NumDsts> guiAccum {};
    std::array<std::atomic<float>, NumDsts> guiRangeMin {};
    std::array<std::atomic<float>, NumDsts> guiRangeMax {};
    juce::Random rng;
};
