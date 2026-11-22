// ==========================================
// File: ModalResonator.h
// Grain-Excited Scale-Tuned Modal Resonator Bank
//
//  8基のステレオ TPT SVF バンドパス共振器（Zavalishin原型）。
//  T60減衰時間・周波数依存ダンピング・剛性非調和分散 (sqrt(1 + B*m^2)) を
//  全サンプルレート (44.1k〜192kHz) で厳密に補償する。
//  ratioMode=0 (Scale) では ScaleQuantizer に厳密調律された和音共鳴を生成。
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include "ScaleQuantizer.h"

class ModalResonatorBank
{
public:
    static constexpr int kNumModes = 8;

    enum RatioMode
    {
        ModeScale = 0,
        ModeHarmonic,
        ModeMarimba,
        ModeBell,
        ModePlate,
        NumRatioModes
    };

    static juce::StringArray getRatioModeNames()
    {
        return { "Scale", "Harmonic", "Marimba", "Bell", "Plate" };
    }

    struct Params
    {
        float mix = 0.0f;         // 0..1
        float decaySec = 0.8f;    // 0.05..5.0 sec
        float damp = 0.4f;        // 0..1 (高次モード減衰)
        float inharm = 0.0f;      // 0..1 (剛性B非調和分散)
        int   ratioMode = 0;      // RatioMode (0..4)
        float baseMidi = 60.0f;   // 励起基準MIDIノート
        int   scaleRoot = 0;      // 0..11
        int   scaleIdx = 0;       // ScaleQuantizer index (0..81)
    };

    void prepare(double sr) noexcept
    {
        sampleRate = sr > 1000.0 ? sr : 44100.0;
        reset();
    }

    void reset() noexcept
    {
        for (auto& m : modes)
        {
            m.ic1L = m.ic2L = m.ic1R = m.ic2R = 0.0f;
            m.curG = 0.0f;
            m.curK = 0.5f;
        }
        resEnv = 0.0f;
        activeTail = false;
    }

    /// Updates modal resonator coefficients once per sub-block and processes `buf` in-place.
    void processBlock(juce::AudioBuffer<float>& buf, const Params& p) noexcept
    {
        const int numSamples = buf.getNumSamples();
        const int numCh      = buf.getNumChannels();
        if (numSamples <= 0 || numCh <= 0) return;

        const float mix = juce::jlimit(0.0f, 1.0f, p.mix);
        if (mix <= 1.0e-4f && !activeTail)
            return;

        updateTargets(p);

        const float sr = (float)sampleRate;
        const float coeffGlide = 1.0f - std::exp(-1.0f / (0.015f * sr));
        const float envAtk     = 1.0f - std::exp(-1.0f / (0.004f * sr));
        const float envRel     = 1.0f - std::exp(-1.0f / (0.120f * sr));
        const float dampGain = juce::jlimit(0.0f, 1.0f, mix);
        const float dryScale = 1.0f - 0.50f * dampGain;

        float* chL = buf.getWritePointer(0);
        float* chR = (numCh > 1) ? buf.getWritePointer(1) : nullptr;

        float peakEnergy = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float inL = chL[i];
            const float inR = (chR != nullptr) ? chR[i] : inL;

            float resL = 0.0f;
            float resR = 0.0f;

            for (int m = 0; m < kNumModes; ++m)
            {
                auto& mode = modes[(size_t)m];
                if (!mode.enabled) continue;

                mode.curG += coeffGlide * (mode.targetG - mode.curG);
                mode.curK += coeffGlide * (mode.targetK - mode.curK);

                const float g  = mode.curG;
                const float k  = mode.curK;
                const float a1 = 1.0f / (1.0f + g * (g + k));
                const float a2 = g * a1;
                const float a3 = g * a2;

                // Left resonator
                const float v3L = inL - mode.ic2L;
                const float v1L = a1 * mode.ic1L + a2 * v3L;
                const float v2L = mode.ic2L + a2 * mode.ic1L + a3 * v3L;
                mode.ic1L = flushDenorm(2.0f * v1L - mode.ic1L);
                mode.ic2L = flushDenorm(2.0f * v2L - mode.ic2L);

                // Right resonator (subtle stereo modal detune & alternate panning)
                const float gR  = g * mode.stereoDetuneR;
                const float a1R = 1.0f / (1.0f + gR * (gR + k));
                const float a2R = gR * a1R;
                const float a3R = gR * a2R;
                const float v3R = inR - mode.ic2R;
                const float v1R = a1R * mode.ic1R + a2R * v3R;
                const float v2R = mode.ic2R + a2R * mode.ic1R + a3R * v3R;
                mode.ic1R = flushDenorm(2.0f * v1R - mode.ic1R);
                mode.ic2R = flushDenorm(2.0f * v2R - mode.ic2R);

                resL += v1L * mode.gainL;
                resR += v1R * mode.gainR;
            }

            // 持続音励起での共振飽和・ハウリングを防ぐ自動レベル制御 + ソフトリミット
            // （インパルス励起時は resEnv < 0.45 のため 1.0x 無圧縮で透明に響く）
            const float resMag = juce::jmax(std::abs(resL), std::abs(resR));
            resEnv = flushDenorm(resEnv + (resMag > resEnv ? envAtk : envRel) * (resMag - resEnv));
            const float autoGain = (resEnv > 0.45f) ? (0.45f / resEnv) : 1.0f;

            const float satL = 0.90f * std::tanh(resL * autoGain * 0.90f);
            const float satR = 0.90f * std::tanh(resR * autoGain * 0.90f);
            peakEnergy = juce::jmax(peakEnergy, std::abs(satL) + std::abs(satR));

            if (chR != nullptr)
            {
                chL[i] = inL * dryScale + satL * dampGain;
                chR[i] = inR * dryScale + satR * dampGain;
            }
            else
            {
                chL[i] = inL * dryScale + 0.5f * (satL + satR) * dampGain;
            }
        }

        activeTail = (mix > 1.0e-4f) || (peakEnergy > 1.0e-5f);
        if (!activeTail)
            reset();
    }

    // Exposed for deterministic testing of computed modal frequencies (Hz)
    std::array<float, kNumModes> getCurrentModeFreqsHz() const noexcept
    {
        std::array<float, kNumModes> out {};
        for (int i = 0; i < kNumModes; ++i)
            out[(size_t)i] = modes[(size_t)i].lastFreqHz;
        return out;
    }

private:
    struct ModeState
    {
        float ic1L = 0.0f, ic2L = 0.0f;
        float ic1R = 0.0f, ic2R = 0.0f;
        float curG = 0.0f;
        float curK = 0.5f;
        float targetG = 0.0f;
        float targetK = 0.5f;
        float gainL = 0.0f;
        float gainR = 0.0f;
        float stereoDetuneR = 1.0f;
        float lastFreqHz = 0.0f;
        bool  enabled = false;
    };

    static float flushDenorm(float x) noexcept
    {
        return (std::abs(x) < 1.0e-18f) ? 0.0f : x;
    }

    void updateTargets(const Params& p) noexcept
    {
        static constexpr float kHarmonicRatios[kNumModes] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
        static constexpr float kMarimbaRatios[kNumModes]  = { 1.0f, 2.756f, 5.404f, 8.93f, 13.34f, 18.64f, 24.8f, 31.8f };
        static constexpr float kBellRatios[kNumModes]     = { 0.5f, 1.0f, 1.2f, 1.5f, 2.0f, 2.756f, 3.0f, 4.07f };
        static constexpr float kPlateRatios[kNumModes]    = { 1.0f, 1.593f, 2.135f, 2.653f, 3.155f, 3.600f, 4.180f, 4.820f };

        const float clampedBaseMidi = juce::jlimit(24.0f, 84.0f, p.baseMidi);
        const float baseHz = 440.0f * std::pow(2.0f, (clampedBaseMidi - 69.0f) / 12.0f);
        const float maxHz  = (float)sampleRate * 0.42f;
        const float inharmB = juce::jlimit(0.0f, 1.0f, p.inharm) * 0.25f;
        const float decaySec = juce::jlimit(0.05f, 5.0f, p.decaySec);
        const float damp = juce::jlimit(0.0f, 1.0f, p.damp);

        // Compute scale-tuned MIDI targets for ModeScale
        // If Chromatic (0) is selected, fall back to Octaves & Fifths (12) so 8 stacked modes form an open chord instead of a chromatic cluster.
        std::array<float, kNumModes> scaleFreqsHz {};
        if (p.ratioMode == ModeScale)
        {
            const int effScaleIdx = (p.scaleIdx == 0) ? 12 : juce::jlimit(0, ScaleQuantizer::kNumScales - 1, p.scaleIdx);
            const auto& scales = ScaleQuantizer::getScales();
            const auto& sc = scales[(size_t)effScaleIdx];
            const float snappedRoot = ScaleQuantizer::quantize(clampedBaseMidi, p.scaleRoot, effScaleIdx);
            int filled = 0;
            const int degCount = juce::jmax(1, sc.size);
            const int rootPc = ((p.scaleRoot % 12) + 12) % 12;
            const int startOct = (int)std::floor(snappedRoot / 12.0f);

            for (int oct = startOct - 1; oct <= startOct + 7 && filled < kNumModes; ++oct)
            {
                for (int d = 0; d < degCount && filled < kNumModes; ++d)
                {
                    const float midi = (float)(oct * 12 + rootPc + sc.degrees[(size_t)d]);
                    if (midi >= snappedRoot - 0.1f && midi <= 126.0f)
                        scaleFreqsHz[(size_t)filled++] = 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
                }
            }
            while (filled < kNumModes)
            {
                scaleFreqsHz[(size_t)filled] = baseHz * (float)(filled + 1);
                ++filled;
            }
        }

        for (int m = 0; m < kNumModes; ++m)
        {
            auto& mode = modes[(size_t)m];
            float f = baseHz;
            switch (p.ratioMode)
            {
            case ModeScale:    f = scaleFreqsHz[(size_t)m]; break;
            case ModeMarimba:  f = baseHz * kMarimbaRatios[m]; break;
            case ModeBell:     f = baseHz * kBellRatios[m]; break;
            case ModePlate:    f = baseHz * kPlateRatios[m]; break;
            case ModeHarmonic:
            default:           f = baseHz * kHarmonicRatios[m]; break;
            }

            // Physical stiffness dispersion: f_m = f_0 * r_m * sqrt(1 + B * m^2)
            const float mf = (float)m;
            f *= std::sqrt(1.0f + inharmB * mf * mf);

            if (f < 25.0f || f > maxHz)
            {
                mode.enabled = false;
                mode.targetG = 0.0f;
                mode.gainL = mode.gainR = 0.0f;
                mode.lastFreqHz = f;
                continue;
            }

            mode.enabled = true;
            mode.lastFreqHz = f;
            mode.targetG = std::tan(juce::MathConstants<float>::pi * f / (float)sampleRate);

            // High-frequency T60 rolloff (Chaigne/Askenfelt internal air + material damping)
            const float tau60 = juce::jmax(0.008f,
                decaySec / (1.0f + 1.2f * damp * std::pow(f / 600.0f, 1.25f)));
            constexpr float ln1000 = 6.907755f;
            const float q = juce::jmax(0.75f, juce::MathConstants<float>::pi * f * tau60 / ln1000);
            mode.targetK = juce::jlimit(0.0004f, 1.4f, 1.0f / q);

            // Equal-energy excitation scaling sqrt(k) + alternating stereo spread
            const float baseAmp = (0.48f / (1.0f + 0.30f * mf)) * std::sqrt(mode.targetK);
            const float pan01 = 0.5f + (((m & 1) == 0) ? -0.25f : 0.25f) * ((float)(m + 1) / (float)kNumModes);
            mode.gainL = baseAmp * std::sqrt(1.0f - pan01);
            mode.gainR = baseAmp * std::sqrt(pan01);
            mode.stereoDetuneR = 1.0f + (((m & 1) == 0) ? 0.0022f : -0.0022f);

            if (mode.curG <= 0.0f)
            {
                mode.curG = mode.targetG;
                mode.curK = mode.targetK;
            }
        }
    }

    double sampleRate = 44100.0;
    std::array<ModeState, kNumModes> modes;
    float resEnv = 0.0f;
    bool activeTail = false;
};
