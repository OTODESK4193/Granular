// ==========================================
// File: FxChain.h
// 4スロット直列FXチェーン（スロット順に適用、GUIからD&Dで並べ替え）
//
// FX5種:
//  - ADAA Saturation : NextGenKick2から移植（アンチエイリアス歪み、アルゴリズム10種）
//  - Ensemble Chorus : Tessellator MirageChorus移植（4ボイス）
//  - Tape Delay      : Tessellator PhantomDelay移植（テンポ同期・ダッキング付き）
//  - Freeze          : Tessellator WarpSmear移植（スペクトル的な止め）
//  - Shimmer Reverb  : Tessellator AbyssReverb移植（16ch FDN＋オクターブシフター）
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>
#include <random>
#include <vector>

namespace gfx
{
    // ------------------------------------------
    // ユーティリティ (Tessellatorから移植)
    // ------------------------------------------
    inline float antiDenormal(float x) noexcept
    {
        return (std::abs(x) < 1.0e-20f) ? 0.0f : x;
    }

    inline float safeLoopSaturate(float x) noexcept
    {
        if (x > 1.0f) return 1.0f + std::tanh((x - 1.0f) * 2.0f) * 0.15f;
        if (x < -1.0f) return -1.0f + std::tanh((x + 1.0f) * 2.0f) * 0.15f;
        return x;
    }

    inline int findNearestPrime(int n)
    {
        auto isPrime = [](int num)
        {
            if (num <= 1) return false;
            if (num <= 3) return true;
            if (num % 2 == 0 || num % 3 == 0) return false;
            for (int i = 5; i * i <= num; i += 6)
                if (num % i == 0 || num % (i + 2) == 0) return false;
            return true;
        };
        if (n <= 2) return 2;
        int up = n;
        while (!isPrime(up)) up++;
        int down = n;
        while (down > 2 && !isPrime(down)) down--;
        return ((up - n) < (n - down)) ? up : down;
    }

    class ChaosLFO
    {
        float phase1 = 0.0f, phase2 = 0.0f, inc1 = 0.0f, inc2 = 0.0f;
    public:
        void setFrequency(float freq, float sampleRate)
        {
            const float safeFreq = (freq > 0.0f) ? freq : 0.0f;
            inc1 = safeFreq / sampleRate;
            inc2 = (safeFreq * 1.41421356f) / sampleRate;
        }
        void setPhase(float p) { phase1 = p; phase2 = p * 1.618f; }
        inline float process() noexcept
        {
            if (inc1 <= 0.0f) return 0.0f;
            phase1 += inc1; if (phase1 >= 1.0f) phase1 -= 1.0f;
            phase2 += inc2; if (phase2 >= 1.0f) phase2 -= 1.0f;
            return (std::sin(phase1 * juce::MathConstants<float>::twoPi)
                  + std::sin(phase2 * juce::MathConstants<float>::twoPi)) * 0.5f;
        }
    };

    // Shimmer用オクターブシフター (SRスケール対応・ヒープ確保)
    class OctaveShifter
    {
        std::vector<float> buffer;
        int bufMask = 0;
        int windowSize = 2048;
        int writeIdx = 0;
        float phase = 0.0f;
    public:
        void prepareToPlay(double sr)
        {
            const int targetWin = juce::jlimit(512, 16384, juce::roundToInt(2048.0 * sr / 44100.0));
            windowSize = targetWin;
            int size = 1024;
            while (size < targetWin * 2) size <<= 1;
            buffer.assign((size_t)size, 0.0f);
            bufMask = size - 1;
            clearState();
        }
        void clearState() noexcept
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIdx = 0;
            phase = 0.0f;
        }
        float process(float in) noexcept
        {
            if (buffer.empty()) return in;
            buffer[(size_t)writeIdx] = in;

            const float winF = (float)windowSize;
            phase += 1.0f / winF;
            if (phase >= 1.0f) phase -= 1.0f;

            float phaseB = phase + 0.5f;
            if (phaseB >= 1.0f) phaseB -= 1.0f;

            const float delayA = (1.0f - phase) * winF;
            const float delayB = (1.0f - phaseB) * winF;

            auto getHermite = [&](float d) noexcept
            {
                const float bufLen = (float)(bufMask + 1);
                float readPos = (float)writeIdx - d;
                while (readPos < 0.0f) readPos += bufLen;
                const int idx1 = ((int)readPos) & bufMask;
                const float frac = readPos - std::floor(readPos);
                const int idx0 = (idx1 - 1) & bufMask;
                const int idx2 = (idx1 + 1) & bufMask;
                const int idx3 = (idx1 + 2) & bufMask;
                const float y0 = buffer[(size_t)idx0];
                const float y1 = buffer[(size_t)idx1];
                const float y2 = buffer[(size_t)idx2];
                const float y3 = buffer[(size_t)idx3];
                const float c0 = y1;
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * frac + c2) * frac + c1) * frac + c0;
            };

            const float outA = getHermite(delayA);
            const float outB = getHermite(delayB);
            const float winA = 0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));
            const float winB = 0.5f * (1.0f - std::cos(phaseB * juce::MathConstants<float>::twoPi));

            writeIdx = (writeIdx + 1) & bufMask;
            return outA * winA + outB * winB;
        }
    };

    struct VelvetNoiseDiffuser
    {
        std::vector<float> buffer;
        int writePos = 0;
        int bufferMask = 0;
        struct Tap { int delay; float gain; };
        std::vector<Tap> taps;
        float amount = 0.0f;

        void prepare(float fs, uint32_t seed = 12345u)
        {
            const float durationMs = 50.0f;
            const float totalSamples = durationMs * fs * 0.001f;
            int size = 1024;
            while ((float)size < totalSamples + 64.0f) size <<= 1;

            buffer.assign((size_t)size, 0.0f);
            bufferMask = size - 1;
            writePos = 0;

            taps.clear();
            const int numTaps = juce::jlimit(48, 128, juce::roundToInt(48.0 * fs / 44100.0));
            const float grid = totalSamples / (float)numTaps;

            std::mt19937 gen(seed);
            std::uniform_real_distribution<float> distOffset(0.0f, juce::jmax(0.25f, grid - 1.0f));
            std::uniform_int_distribution<> sign(0, 1);
            const float normGain = 1.0f / std::sqrt((float)numTaps);

            for (int i = 0; i < numTaps; ++i)
            {
                Tap t;
                t.delay = (int)((float)i * grid + distOffset(gen));
                t.delay = juce::jlimit(1, size - 1, t.delay);
                t.gain = (sign(gen) == 0 ? 1.0f : -1.0f) * normGain;
                taps.push_back(t);
            }
        }
        void setAmount(float a) { amount = a; }
        void clearState() noexcept
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        inline float process(float in) noexcept
        {
            if (buffer.empty()) return in;
            buffer[(size_t)writePos] = in;
            
            float out = 0.0f;
            if (amount >= 0.01f)
            {
                for (const auto& t : taps)
                {
                    const int r = (writePos - t.delay) & bufferMask;
                    out += buffer[(size_t)r] * t.gain;
                }
            }
            
            writePos = (writePos + 1) & bufferMask;
            
            if (amount < 0.01f) return in;
            return in * (1.0f - amount) + out * amount;
        }
    };

    static inline void matrixHadamard(float* x) noexcept
    {
        for (int i = 0; i < 16; i += 2) { const float a = x[i]; const float b = x[i + 1]; x[i] = a + b; x[i + 1] = a - b; }
        for (int i = 0; i < 16; i += 4)
        {
            const float a0 = x[i]; const float b0 = x[i + 2]; x[i] = a0 + b0; x[i + 2] = a0 - b0;
            const float a1 = x[i + 1]; const float b1 = x[i + 3]; x[i + 1] = a1 + b1; x[i + 3] = a1 - b1;
        }
        for (int i = 0; i < 16; i += 8)
            for (int j = 0; j < 4; ++j)
            { const float a = x[i + j]; const float b = x[i + j + 4]; x[i + j] = a + b; x[i + j + 4] = a - b; }
        for (int i = 0; i < 8; ++i) { const float a = x[i]; const float b = x[i + 8]; x[i] = a + b; x[i + 8] = a - b; }
        for (int i = 0; i < 16; ++i) x[i] *= 0.25f;
    }

    // ------------------------------------------
    // Shimmer Reverb (16ch FDN + OctaveShifter + Jot T60)
    // ------------------------------------------
    class ShimmerReverb
    {
        static constexpr int NUM_CHANNELS = 16;
        double sampleRate = 44100.0;
        std::vector<std::vector<float>> delayBuffers;
        std::array<int, NUM_CHANNELS> writePos {};
        std::array<int, NUM_CHANNELS> delayLengths {};
        std::array<float, NUM_CHANNELS> dampStates {};
        std::array<float, NUM_CHANNELS> dcX1 {}, dcY1 {}; // DCブロッカー状態
        std::array<float, NUM_CHANNELS> cachedFbGains {};
        float curAmount = 0.0f;                            // スムージング済みwet量
        float curDecay = 0.7f;
        float cachedDecay = -1.0f;
        float cachedDamp = -1.0f;
        float cachedDampFactor = 0.2f;
        float dcCoef = 0.9975f;
        float amountSmoothCoef = 0.008f;
        std::array<ChaosLFO, NUM_CHANNELS> lfos;

        VelvetNoiseDiffuser velvetL, velvetR;
        OctaveShifter shifter;

    public:
        ShimmerReverb() { delayBuffers.resize(NUM_CHANNELS); }

        void clearState() noexcept
        {
            velvetL.clearState();
            velvetR.clearState();
            shifter.clearState();
            for (int i = 0; i < NUM_CHANNELS; ++i)
            {
                std::fill(delayBuffers[(size_t)i].begin(), delayBuffers[(size_t)i].end(), 0.0f);
                writePos[(size_t)i] = 0;
                dampStates[(size_t)i] = 0.0f;
                dcX1[(size_t)i] = dcY1[(size_t)i] = 0.0f;
                lfos[(size_t)i].setPhase((float)i / (float)NUM_CHANNELS);
            }
            curAmount = 0.0f;
            cachedDecay = -1.0f;
            cachedDamp = -1.0f;
        }

        void prepareToPlay(double sr)
        {
            sampleRate = sr > 1000.0 ? sr : 44100.0;
            // ステレオ非相関のためにL/Rで異なるベロシティノイズシードを設定
            velvetL.prepare((float)sampleRate, 12345u);
            velvetR.prepare((float)sampleRate, 67891u);
            shifter.prepareToPlay(sampleRate);

            dcCoef = std::exp(-juce::MathConstants<float>::twoPi * 15.0f / (float)sampleRate);
            amountSmoothCoef = 1.0f - std::exp(-1.0f / (0.006f * (float)sampleRate));

            // 最大ベース遅延101ms + モジュレーション余裕を含めた200ms確保（192kHzでも省メモリ）
            const int maxDelaySamples = std::max(256, (int)std::ceil(sampleRate * 0.20));

            static const float LFO_RATIOS[16] = {
                1.000f, 0.618f, 1.272f, 0.786f, 1.618f, 0.382f, 1.414f, 0.528f,
                1.175f, 0.854f, 1.324f, 0.472f, 1.089f, 0.927f, 1.236f, 0.691f };
            static const float baseDelaysMs[16] = {
                31.0f, 37.0f, 41.0f, 43.0f, 47.0f, 53.0f, 59.0f, 61.0f,
                67.0f, 71.0f, 73.0f, 79.0f, 83.0f, 89.0f, 97.0f, 101.0f };

            for (int i = 0; i < NUM_CHANNELS; ++i)
            {
                delayBuffers[(size_t)i].assign((size_t)maxDelaySamples, 0.0f);
                const float targetSamps = baseDelaysMs[i] * 0.001f * (float)sampleRate;
                delayLengths[(size_t)i] = findNearestPrime((int)targetSamps);
                if (delayLengths[(size_t)i] > maxDelaySamples - 64) delayLengths[(size_t)i] = maxDelaySamples - 64;
                lfos[(size_t)i].setFrequency(0.5f * LFO_RATIOS[i], (float)sampleRate);
            }
            clearState();
        }

        // amount=Wet量, decay=残響の長さ, shimmer=オクターブ上成分,
        // damp=高域減衰, mod=モジュレーション深さ
        void process(float& inOutL, float& inOutR, float amount,
                     float decay, float shimmer, float damp, float mod) noexcept
        {
            // wet量をSR独立スムージング。amount==0でもFDN内部状態は回し続けてテールフリーズ/復帰クリックを防止。
            curAmount += amountSmoothCoef * (amount - curAmount);
            const float mixAmount = juce::jlimit(0.0f, 1.0f, curAmount);

            const float targetDecay = juce::jlimit(0.0f, 1.0f, decay);
            curDecay += amountSmoothCoef * (targetDecay - curDecay);

            // Jot / Chowning SR独立 T60 フィードバックゲイン計算 (T60 = 0.25s .. 14.25s)
            if (std::abs(curDecay - cachedDecay) > 2.0e-4f || cachedDecay < 0.0f)
            {
                cachedDecay = curDecay;
                const double t60 = 0.25 + (double)(curDecay * curDecay) * 14.0;
                const double logDecayPerSample = -6.907755278982137 / (t60 * sampleRate); // ln(0.001) / (T60 * sr)
                for (int i = 0; i < NUM_CHANNELS; ++i)
                    cachedFbGains[(size_t)i] = juce::jlimit(0.05f, 0.995f,
                        (float)std::exp(logDecayPerSample * (double)delayLengths[(size_t)i]));
            }

            // SR独立ワンポールLPFダンピング係数 (18kHz .. 1.2kHz)
            const float dampClamped = juce::jlimit(0.0f, 1.0f, damp);
            if (std::abs(dampClamped - cachedDamp) > 2.0e-4f || cachedDamp < 0.0f)
            {
                cachedDamp = dampClamped;
                const float dampHz = 18000.0f * std::pow(1200.0f / 18000.0f, dampClamped);
                cachedDampFactor = std::exp(-juce::MathConstants<float>::twoPi * dampHz / (float)sampleRate);
            }
            const float dampFactor = cachedDampFactor;

            velvetL.setAmount(0.45f + mixAmount * 0.45f);
            velvetR.setAmount(0.45f + mixAmount * 0.45f);

            const float vL = velvetL.process(inOutL);
            const float vR = velvetR.process(inOutR);

            const float monoIn = (vL + vR) * 0.5f;
            const float shimmerSig = shifter.process(monoIn);

            const float shimmerMix = shimmer * 0.7f;
            const float injectL = vL + shimmerSig * shimmerMix;
            const float injectR = vR + shimmerSig * shimmerMix;

            float delayOutputs[16] = {};
            float feedbackInputs[16] = {};

            const float lfoDepth = mod * (15.0f * (float)(sampleRate / 44100.0));

            for (int i = 0; i < NUM_CHANNELS; ++i)
            {
                const float modVal = lfos[(size_t)i].process() * lfoDepth;
                float readPos = (float)writePos[(size_t)i] - ((float)delayLengths[(size_t)i] + modVal);
                const int bufSize = (int)delayBuffers[(size_t)i].size();
                if (bufSize == 0) continue;

                while (readPos < 0.0f) readPos += (float)bufSize;
                while (readPos >= (float)bufSize) readPos -= (float)bufSize;

                const int idx1 = (int)readPos;
                const float frac = readPos - (float)idx1;
                int idx2 = idx1 + 1;
                if (idx2 >= bufSize) idx2 = 0;

                const float rawRead = delayBuffers[(size_t)i][(size_t)idx1] * (1.0f - frac)
                                    + delayBuffers[(size_t)i][(size_t)idx2] * frac;

                dampStates[(size_t)i] = rawRead * (1.0f - dampFactor) + dampStates[(size_t)i] * dampFactor;

                // SR補償済み 15Hz DCブロッカー (ナイキストゲイン <= 1.0 厳密正規化)
                const float dcIn = dampStates[(size_t)i];
                const float dcScale = 0.5f * (1.0f + dcCoef);
                const float hp = dcScale * (dcIn - dcX1[(size_t)i]) + dcCoef * dcY1[(size_t)i];
                dcX1[(size_t)i] = dcIn;
                dcY1[(size_t)i] = antiDenormal(hp);

                delayOutputs[i] = hp;
                feedbackInputs[i] = hp;
            }

            matrixHadamard(feedbackInputs);

            float sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < NUM_CHANNELS; ++i)
            {
                const int bufSize = (int)delayBuffers[(size_t)i].size();
                if (bufSize == 0) continue;

                const float fb = cachedFbGains[(size_t)i];
                const float inSig = (i % 2 == 0) ? injectL : injectR;
                float v_n = inSig * (1.0f - fb) + feedbackInputs[i] * fb;
                v_n = antiDenormal(safeLoopSaturate(v_n));

                delayBuffers[(size_t)i][(size_t)writePos[(size_t)i]] = v_n;
                if (++writePos[(size_t)i] >= bufSize) writePos[(size_t)i] = 0;

                if (i < 8) sumL += delayOutputs[i];
                else       sumR += delayOutputs[i];
            }

            if (mixAmount > 1.0e-5f)
            {
                sumL *= 0.25f;
                sumR *= 0.25f;

                auto softClipOutput = [](float x) noexcept
                {
                    const float ax = std::abs(x);
                    if (ax > 1.0f) return x > 0.0f ? 1.0f : -1.0f;
                    return x * (1.5f - 0.5f * x * x);
                };

                inOutL = inOutL * (1.0f - mixAmount) + softClipOutput(sumL) * mixAmount * 0.90f;
                inOutR = inOutR * (1.0f - mixAmount) + softClipOutput(sumR) * mixAmount * 0.90f;
            }
        }
    };

    // ------------------------------------------
    // Ensemble Chorus (4ボイス・ヒープ確保・192kHz対応)
    // ------------------------------------------
    class EnsembleChorus
    {
    public:
        void prepareToPlay(double sr)
        {
            sampleRate = sr > 1000.0 ? sr : 44100.0;
            const int reqHalf = std::max(512, (int)std::ceil(sampleRate * 0.05) + 16);
            delayBuffer.assign((size_t)reqHalf * 2, 0.0f);
            clearState();
        }
        void clearState() noexcept
        {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
            writeIndex = 0;
            lfoPhase1 = lfoPhase2 = 0.0f;
        }

        // amount=Wet量, rate=LFO速度(Hz), depth=揺れ幅, width=ステレオ幅
        void process(float& inOutL, float& inOutR, float amount,
                     float rate, float depth, float width) noexcept
        {
            const int halfSize = (int)(delayBuffer.size() / 2);
            if (halfSize < 16) return;

            const float lfoRate1 = rate;
            const float lfoRate2 = rate * 1.5f;

            lfoPhase1 += lfoRate1 / (float)sampleRate;
            if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
            lfoPhase2 += lfoRate2 / (float)sampleRate;
            if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

            const float mix = amount * 0.5f;
            float outL = 0.0f, outR = 0.0f;

            auto hermite = [](float frac, float y0, float y1, float y2, float y3) noexcept
            {
                const float c0 = y1;
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                return ((c3 * frac + c2) * frac + c1) * frac + c0;
            };

            float mods[4];
            mods[0] = std::sin(lfoPhase1 * juce::MathConstants<float>::twoPi);
            mods[1] = -mods[0];
            mods[2] = std::sin(lfoPhase2 * juce::MathConstants<float>::twoPi);
            mods[3] = -mods[2];

            for (int i = 0; i < 4; ++i)
            {
                const float delayMs = 12.0f + (mods[i] * 5.0f * depth) + ((float)i * 3.0f);
                const float delaySamps = delayMs * (float)(sampleRate / 1000.0);

                float readPos = (float)writeIndex - delaySamps;
                while (readPos < 0.0f) readPos += (float)halfSize;

                int idx1 = (int)readPos;
                if (idx1 >= halfSize) idx1 %= halfSize;
                const float frac = readPos - std::floor(readPos);

                int idx0 = idx1 - 1; if (idx0 < 0) idx0 += halfSize;
                int idx2 = idx1 + 1; if (idx2 >= halfSize) idx2 -= halfSize;
                int idx3 = idx1 + 2; if (idx3 >= halfSize) idx3 -= halfSize;

                const float chL = hermite(frac, delayBuffer[(size_t)idx0 * 2], delayBuffer[(size_t)idx1 * 2],
                                          delayBuffer[(size_t)idx2 * 2], delayBuffer[(size_t)idx3 * 2]);
                const float chR = hermite(frac, delayBuffer[(size_t)idx0 * 2 + 1], delayBuffer[(size_t)idx1 * 2 + 1],
                                          delayBuffer[(size_t)idx2 * 2 + 1], delayBuffer[(size_t)idx3 * 2 + 1]);

                if (i == 0 || i == 2)
                {
                    outL += chL * 0.7f + chR * 0.3f;
                    outR += chR * 0.3f - chL * 0.3f;
                }
                else
                {
                    outR += chR * 0.7f + chL * 0.3f;
                    outL += chL * 0.3f - chR * 0.3f;
                }
            }

            delayBuffer[(size_t)writeIndex * 2] = inOutL;
            delayBuffer[(size_t)writeIndex * 2 + 1] = inOutR;

            if (++writeIndex >= halfSize) writeIndex = 0;

            float wetL = outL * 0.25f;
            float wetR = outR * 0.25f;
            const float mono = (wetL + wetR) * 0.5f;
            wetL = mono + (wetL - mono) * width;
            wetR = mono + (wetR - mono) * width;

            if (amount > 0.0f)
            {
                inOutL = inOutL * (1.0f - mix) + wetL * mix;
                inOutR = inOutR * (1.0f - mix) + wetR * mix;
            }
        }

    private:
        double sampleRate = 44100.0;
        std::vector<float> delayBuffer;
        int writeIndex = 0;
        float lfoPhase1 = 0.0f, lfoPhase2 = 0.0f;
    };

    // ------------------------------------------
    // Tape Delay (テンポ同期・ダッキング付き・192kHzヒープ確保)
    // ------------------------------------------
    class TapeDelay
    {
    public:
        void prepareToPlay(double sr)
        {
            sampleRate = sr > 1000.0 ? sr : 44100.0;
            // 最大4.5秒を確保（192kHzでも2ビート低BPMまで余裕）
            const int reqHalf = std::max(2048, (int)std::ceil(sampleRate * 4.5));
            delayBuffer.assign((size_t)reqHalf * 2, 0.0f);

            amountSmoothCoef = 1.0f - std::exp(-1.0f / (0.015f * (float)sampleRate));
            delaySmoothCoef  = 1.0f - std::exp(-1.0f / (0.020f * (float)sampleRate));
            envAttackCoef    = 1.0f - std::exp(-1.0f / (0.020f * (float)sampleRate));
            envReleaseCoef   = 1.0f - std::exp(-1.0f / (0.120f * (float)sampleRate));
            dcCoef = std::exp(-juce::MathConstants<float>::twoPi * 15.0f / (float)sampleRate);

            clearState();
        }
        void clearState() noexcept
        {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
            writeIndex = 0;
            envelope = 0.0f;
            currentDelaySamps = 0.0f;
            curAmount = 0.0f;
            curFeedback = 0.0f;
            lpStateL = lpStateR = 0.0f;
            dcX1L = dcY1L = dcX1R = dcY1R = 0.0f;
            cachedDamp = -1.0f;
            cachedLpCoef = 0.5f;
        }

        // amount=Wet量, timeBeats=ディレイタイム(拍), feedback=0..0.95,
        // duck=入力ダッキング量, damp=フィードバック高域減衰
        void process(float& inOutL, float& inOutR, float amount, double bpm,
                     float timeBeats, float feedback, float duck, float damp) noexcept
        {
            const int halfSize = (int)(delayBuffer.size() / 2);
            if (halfSize < 64) return;

            curAmount   += amountSmoothCoef * (amount - curAmount);
            curFeedback += amountSmoothCoef * (juce::jlimit(0.0f, 0.95f, feedback) - curFeedback);

            const float inSum = std::abs(inOutL) + std::abs(inOutR);
            if (inSum > envelope) envelope += envAttackCoef * (inSum - envelope);
            else envelope += envReleaseCoef * (inSum - envelope);

            const float duckingGain = 1.0f - juce::jlimit(0.0f, 0.85f, envelope * 4.0f * duck);

            const double effectiveBpm = bpm > 0.0 ? bpm : 120.0;
            const double beatSec = 60.0 / effectiveBpm;

            float targetDelaySamps = (float)(sampleRate * beatSec * (double)timeBeats);
            targetDelaySamps = juce::jlimit(32.0f, (float)(halfSize - 2), targetDelaySamps);

            if (currentDelaySamps == 0.0f) currentDelaySamps = targetDelaySamps;
            currentDelaySamps += delaySmoothCoef * (targetDelaySamps - currentDelaySamps);

            float readPos = (float)writeIndex - currentDelaySamps;
            while (readPos < 0.0f) readPos += (float)halfSize;

            int idx1 = (int)readPos;
            if (idx1 >= halfSize) idx1 %= halfSize;
            const float frac = readPos - std::floor(readPos);

            int idx2 = idx1 + 1;
            if (idx2 >= halfSize) idx2 -= halfSize;

            float dL = delayBuffer[(size_t)idx1 * 2] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2] * frac;
            float dR = delayBuffer[(size_t)idx1 * 2 + 1] * (1.0f - frac) + delayBuffer[(size_t)idx2 * 2 + 1] * frac;

            // DAMP: SR独立カットオフによる高域減衰 (18kHz .. 1kHz)
            const float dampClamped = juce::jlimit(0.0f, 1.0f, damp);
            if (std::abs(dampClamped - cachedDamp) > 2.0e-4f || cachedDamp < 0.0f)
            {
                cachedDamp = dampClamped;
                const float dampHz = 18000.0f * std::pow(1000.0f / 18000.0f, dampClamped);
                cachedLpCoef = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * dampHz / (float)sampleRate);
            }
            const float lpCoef = cachedLpCoef;
            lpStateL += lpCoef * (dL - lpStateL);
            lpStateR += lpCoef * (dR - lpStateR);
            dL = lpStateL;
            dR = lpStateR;

            // SR補償済み 15Hz DCブロッカー (ナイキストゲイン <= 1.0 厳密正規化)
            const float dcScale = 0.5f * (1.0f + dcCoef);
            const float hpL = dcScale * (dL - dcX1L) + dcCoef * dcY1L; dcX1L = dL; dcY1L = antiDenormal(hpL); dL = hpL;
            const float hpR = dcScale * (dR - dcX1R) + dcCoef * dcY1R; dcX1R = dR; dcY1R = antiDenormal(hpR); dR = hpR;

            const float fb = curFeedback;
            delayBuffer[(size_t)writeIndex * 2] =
                gfx::antiDenormal(gfx::safeLoopSaturate((inOutL * duckingGain) + (dL * fb)));
            delayBuffer[(size_t)writeIndex * 2 + 1] =
                gfx::antiDenormal(gfx::safeLoopSaturate((inOutR * duckingGain) + (dR * fb)));

            if (++writeIndex >= halfSize) writeIndex = 0;

            inOutL += dL * curAmount;
            inOutR += dR * curAmount;
        }

    private:
        double sampleRate = 44100.0;
        std::vector<float> delayBuffer;
        int writeIndex = 0;
        float envelope = 0.0f;
        float currentDelaySamps = 0.0f;
        float lpStateL = 0.0f, lpStateR = 0.0f;
        float curAmount = 0.0f;      // スムージング済みWet量
        float curFeedback = 0.0f;    // スムージング済みFeedback
        float dcX1L = 0.0f, dcY1L = 0.0f, dcX1R = 0.0f, dcY1R = 0.0f; // DCブロッカー
        float amountSmoothCoef = 0.0015f, delaySmoothCoef = 0.0011f;
        float envAttackCoef = 0.001f, envReleaseCoef = 0.0002f;
        float dcCoef = 0.9975f;
        float cachedDamp = -1.0f, cachedLpCoef = 0.5f;
    };

    // ------------------------------------------
    // Freeze / Smear (192kHzヒープ確保 + ループ内ユニティAP拡散 + ループ外Velvetスメア)
    //   ※ 非オールパスFIR(Velvet)をフィードバックループ内に入れると特定周波数で
    //     |H(w)| > 1.0 となりハウリング発振するため、ループ内は厳密に |H(w)|==1.0 の
    //     Schroeder オールパス2段で拡散し、Velvetはループ外のウェット出力に適用する。
    // ------------------------------------------
    class FreezeSmear
    {
        struct AllpassStage
        {
            std::vector<float> buf;
            int idx = 0;

            void prepare(int delaySamps)
            {
                buf.assign((size_t)juce::jmax(16, delaySamps), 0.0f);
                idx = 0;
            }
            void clear() noexcept
            {
                std::fill(buf.begin(), buf.end(), 0.0f);
                idx = 0;
            }
            inline float process(float x, float g) noexcept
            {
                if (buf.empty()) return x;
                const float d = buf[(size_t)idx];
                const float v = antiDenormal(x + g * d);
                buf[(size_t)idx] = v;
                if (++idx >= (int)buf.size()) idx = 0;
                return d - g * v;
            }
        };

    public:
        void prepareToPlay(double sr)
        {
            sampleRate = sr > 1000.0 ? sr : 44100.0;
            // 最大1.25秒確保 (sizeMs上限1000msを192kHzでもカバー)
            const int reqHalf = std::max(1024, (int)std::ceil(sampleRate * 1.25));
            buffer.assign((size_t)reqHalf * 2, 0.0f);

            // ループ内拡散用オールパス (L/R で異なる素数長 -> 豊かなステレオ拡散 & 絶対安定 |H|=1)
            const double srScale = sampleRate / 44100.0;
            apL[0].prepare(findNearestPrime((int)std::round(487.0 * srScale)));
            apL[1].prepare(findNearestPrime((int)std::round(751.0 * srScale)));
            apR[0].prepare(findNearestPrime((int)std::round(541.0 * srScale)));
            apR[1].prepare(findNearestPrime((int)std::round(823.0 * srScale)));

            velvetL.prepare((float)sampleRate, 24681u);
            velvetR.prepare((float)sampleRate, 97531u);
            amountSmoothCoef = 1.0f - std::exp(-1.0f / (0.010f * (float)sampleRate));
            delaySmoothCoef  = 1.0f - std::exp(-1.0f / (0.025f * (float)sampleRate));
            dcCoef = std::exp(-juce::MathConstants<float>::twoPi * 15.0f / (float)sampleRate);

            clearState();
        }
        void clearState() noexcept
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            for (int k = 0; k < 2; ++k) { apL[k].clear(); apR[k].clear(); }
            velvetL.clearState();
            velvetR.clearState();
            writeIndex = 0;
            curAmount = 0.0f;
            curFeedback = 0.0f;
            curDelaySamps = 0.0f;
            lpStateL = lpStateR = 0.0f;
            dcX1L = dcY1L = dcX1R = dcY1R = 0.0f;
            cachedDamp = -1.0f;
            cachedLpCoef = 0.5f;
        }

        // amount=Wet量, sizeMs=ループ長(20..1000ms), feedback=0..0.96, damp=高域減衰
        void process(float& inOutL, float& inOutR, float amount,
                     float sizeMs, float feedback, float damp) noexcept
        {
            const int halfSize = (int)(buffer.size() / 2);
            if (halfSize < 128) return;

            curAmount   += amountSmoothCoef * (juce::jlimit(0.0f, 1.0f, amount) - curAmount);
            curFeedback += amountSmoothCoef * (juce::jlimit(0.0f, 0.96f, feedback) - curFeedback);

            const float targetDelay = juce::jlimit(64.0f, (float)(halfSize - 2),
                                                   (float)(sampleRate * (double)sizeMs * 0.001));
            if (curDelaySamps <= 0.0f) curDelaySamps = targetDelay;
            curDelaySamps += delaySmoothCoef * (targetDelay - curDelaySamps);

            float readPos = (float)writeIndex - curDelaySamps;
            while (readPos < 0.0f) readPos += (float)halfSize;
            int idx1 = (int)readPos;
            if (idx1 >= halfSize) idx1 %= halfSize;
            const float frac = readPos - std::floor(readPos);
            int idx2 = idx1 + 1;
            if (idx2 >= halfSize) idx2 -= halfSize;

            float smL = buffer[(size_t)idx1 * 2]     * (1.0f - frac) + buffer[(size_t)idx2 * 2]     * frac;
            float smR = buffer[(size_t)idx1 * 2 + 1] * (1.0f - frac) + buffer[(size_t)idx2 * 2 + 1] * frac;

            // SR独立ループ内LPF減衰 (18kHz .. 1kHz)
            const float dampClamped = juce::jlimit(0.0f, 1.0f, damp);
            if (std::abs(dampClamped - cachedDamp) > 2.0e-4f || cachedDamp < 0.0f)
            {
                cachedDamp = dampClamped;
                const float dampHz = 18000.0f * std::pow(1000.0f / 18000.0f, dampClamped);
                cachedLpCoef = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * dampHz / (float)sampleRate);
            }
            const float lpCoef = cachedLpCoef;
            lpStateL += lpCoef * (smL - lpStateL);
            lpStateR += lpCoef * (smR - lpStateR);
            smL = lpStateL;
            smR = lpStateR;

            // SR補償済み 15Hz DCブロッカー (ナイキストゲイン <= 1.0 厳密正規化)
            const float dcScale = 0.5f * (1.0f + dcCoef);
            const float hpL = dcScale * (smL - dcX1L) + dcCoef * dcY1L; dcX1L = smL; dcY1L = antiDenormal(hpL); smL = hpL;
            const float hpR = dcScale * (smR - dcX1R) + dcCoef * dcY1R; dcX1R = smR; dcY1R = antiDenormal(hpR); smR = hpR;

            // ループ内オールパス2段拡散 (全周波数で厳密 |H(w)| == 1.0 -> ハウリング不可)
            smL = apL[1].process(apL[0].process(smL, 0.45f), 0.42f);
            smR = apR[1].process(apR[0].process(smR, 0.45f), 0.42f);

            // クロスフェード蓄積器: in*(1-fb) + loop*fb (受動的・絶対安定)
            const float fb = curFeedback;
            buffer[(size_t)writeIndex * 2] =
                gfx::antiDenormal(gfx::safeLoopSaturate(inOutL * (1.0f - fb) + smL * fb));
            buffer[(size_t)writeIndex * 2 + 1] =
                gfx::antiDenormal(gfx::safeLoopSaturate(inOutR * (1.0f - fb) + smR * fb));

            if (++writeIndex >= halfSize) writeIndex = 0;

            // Velvet Noise ディフューザーはループ外の出力段で適用（フィードフォワードのみ -> 発振しない）
            const float smearAmt = 0.45f + dampClamped * 0.25f;
            velvetL.setAmount(smearAmt);
            velvetR.setAmount(smearAmt);
            const float wetL = velvetL.process(smL);
            const float wetR = velvetR.process(smR);

            if (curAmount > 1.0e-5f)
            {
                inOutL = inOutL * (1.0f - curAmount) + wetL * curAmount;
                inOutR = inOutR * (1.0f - curAmount) + wetR * curAmount;
            }
        }

    private:
        double sampleRate = 44100.0;
        std::vector<float> buffer;
        AllpassStage apL[2], apR[2];
        VelvetNoiseDiffuser velvetL, velvetR;
        int writeIndex = 0;
        float curAmount = 0.0f;
        float curFeedback = 0.0f;
        float curDelaySamps = 0.0f;
        float amountSmoothCoef = 0.002f;
        float delaySmoothCoef = 0.001f;
        float lpStateL = 0.0f, lpStateR = 0.0f;
        float dcX1L = 0.0f, dcY1L = 0.0f, dcX1R = 0.0f, dcY1R = 0.0f;
        float dcCoef = 0.9975f;
        float cachedDamp = -1.0f, cachedLpCoef = 0.5f;
    };

    // ------------------------------------------
    // ADAA Saturation (NextGenKick2から移植)
    // ------------------------------------------
    struct SaturationState
    {
        float tapeHysteresis = 0.0f;
        float lastX = 0.0f;
        float lastF = 0.0f;
        bool  active = false;

        void reset() noexcept { tapeHysteresis = 0.0f; lastX = 0.0f; lastF = 0.0f; active = false; }
    };

    inline float calcADAAFunc(float x, int type) noexcept
    {
        switch (type)
        {
        case 0: // Soft Tanh
            if (std::abs(x) > 10.0f) return std::abs(x) - 0.693147f;
            return std::log(std::cosh(x));
        case 1: // Hard Clip
            if (x < -1.0f) return -x - 0.5f;
            if (x > 1.0f)  return  x - 0.5f;
            return 0.5f * x * x;
        case 6: // BJT (Atan based)
        {
            const float k = 2.2f;
            const float scale = 0.58f;
            const float term1 = x * std::atan(k * x);
            const float term2 = (0.5f / k) * std::log(1.0f + k * k * x * x);
            return scale * (term1 - term2);
        }
        case 7: // Wavefold
            return -1.0f / juce::MathConstants<float>::pi * std::cos(x * juce::MathConstants<float>::pi);
        case 10: // Cubic
            return (0.5f * x * x) - (x * x * x * x * 0.08333333f);
        default: return 0.0f;
        }
    }

    inline float processSaturationSampleADAA(float x, int type, float drive, SaturationState& state) noexcept
    {
        if (drive <= 1.001f)
        {
            state.active = false;
            state.lastX = x;
            return x;
        }

        // Tape (履歴を使う特殊処理)
        if (type == 3)
        {
            const float g = x * drive;
            const float y = 0.92f * std::tanh(g + 0.08f * state.tapeHysteresis);
            state.tapeHysteresis = y;
            return y;
        }

        // ADAAを使わない静的カーブ群 (原典どおり Cubic=10 は有界版の静的カーブ)
        if (type == 2 || type == 4 || type == 5 || type == 8 || type == 9 || type == 10)
        {
            const float g = x * drive;
            switch (type)
            {
            case 2: { const float b = 0.25f; return std::tanh(g + b) - std::tanh(b); }        // Triode
            case 4: return g / (1.0f + 0.45f * std::abs(g));                                  // Transformer
            case 5: return (std::abs(g) < 1.0f) ? g - (g * g * g) / 3.0f
                                                : (g > 0 ? (2.0f / 3.0f) : -(2.0f / 3.0f));   // JFET
            case 8: { const float step = 1.0f / (1.0f + (25.0f - drive)); return std::round(g / step) * step; }
            case 9: { const float s = std::tanh(g); return s + 0.3f * (std::tanh(3.0f * g) - s); } // Exciter
            case 10: { // Cubic soft clip: 有界・単調（高ドライブでも発散しない）
                const float c = juce::jlimit(-1.0f, 1.0f, g);
                return 1.5f * c - 0.5f * c * c * c;
            }
            default: return g;
            }
        }

        // ADAA (1次アンチエイリアス) を使う群 (0,1,6,7)
        const float g = x * drive;

        auto direct = [type](float v) noexcept
        {
            switch (type)
            {
            case 0: return std::tanh(v);
            case 1: return juce::jlimit(-1.0f, 1.0f, v);
            case 6: return std::atan(v * 2.2f) * 0.58f;
            case 7: return std::sin(v * juce::MathConstants<float>::pi);
            case 10: return v - (v * v * v) / 3.1f;
            default: return v;
            }
        };

        if (!state.active)
        {
            const float prevG = state.lastX * drive;
            state.active = true;
            state.lastX = g;
            state.lastF = calcADAAFunc(g, type);
            return direct(0.5f * (g + prevG));
        }

        const float Fx = calcADAAFunc(g, type);
        const float delta = g - state.lastX;

        float output;
        if (std::abs(delta) < 1.0e-5f)
            output = direct(0.5f * (g + state.lastX));
        else
            output = (Fx - state.lastF) / delta;

        state.lastX = g;
        state.lastF = Fx;
        return output;
    }

    // ------------------------------------------
    // SPECTRA8 移植: Formant Gate 用 SVF & softClip
    // ------------------------------------------
    struct SvfCoeffs
    {
        float g = 0.0f, k = 0.2f, a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;
        void update(float fcNorm, float Q) noexcept
        {
            g = std::tan(juce::MathConstants<float>::pi
                       * juce::jlimit(0.0005f, 0.49f, fcNorm));
            k = 1.0f / juce::jmax(0.3f, Q);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
    };

    struct Svf
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        void reset() noexcept { ic1 = ic2 = 0.0f; }

        void processWithCoeffs(float in, const SvfCoeffs& c, float& lp, float& bpNorm, float& hp) noexcept
        {
            const float v3 = in - ic2;
            const float v1 = c.a1 * ic1 + c.a2 * v3;
            const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;

            lp = v2;
            bpNorm = c.k * v1; // Q=6等でも中心周波数でピークゲイン1.0(0dB)になる正規化BP
            hp = in - bpNorm - v2;
        }

        void process(float in, float fcNorm, float Q, float& lp, float& bp, float& hp) noexcept
        {
            SvfCoeffs c;
            c.update(fcNorm, Q);
            float bpNorm = 0.0f;
            processWithCoeffs(in, c, lp, bpNorm, hp);
            bp = bpNorm / c.k;
        }
    };

    inline float softClip(float x) noexcept
    {
        if (x < -3.0f) return -1.0f;
        if (x >  3.0f) return  1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // ==========================================
    // SPECTRA8 移植: Formant Rhythm Gate (50 Patterns)
    // ==========================================
    class FormantGate
    {
    public:
        static constexpr int kNumSteps = 16;
        static constexpr int kTotalPatterns = 50;

        static juce::StringArray getRateNames()
        {
            return { "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
        }

        static juce::StringArray getPatternNames()
        {
            return {
                // Category 1: Straight (10)
                "1/2 Beat", "1/4 Beat", "1/8 Beat", "1/16 Beat", "1/32 Beat",
                "Off-Beat 8th", "Off-Beat 16th", "Pulse 25%", "Pulse 75%", "March 4-on-floor",
                // Category 2: Triplet & Swing (8)
                "1/4 Triplet", "1/8 Triplet", "1/16 Triplet", "Shuffle Light", "Shuffle Heavy",
                "Triplet Burst", "Gallop", "Waltz",
                // Category 3: Dotted & Polyrhythm (8)
                "1/4 Dotted", "1/8 Dotted", "1/16 Dotted", "Syncopated Dotted",
                "Polyrhythm 3:4", "Polyrhythm 3:2", "Hemiola", "Tresillo 3-3-2",
                // Category 4: ColorBass & EDM (12)
                "Dubstep Wobble", "Riddim Chug", "Future Bass Pump", "Trance Gate Classic",
                "Sidechain Pump", "Psytrance 16th", "Trap Hihat Roll", "Half-Time Chop",
                "Drum'n'Bass Chop", "UK Garage Bounce", "Bounce Groove", "Stutter Drop",
                // Category 5: Glitch & Complex (12)
                "Euclid 3/8", "Euclid 5/8", "Euclid 5/16", "Euclid 7/16", "Euclid 9/16",
                "Binary Counter", "Fibonacci", "Accelerated Burst", "Decelerated Drop",
                "Micro Stutter", "Chaos Squelch", "Morse Code"
            };
        }

        void prepare(double sr)
        {
            sampleRate = sr > 1000.0 ? sr : 44100.0;
            slewCoeff = (float)(1.0 - std::exp(-1.0 / (sampleRate * 0.0008)));
            vowelGlideCoeff = (float)(1.0 - std::exp(-1.0 / (sampleRate * 0.0035)));
            reset();
        }

        void reset()
        {
            phase = 0.0;
            stepIdx = 0;
            prevStepIdx = 0;
            curEnv = 0.0f;
            for (auto& f : fmtL) f.reset();
            for (auto& f : fmtR) f.reset();
            curF1 = 700.0f; curF2 = 1100.0f; curF3 = 2400.0f;
            cachedF1 = cachedF2 = cachedF3 = -1.0f;
        }

        void setParams(int rateIdx, int patternIdx, float depth01, float decay01, float vowel01) noexcept
        {
            mRate = juce::jlimit(0, 6, rateIdx);
            mPattern = juce::jlimit(0, kTotalPatterns - 1, patternIdx);
            mDepth = juce::jlimit(0.0f, 1.0f, depth01);
            mDecay = juce::jlimit(0.05f, 1.0f, decay01);
            mVowel = juce::jlimit(0.0f, 1.0f, vowel01);
        }

        void process(float& l, float& r, double bpm = 120.0, double ppqPosition = 0.0, bool isPlaying = false) noexcept
        {
            static const double kStepBeats[7] = {
                2.0,            // 1/2
                1.0,            // 1/4
                0.5,            // 1/8
                1.0 / 3.0,      // 1/8T
                0.25,           // 1/16
                1.0 / 6.0,      // 1/16T
                0.125           // 1/32
            };
            const double stepBeats = kStepBeats[juce::jlimit(0, 6, mRate)];
            const double safeBpm = (bpm > 20.0 && bpm < 400.0) ? bpm : 120.0;

            int curStep = 0;
            int nextStep = 0;
            float p = 0.0f;

            if (isPlaying && ppqPosition >= 0.0)
            {
                double stepPos = ppqPosition / stepBeats;
                double intPart = std::floor(stepPos);
                p = (float)(stepPos - intPart);
                curStep = ((int)intPart % kNumSteps + kNumSteps) % kNumSteps;
                nextStep = (curStep + 1) % kNumSteps;
                stepIdx = curStep;
            }
            else
            {
                double stepHz = (safeBpm / 60.0) / stepBeats;
                phase += stepHz / sampleRate;
                while (phase >= 1.0)
                {
                    phase -= 1.0;
                    stepIdx = (stepIdx + 1) % kNumSteps;
                }
                curStep = stepIdx;
                nextStep = (stepIdx + 1) % kNumSteps;
                p = (float)phase;
            }

            if (curStep != prevStepIdx)
                prevStepIdx = curStep;

            const int prevStep = (curStep - 1 + kNumSteps) % kNumSteps;
            const bool prevOn = getPatternStep(mPattern, prevStep);
            const bool curOn = getPatternStep(mPattern, curStep);
            const bool nextOn = getPatternStep(mPattern, nextStep);

            const double stepSec = (60.0 / safeBpm) * stepBeats;
            const float fadeRatio = (float)juce::jlimit(0.015, 0.35, 0.0035 / stepSec);

            const bool isLegato = (mDecay >= 0.95f);
            const float openRatio = isLegato ? 1.0f : juce::jmap(mDecay, 0.05f, 0.95f, 0.15f, 0.90f);
            const float gateLen = juce::jmax(fadeRatio * 2.0f, openRatio);
            const float releaseStart = gateLen - fadeRatio;

            float stepEnv = 0.0f;

            if (curOn)
            {
                const bool tiedFromPrev = prevOn && isLegato;
                const bool tiedToNext = nextOn && isLegato;

                if (p < fadeRatio)
                {
                    if (tiedFromPrev)
                        stepEnv = 1.0f;
                    else
                        stepEnv = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * (p / fadeRatio)));
                }
                else if (p >= releaseStart)
                {
                    if (tiedToNext)
                        stepEnv = 1.0f;
                    else if (p < gateLen)
                    {
                        const float fallP = (p - releaseStart) / fadeRatio;
                        stepEnv = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * juce::jlimit(0.0f, 1.0f, fallP)));
                    }
                    else
                    {
                        stepEnv = 0.0f;
                    }
                }
                else
                {
                    stepEnv = 1.0f;
                }
            }
            else
            {
                stepEnv = 0.0f;
            }

            curEnv += slewCoeff * (stepEnv - curEnv);
            const float gain = 1.0f - mDepth * (1.0f - curEnv);

            if (mVowel > 0.001f)
            {
                static const float kVowels[5][3] = {
                    {  730.0f, 1090.0f, 2440.0f }, // /a/
                    {  270.0f, 2290.0f, 3010.0f }, // /i/
                    {  300.0f,  870.0f, 2240.0f }, // /u/
                    {  530.0f, 1840.0f, 2480.0f }, // /e/
                    {  570.0f,  840.0f, 2410.0f }, // /o/
                };
                const int v = stepIdx % 5;
                const float targetF1 = kVowels[v][0];
                const float targetF2 = kVowels[v][1];
                const float targetF3 = kVowels[v][2];

                curF1 += (targetF1 - curF1) * vowelGlideCoeff;
                curF2 += (targetF2 - curF2) * vowelGlideCoeff;
                curF3 += (targetF3 - curF3) * vowelGlideCoeff;

                // ステレオ共通のSVF係数をホイスティングし、周波数が動いた時のみ std::tan を計算
                if (std::abs(curF1 - cachedF1) > 0.25f || std::abs(curF2 - cachedF2) > 0.25f || std::abs(curF3 - cachedF3) > 0.25f || cachedF1 < 0.0f)
                {
                    cachedF1 = curF1;
                    cachedF2 = curF2;
                    cachedF3 = curF3;
                    const float freqs[3] = { curF1, curF2, curF3 };
                    for (int j = 0; j < 3; ++j)
                    {
                        const float fc = juce::jlimit(20.0f, (float)sampleRate * 0.45f, freqs[j]) / (float)sampleRate;
                        bandCoeffs[(size_t)j].update(fc, 6.0f);
                    }
                }

                static const float bandWeights[3] = { 1.0f, 0.85f, 0.65f };
                float wetL = 0.0f, wetR = 0.0f;

                for (int j = 0; j < 3; ++j)
                {
                    float lp, bpNorm, hp;
                    fmtL[(size_t)j].processWithCoeffs(l, bandCoeffs[(size_t)j], lp, bpNorm, hp);
                    wetL += bpNorm * bandWeights[j];
                    fmtR[(size_t)j].processWithCoeffs(r, bandCoeffs[(size_t)j], lp, bpNorm, hp);
                    wetR += bpNorm * bandWeights[j];
                }

                l = l * (1.0f - mVowel) + wetL * 1.15f * mVowel;
                r = r * (1.0f - mVowel) + wetR * 1.15f * mVowel;
            }

            l = softClip(l * gain);
            r = softClip(r * gain);
        }

        static bool getPatternStep(int pattern, int step) noexcept
        {
            static const uint16_t kPatternMasks[kTotalPatterns] = {
                // Category 1: Straight (10)
                0xAAAA, 0x8888, 0xCCCC, 0xFFFF, 0xEEEE,
                0x5555, 0x2222, 0x1111, 0x7777, 0x9999,
                // Category 2: Triplet & Swing (8)
                0x9249, 0xB6DB, 0xEDDB, 0x8A8A, 0x8282,
                0xEA20, 0x9292, 0x4924,
                // Category 3: Dotted & Polyrhythm (8)
                0x8421, 0x9249, 0x4924, 0x8892,
                0x8A42, 0x94A5, 0x6969, 0x8924,
                // Category 4: ColorBass & EDM (12)
                0xEA00, 0x00A0, 0x0888, 0xA5A5,
                0x000F, 0x7777, 0xFFF0, 0x8080,
                0x9090, 0x9A48, 0x69B2, 0xFE02,
                // Category 5: Glitch & Complex (12)
                0x9248, 0xAD5B, 0x8892, 0xAA92, 0xADA5,
                0x5A5A, 0x9421, 0xF842, 0x248F,
                0xF0F0, 0x6B3D, 0xA8AE
            };

            const int pIdx = juce::jlimit(0, kTotalPatterns - 1, pattern);
            return ((kPatternMasks[pIdx] >> (step % 16)) & 1) != 0;
        }

    private:
        double sampleRate = 44100.0;
        double phase = 0.0;
        int stepIdx = 0, prevStepIdx = 0;
        float curEnv = 0.0f;
        float slewCoeff = 0.025f;
        float vowelGlideCoeff = 0.006f;
        int mRate = 4, mPattern = 0;
        float mDepth = 0.8f, mVowel = 0.5f, mDecay = 0.5f;
        std::array<Svf, 3> fmtL, fmtR;
        std::array<SvfCoeffs, 3> bandCoeffs;
        float curF1 = 700.0f, curF2 = 1100.0f, curF3 = 2400.0f;
        float cachedF1 = -1.0f, cachedF2 = -1.0f, cachedF3 = -1.0f;
    };
} // namespace gfx

// ==========================================
// FxChain — 4スロット直列
// ==========================================
class FxChain
{
public:
    FxChain() = default;

    static constexpr int kNumSlots = 5; // FXの数と一致

    enum FxType { None = 0, Saturation, Chorus, Delay, Freeze, Reverb, Gate };

    static juce::StringArray getTypeNames()
    {
        return { "None", "Saturation (ADAA)", "Ensemble Chorus", "Tape Delay", "Freeze", "Shimmer Reverb", "Gate" };
    }
    static juce::StringArray getSatAlgoNames()
    {
        return { "Soft Tanh", "Hard Clip", "Triode", "Tape", "Transformer",
                 "JFET", "BJT", "Wavefold", "Exciter", "Cubic" };
    }
    // コンボ index → ngk 内部 type (Bitcrush=8を除外して詰めたもの)
    static int satAlgoToType(int combo) noexcept
    {
        static const int map[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 10 };
        return map[juce::jlimit(0, 9, combo)];
    }

    // Delayタイム表 (拍数)
    static juce::StringArray getDelayTimeNames()
    {
        return { "1/2", "1/4.", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T" };
    }
    static float delayTimeToBeats(int idx) noexcept
    {
        static const float beats[10] = { 2.0f, 1.5f, 1.0f, 2.0f / 3.0f, 0.75f,
                                         0.5f, 1.0f / 3.0f, 0.375f, 0.25f, 1.0f / 6.0f };
        return beats[juce::jlimit(0, 9, idx)];
    }

    struct Params
    {
        std::array<int, kNumSlots> type {};
        std::array<float, kNumSlots> amount {};   // 0..1 (Wet量)
        double bpm = 120.0;

        // --- Saturation 詳細 ---
        int   satAlgo = 0;
        float satDrive = 2.0f;      // 1..12
        float satPreHz = 20.0f;     // 20..2000 (20≒スルー)
        float satTrimDb = 0.0f;     // -12..+12

        // --- Chorus 詳細 ---
        float choRate = 0.8f;       // Hz
        float choDepth = 0.5f;      // 0..1
        float choWidth = 1.0f;      // 0..1

        // --- Delay 詳細 ---
        int   dlyTime = 4;          // getDelayTimeNames() インデックス
        float dlyFeedback = 0.45f;  // 0..0.95
        float dlyDuck = 0.5f;       // 0..1
        float dlyDamp = 0.3f;       // 0..1

        // --- Freeze 詳細 ---
        float frzSize = 100.0f;     // ms
        float frzFeedback = 0.9f;   // 0..0.99
        float frzDamp = 0.2f;       // 0..1

        // --- Reverb 詳細 ---
        float revDecay = 0.7f;      // 0..1
        float revShimmer = 0.4f;    // 0..1
        float revDamp = 0.3f;       // 0..1
        float revMod = 0.4f;        // 0..1

        // --- Gate 詳細 (SPECTRA8 完全一致) ---
        int   gateRate = 4;        // 1/16 (FormantGate::getRateNames())
        int   gatePattern = 0;     // 0..49 (FormantGate::getPatternNames())
        float gateDepth = 80.0f;   // 0..100 (%)
        float gateDecay = 50.0f;   // 10..100 (%)
        float gateVowel = 50.0f;   // 0..100 (%)
        double ppqPosition = 0.0;
        bool   isPlaying = false;
    };

    void prepare(double sr)
    {
        sampleRate = sr > 1000.0 ? sr : 44100.0;
        chorus.prepareToPlay(sampleRate);
        delay.prepareToPlay(sampleRate);
        freeze.prepareToPlay(sampleRate);
        reverb.prepareToPlay(sampleRate);
        formantGate.prepare(sampleRate);
        // Sat DCブロッカーを ~6Hz に最適化（従来の ~35Hz による低域ロールオフを解消）
        dcCoef = std::exp(-juce::MathConstants<float>::twoPi * 6.0f / (float)sampleRate);
        for (int ch = 0; ch < 2; ++ch) { satState[ch].reset(); dcState[ch] = 0.0f; preX1[ch] = preY1[ch] = 0.0f; }
    }

    // オーディオ・スレッド安全: メモリ再確保を行わずに状態のみ消去
    void reset() noexcept
    {
        chorus.clearState();
        delay.clearState();
        freeze.clearState();
        reverb.clearState();
        formantGate.reset();
        for (int ch = 0; ch < 2; ++ch) { satState[ch].reset(); dcState[ch] = 0.0f; preX1[ch] = preY1[ch] = 0.0f; }
    }

    void process(juce::AudioBuffer<float>& buf, const Params& p) noexcept
    {
        const int numSamples = buf.getNumSamples();
        const int channels = buf.getNumChannels();
        if (numSamples <= 0 || channels == 0) return;

        // anyActive 早期リターンを削除。
        // FXスロットが全て amount==0 でも、Delay/Freeze/Reverb の内部バッファに
        // 入力を流し続ける必要がある。書き込みを止めると、amount が 0→非0 に
        // 変わった瞬間に古いバッファ残存データがフィードバックに載り爆発する。
        // 代わりに、全スロットが None の場合のみ早期リターンする。
        bool anyTyped = false;
        for (int s = 0; s < kNumSlots; ++s)
            if (p.type[(size_t)s] > 0) anyTyped = true;
        if (!anyTyped) return;

        float* dL = buf.getWritePointer(0);
        float* dR = channels > 1 ? buf.getWritePointer(1) : dL;

        const int satType = satAlgoToType(p.satAlgo);
        const float trimGain = juce::Decibels::decibelsToGain(juce::jlimit(-12.0f, 12.0f, p.satTrimDb));
        const bool satPreBypass = (p.satPreHz <= 21.0f);
        const float preAlpha = 1.0f / (1.0f + juce::MathConstants<float>::twoPi
                                       * juce::jlimit(20.0f, 2000.0f, p.satPreHz) / (float)sampleRate);
        const float dlyBeats = delayTimeToBeats(p.dlyTime);

        bool hasGate = false;
        for (int s = 0; s < kNumSlots; ++s)
            if (p.type[(size_t)s] == Gate) { hasGate = true; break; }
        if (hasGate)
            formantGate.setParams(p.gateRate, p.gatePattern,
                                  p.gateDepth * 0.01f, p.gateDecay * 0.01f, p.gateVowel * 0.01f);

        // スロット間ソフトクリップ: 前段FXが異常な振幅を出力しても後段で爆発しない
        auto interSlotClip = [](float x) noexcept -> float
        {
            const float t = 1.2f;
            const float a = std::abs(x);
            if (a <= t) return x;
            const float s = x < 0.0f ? -1.0f : 1.0f;
            return s * (t + (1.0f - t * 0.25f) * std::tanh((a - t) * 2.0f));
        };

        for (int i = 0; i < numSamples; ++i)
        {
            float l = dL[i];
            float r = dR[i];

            for (int s = 0; s < kNumSlots; ++s)
            {
                const int fxType = p.type[(size_t)s];
                if (fxType == None) continue;

                const float amt = p.amount[(size_t)s];

                // Delay/Freeze/Reverb は amount==0 でも内部バッファを更新し続ける。
                // Saturation/Chorus はステートレスに近いため amount==0 でスキップ可。
                switch (fxType)
                {
                case Saturation:
                    if (amt > 0.0005f)
                        saturate(l, r, amt, satType, p.satDrive, preAlpha, satPreBypass, trimGain);
                    break;
                case Chorus:
                    chorus.process(l, r, amt, p.choRate, p.choDepth, p.choWidth);
                    break;
                case Delay:
                    delay.process(l, r, amt, p.bpm, dlyBeats, p.dlyFeedback, p.dlyDuck, p.dlyDamp);
                    break;
                case Freeze:
                    freeze.process(l, r, amt, p.frzSize, p.frzFeedback, p.frzDamp);
                    break;
                case Reverb:
                    reverb.process(l, r, amt, p.revDecay, p.revShimmer, p.revDamp, p.revMod);
                    break;
                case Gate:
                {
                    const float inL = l;
                    const float inR = r;
                    float procL = l;
                    float procR = r;
                    const double sampleBeats = (p.bpm / 60.0) / sampleRate;
                    const double curPpq = p.ppqPosition + (double)i * sampleBeats;
                    formantGate.process(procL, procR, p.bpm, curPpq, p.isPlaying);
                    l = inL * (1.0f - amt) + procL * amt;
                    r = inR * (1.0f - amt) + procR * amt;
                    break;
                }
                default: break;
                }

                // スロット間ソフトクリップ（最終スロット以外）
                if (s < kNumSlots - 1)
                {
                    l = interSlotClip(l);
                    r = interSlotClip(r);
                }
            }

            dL[i] = l;
            if (channels > 1) dR[i] = r;
        }
    }

private:
    // amount=Dry/Wet, drive=1..12, preAlpha=プリHPF係数, preBypass=HPFバイパス, trimGain=出力トリム
    inline void saturate(float& l, float& r, float amount, int satType,
                         float drive, float preAlpha, bool preBypass, float trimGain) noexcept
    {
        const float comp = 1.0f / std::sqrt(juce::jmax(1.0f, drive)); // 音量補償
        const float mix = amount;

        auto one = [&](float x, int ch) noexcept
        {
            float hpfOut = x;
            if (preBypass)
            {
                // プリHPFバイパス時も境界切替が滑らかになるよう状態を追従
                preX1[ch] = x;
                preY1[ch] = x;
            }
            else
            {
                // 1次HPFプリフィルター (NextGenKick2と同構成)
                hpfOut = preAlpha * (preY1[ch] + x - preX1[ch]);
                preX1[ch] = x;
                preY1[ch] = hpfOut;
            }

            float sat = gfx::processSaturationSampleADAA(hpfOut, satType, drive, satState[ch]);
            // 6Hz DCブロッカー (Triode等の非対称カーブ対策)
            dcState[ch] = dcCoef * dcState[ch] + (1.0f - dcCoef) * sat;
            sat -= dcState[ch];
            return x * (1.0f - mix) + sat * comp * trimGain * mix;
        };

        l = one(l, 0);
        r = one(r, 1);
    }

    double sampleRate = 44100.0;

    gfx::EnsembleChorus chorus;
    gfx::TapeDelay delay;
    gfx::FreezeSmear freeze;
    gfx::ShimmerReverb reverb;
    gfx::FormantGate formantGate;

    gfx::SaturationState satState[2];
    float dcState[2] = { 0.0f, 0.0f };
    float preX1[2] = { 0.0f, 0.0f };
    float preY1[2] = { 0.0f, 0.0f };
    float dcCoef = 0.995f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChain)
};
