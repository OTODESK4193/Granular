// ==========================================
// File: PitchAnalyzer.cpp
// ==========================================
#include "PitchAnalyzer.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace
{
    const char* kNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    void fillNoteInfo(PitchInfo& info)
    {
        const double midiFloat = 69.0 + 12.0 * std::log2(info.frequency / 440.0);
        info.midiNote = juce::roundToInt(midiFloat);
        info.centsOffset = (float)((midiFloat - (double)info.midiNote) * 100.0);
        info.noteName = juce::String(kNoteNames[juce::jmax(0, info.midiNote) % 12])
                      + juce::String((info.midiNote / 12) - 1);
        info.isDetected = true;
    }
}

PitchInfo PitchAnalyzer::analyzeAutocorrelation(const float* audioData, int length, double sampleRate,
                                                float minFreq, float corrThreshold)
{
    PitchInfo info;
    if (length < 1024) return info;

    int minLag = (int)(sampleRate / 1200.0);
    int maxLag = (int)(sampleRate / (double)minFreq);
    maxLag = juce::jmin(maxLag, length / 2);

    std::vector<float> corr((size_t)maxLag, 0.0f);
    float maxCorr = 0.0f;
    int bestLag = -1;

    for (int lag = minLag; lag < maxLag; ++lag) {
        float sum = 0.0f;
        for (size_t i = 0; i < (size_t)(length / 2); ++i) sum += audioData[i] * audioData[i + (size_t)lag];
        corr[(size_t)lag] = sum;
        if (sum > maxCorr) { maxCorr = sum; bestLag = lag; }
    }

    float energy = 0.0f;
    for (size_t i = 0; i < (size_t)(length / 2); ++i) energy += audioData[i] * audioData[i];

    if (energy > 0.0001f && bestLag > minLag && bestLag < maxLag - 1 && (maxCorr / energy) > corrThreshold) {
        const float alpha = corr[(size_t)bestLag - 1];
        const float beta = corr[(size_t)bestLag];
        const float gamma = corr[(size_t)bestLag + 1];
        const float peakOffset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);

        const double refinedLag = (double)bestLag + (double)peakOffset;
        info.frequency = sampleRate / refinedLag;

        if (info.frequency > 15.0 && info.frequency < 5000.0)
            fillNoteInfo(info);
    }
    return info;
}

PitchInfo PitchAnalyzer::analyzeOmni(const float* audioData, int length, double sampleRate,
                                     const juce::String& filename, int materialMode)
{
    enum Route { Standard, KickFFT, HatCym, Loop, Micro };
    std::vector<Route> plan;

    if (materialMode == 1) {
        plan = { KickFFT, HatCym, Standard };
    }
    else if (materialMode == 2) {
        plan = { Loop, Standard };
    }
    else if (materialMode == 3) {
        plan = { Standard, Micro };
    }
    else {
        juce::String fn = filename.toLowerCase();
        if (length < 2048) {
            plan = { Micro, Standard };
        }
        else if (fn.contains("kick") || fn.contains("bass") || fn.contains("sub") || fn.contains("808") || fn.contains("drum")) {
            plan = { KickFFT, Standard };
        }
        else if (fn.contains("hat") || fn.contains("cym") || fn.contains("snare") || fn.contains("perc")) {
            plan = { HatCym, Standard };
        }
        else if (fn.contains("loop") || length > (int)(sampleRate * 2.0)) {
            plan = { Loop, Standard };
        }
        else {
            plan = { Standard, KickFFT, HatCym };
        }
    }

    PitchInfo finalResult;
    for (Route r : plan) {
        PitchInfo result;
        if (r == Standard) {
            result = analyzeAutocorrelation(audioData, length, sampleRate, 40.0f, 0.35f);
        }
        else if (r == KickFFT) {
            // アタックを飛ばしてテール部の基音をDFTで探す (20-150Hz)
            const int skipSamples = (int)(sampleRate * 0.04);
            if (length - skipSamples > 1024) {
                const int dftSize = juce::jmin(length - skipSamples, 8192);
                const float* tail = audioData + skipSamples;

                float maxMag = 0.0f;
                int peakK = -1;

                const int kMin = juce::jmax(1, (int)(20.0 * (double)dftSize / sampleRate));
                const int kMax = (int)(150.0 * (double)dftSize / sampleRate);

                std::vector<float> magnitudes((size_t)kMax + 2, 0.0f);

                for (int k = kMin; k <= kMax; ++k) {
                    float re = 0.0f, im = 0.0f;
                    const float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                    for (int n = 0; n < dftSize; ++n) {
                        const float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)n / (float)(dftSize - 1)));
                        const float sample = tail[n] * window;
                        re += sample * std::cos(freqPhase * (float)n);
                        im -= sample * std::sin(freqPhase * (float)n);
                    }
                    const float mag = re * re + im * im;
                    magnitudes[(size_t)k] = mag;
                    if (mag > maxMag) { maxMag = mag; peakK = k; }
                }

                if (peakK > kMin && peakK < kMax) {
                    const float alpha = magnitudes[(size_t)peakK - 1];
                    const float beta = magnitudes[(size_t)peakK];
                    const float gamma = magnitudes[(size_t)peakK + 1];
                    const float peakOffset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);

                    const double refinedK = (double)peakK + (double)peakOffset;
                    result.frequency = refinedK * sampleRate / (double)dftSize;

                    if (result.frequency >= 20.0 && result.frequency <= 150.0)
                        fillNoteInfo(result);
                }
            }
        }
        else if (r == HatCym) {
            // 高域素材: 1/4ダウンサンプルしてDFTピークを探す
            const size_t downFactor = 4;
            const size_t downLen = (size_t)length / downFactor;
            if (downLen > 512) {
                std::vector<float> down(downLen);
                for (size_t i = 0; i < downLen; ++i) down[i] = audioData[i * downFactor];

                const int dftSize = juce::jmin((int)downLen, 4096);
                float maxMagnitude = 0.0f;
                int peakBin = -1;
                for (int k = 1; k < dftSize / 2; ++k) {
                    float re = 0.0f, im = 0.0f;
                    const float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                    for (int n = 0; n < dftSize; ++n) {
                        re += down[(size_t)n] * std::cos(freqPhase * (float)n);
                        im -= down[(size_t)n] * std::sin(freqPhase * (float)n);
                    }
                    const float mag = re * re + im * im;
                    if (mag > maxMagnitude) { maxMagnitude = mag; peakBin = k; }
                }
                if (peakBin > 0) {
                    const double downFreq = (double)peakBin * (sampleRate / (double)downFactor) / (double)dftSize;
                    result.frequency = downFreq * (double)downFactor;
                    if (result.frequency > 20.0 && result.frequency < 10000.0)
                        fillNoteInfo(result);
                }
            }
        }
        else if (r == Loop) {
            const int cutLen = juce::jmin(length, (int)(sampleRate * 0.3));
            result = analyzeAutocorrelation(audioData, cutLen, sampleRate, 30.0f, 0.3f);
        }
        else if (r == Micro) {
            // 極小素材は繰り返しパディングしてから解析
            const size_t repCount = 10;
            std::vector<float> padded((size_t)length * repCount);
            for (size_t i = 0; i < repCount; ++i)
                std::memcpy(padded.data() + i * (size_t)length, audioData, (size_t)length * sizeof(float));
            result = analyzeAutocorrelation(padded.data(), (int)padded.size(), sampleRate, 40.0f, 0.2f);
        }

        if (result.isDetected) {
            finalResult = result;
            break;
        }
    }

    // クロマグラムで Root と Maj/Min を推定
    if (finalResult.isDetected) {
        float chromagram[12] = { 0.0f };
        const int calcLen = juce::jmin(length, 4096);

        for (int oct = 2; oct <= 5; ++oct) {
            for (int note = 0; note < 12; ++note) {
                const float freq = 440.0f * std::pow(2.0f, (float)((oct * 12 + note) - 69) / 12.0f);
                float re = 0.0f, im = 0.0f;
                const float phaseInc = juce::MathConstants<float>::twoPi * freq / (float)sampleRate;

                for (int n = 0; n < calcLen; n += 4) {
                    re += audioData[n] * std::cos(phaseInc * (float)n);
                    im -= audioData[n] * std::sin(phaseInc * (float)n);
                }
                chromagram[note] += std::sqrt(re * re + im * im);
            }
        }

        int bestRoot = 0;
        float maxChroma = 0.0f;
        for (int i = 0; i < 12; ++i)
            if (chromagram[i] > maxChroma) { maxChroma = chromagram[i]; bestRoot = i; }

        finalResult.detectedRoot = bestRoot;

        const float m3 = chromagram[(bestRoot + 3) % 12];
        const float M3 = chromagram[(bestRoot + 4) % 12];
        finalResult.isMajor = (M3 >= m3);

        finalResult.scaleName = juce::String(kNoteNames[bestRoot]) + (finalResult.isMajor ? " Major" : " Minor");
    }

    return finalResult;
}
