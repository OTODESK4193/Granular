// ==========================================
// File: PitchAnalyzer.cpp
// 高精度化ポイント:
//  1. 正規化自己相関 (NSDF風) — 音量に依存しない安定検出
//  2. オクターブエラーガード — 1オクターブ下の候補が僅差なら低い方を採用
//  3. 複数ウィンドウ解析 — サンプル3箇所の解析結果の中央値を採用
//  4. ファイル名Keyヒント — "Pad-C4"等からKeyを確定し、解析は高さの特定に専念
//  5. 表記はAbleton基準 (C3 = MIDI 60)
// ==========================================
#include "PitchAnalyzer.h"

#include <algorithm>
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
        info.noteName = PitchAnalyzer::midiNoteName(info.midiNote);
        info.isDetected = true;
    }

    inline bool isNoteLetter(juce::juce_wchar c) noexcept
    {
        return (c >= 'A' && c <= 'G') || (c >= 'a' && c <= 'g');
    }
    inline bool isAlnumChar(juce::juce_wchar c) noexcept
    {
        return juce::CharacterFunctions::isLetterOrDigit(c);
    }
    inline int letterToPitchClass(juce::juce_wchar c) noexcept
    {
        switch (juce::CharacterFunctions::toUpperCase(c))
        {
            case 'C': return 0;  case 'D': return 2;  case 'E': return 4;
            case 'F': return 5;  case 'G': return 7;  case 'A': return 9;
            case 'B': return 11; default:  return -1;
        }
    }
}

juce::String PitchAnalyzer::midiNoteName(int midiNote)
{
    // Ableton基準: C3 = MIDI 60 → octave = midi/12 - 2
    const int n = juce::jlimit(0, 127, midiNote);
    return juce::String(kNoteNames[n % 12]) + juce::String(n / 12 - 2);
}

// ------------------------------------------
// ファイル名からのKey抽出
//   対応例: "Pad-C4" "Bass(D)" "loop_C" "Am_pluck" "F#3_lead" "Db2" "Vox_Cmin"
//   誤検出防止: トークン前後が英数字でないこと（"Bass"のB, "Ambient"のA等は除外）
// ------------------------------------------
FilenameHint PitchAnalyzer::parseFilenameHint(const juce::String& filename)
{
    FilenameHint hint;
    const juce::String base = filename.upToLastOccurrenceOf(".", false, false);
    const int n = base.length();

    for (int i = 0; i < n; ++i)
    {
        const auto c = base[i];
        if (!isNoteLetter(c)) continue;

        // トークン開始境界: 直前が英数字なら不可
        if (i > 0 && isAlnumChar(base[i - 1])) continue;

        int pc = letterToPitchClass(c);
        int j = i + 1;

        // 変化記号 (#は常に。bは直後が英字でない場合、またはmaj/min等の
        //  クオリティトークンが続く場合のみフラットとみなす — "Bass"等の誤検出防止)
        if (j < n && base[j] == '#') { pc = (pc + 1) % 12; ++j; }
        else if (j < n && base[j] == 'b')
        {
            const bool nextLetter = (j + 1 < n && juce::CharacterFunctions::isLetter(base[j + 1]));
            bool qualityFollows = false;
            if (nextLetter)
            {
                const juce::String r = base.substring(j + 1).toLowerCase();
                auto ba = [&](int len) { return r.length() == len || !isAlnumChar(r[len]); };
                qualityFollows = (r.startsWith("minor") && ba(5)) || (r.startsWith("major") && ba(5))
                              || (r.startsWith("min") && ba(3))   || (r.startsWith("maj") && ba(3))
                              || (r.length() > 0 && r[0] == 'm' && base[j + 1] == 'm' && ba(1));
            }
            if (!nextLetter || qualityFollows) { pc = (pc + 11) % 12; ++j; }
        }

        // オクターブ (1桁数字, Ableton表記とみなす)
        int oct = -128;
        if (j < n && juce::CharacterFunctions::isDigit(base[j]))
        {
            oct = (int)(base[j] - '0');
            ++j;
        }

        // クオリティ (m / min / minor / maj / major)
        bool minor = false, hasQ = false;
        {
            const juce::String rest = base.substring(j).toLowerCase();
            auto boundaryAfter = [&](int len)
            {
                return rest.length() == len || !isAlnumChar(rest[len]);
            };
            if (rest.startsWith("minor") && boundaryAfter(5)) { minor = true; hasQ = true; j += 5; }
            else if (rest.startsWith("major") && boundaryAfter(5)) { minor = false; hasQ = true; j += 5; }
            else if (rest.startsWith("min") && boundaryAfter(3)) { minor = true; hasQ = true; j += 3; }
            else if (rest.startsWith("maj") && boundaryAfter(3)) { minor = false; hasQ = true; j += 3; }
            else if (rest.length() > 0 && rest[0] == 'm' && base[j] == 'm' && boundaryAfter(1))
            {
                minor = true; hasQ = true; j += 1; // 小文字'm'単体はminor ("Am")
            }
        }

        // トークン終端境界: 直後が英数字なら不可
        if (j < n && isAlnumChar(base[j])) continue;

        // 有効トークン。後方（サフィックス）優先なので上書きしていく
        hint.pitchClass = pc;
        hint.octave = oct;
        hint.isMinor = minor;
        hint.hasQuality = hasQ;
    }
    return hint;
}

// ------------------------------------------
// 正規化自己相関 (NSDF風) + オクターブエラーガード
// ------------------------------------------
PitchInfo PitchAnalyzer::analyzeAutocorrelation(const float* audioData, int length, double sampleRate,
                                                float minFreq, float corrThreshold)
{
    PitchInfo info;
    if (length < 1024) return info;

    const int windowLen = length / 2;
    int minLag = (int)(sampleRate / 1200.0);
    int maxLag = (int)(sampleRate / (double)minFreq);
    maxLag = juce::jmin(maxLag, windowLen);
    if (maxLag <= minLag + 2) return info;

    // エネルギーの累積和（正規化用）
    std::vector<double> prefixSq((size_t)(windowLen + maxLag + 1), 0.0);
    for (int i = 0; i < windowLen + maxLag && i < length; ++i)
        prefixSq[(size_t)i + 1] = prefixSq[(size_t)i] + (double)audioData[i] * (double)audioData[i];

    const double energy0 = prefixSq[(size_t)windowLen];
    if (energy0 < 1.0e-6) return info; // ほぼ無音

    std::vector<float> normCorr((size_t)maxLag, 0.0f);
    float maxVal = 0.0f;
    int bestLag = -1;

    for (int lag = minLag; lag < maxLag; ++lag)
    {
        float sum = 0.0f;
        for (int i = 0; i < windowLen; ++i)
            sum += audioData[i] * audioData[i + lag];

        const int hi = juce::jmin(lag + windowLen, length);
        const double energyLag = prefixSq[(size_t)hi] - prefixSq[(size_t)lag];
        const float norm = (float)((double)sum / (std::sqrt(energy0 * energyLag) + 1.0e-12));

        normCorr[(size_t)lag] = norm;
        if (norm > maxVal) { maxVal = norm; bestLag = lag; }
    }

    if (bestLag <= minLag || bestLag >= maxLag - 1 || maxVal < corrThreshold)
        return info;

    // オクターブエラーガード: 2倍ラグ(1オクターブ下)が僅差なら低い方を採用
    for (int guard = 0; guard < 2; ++guard)
    {
        const int lag2 = bestLag * 2;
        if (lag2 < maxLag - 1 && normCorr[(size_t)lag2] > 0.90f * normCorr[(size_t)bestLag])
            bestLag = lag2;
        else
            break;
    }

    // 放物線補間でラグを精密化
    const float alpha = normCorr[(size_t)bestLag - 1];
    const float beta = normCorr[(size_t)bestLag];
    const float gamma = normCorr[(size_t)bestLag + 1];
    const float denom = alpha - 2.0f * beta + gamma;
    const float peakOffset = (std::abs(denom) > 1.0e-9f) ? 0.5f * (alpha - gamma) / denom : 0.0f;

    const double refinedLag = (double)bestLag + (double)juce::jlimit(-0.5f, 0.5f, peakOffset);
    info.frequency = sampleRate / refinedLag;

    if (info.frequency > 15.0 && info.frequency < 5000.0)
        fillNoteInfo(info);

    return info;
}

// ------------------------------------------
// Omni-Analyzer (複数ルート × 複数ウィンドウ + Keyヒント)
// ------------------------------------------
PitchInfo PitchAnalyzer::analyzeOmni(const float* audioData, int length, double sampleRate,
                                     const juce::String& filename, int materialMode)
{
    enum Route { Standard, KickFFT, HatCym, Loop, Micro };
    std::vector<Route> plan;

    if (materialMode == 1)      plan = { KickFFT, HatCym, Standard };
    else if (materialMode == 2) plan = { Loop, Standard };
    else if (materialMode == 3) plan = { Standard, Micro };
    else
    {
        juce::String fn = filename.toLowerCase();
        if (length < 2048)
            plan = { Micro, Standard };
        else if (fn.contains("kick") || fn.contains("bass") || fn.contains("sub") || fn.contains("808") || fn.contains("drum"))
            plan = { KickFFT, Standard };
        else if (fn.contains("hat") || fn.contains("cym") || fn.contains("snare") || fn.contains("perc"))
            plan = { HatCym, Standard };
        else if (fn.contains("loop") || length > (int)(sampleRate * 2.0))
            plan = { Loop, Standard };
        else
            plan = { Standard, KickFFT, HatCym };
    }

    // --- 1セグメント分の解析（従来のルート試行） ---
    auto runPlan = [&](const float* data, int segLen) -> PitchInfo
    {
        for (Route r : plan)
        {
            PitchInfo result;
            if (r == Standard)
            {
                result = analyzeAutocorrelation(data, segLen, sampleRate, 40.0f, 0.40f);
            }
            else if (r == KickFFT)
            {
                const int skipSamples = (int)(sampleRate * 0.04);
                if (segLen - skipSamples > 1024)
                {
                    const int dftSize = juce::jmin(segLen - skipSamples, 8192);
                    const float* tail = data + skipSamples;

                    float maxMag = 0.0f;
                    int peakK = -1;
                    const int kMin = juce::jmax(1, (int)(20.0 * (double)dftSize / sampleRate));
                    const int kMax = (int)(150.0 * (double)dftSize / sampleRate);
                    std::vector<float> magnitudes((size_t)kMax + 2, 0.0f);

                    for (int k = kMin; k <= kMax; ++k)
                    {
                        float re = 0.0f, im = 0.0f;
                        const float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                        for (int m = 0; m < dftSize; ++m)
                        {
                            const float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)m / (float)(dftSize - 1)));
                            const float sample = tail[m] * window;
                            re += sample * std::cos(freqPhase * (float)m);
                            im -= sample * std::sin(freqPhase * (float)m);
                        }
                        const float mag = re * re + im * im;
                        magnitudes[(size_t)k] = mag;
                        if (mag > maxMag) { maxMag = mag; peakK = k; }
                    }

                    if (peakK > kMin && peakK < kMax)
                    {
                        const float alpha = magnitudes[(size_t)peakK - 1];
                        const float beta = magnitudes[(size_t)peakK];
                        const float gamma = magnitudes[(size_t)peakK + 1];
                        const float peakOffset = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
                        result.frequency = ((double)peakK + (double)peakOffset) * sampleRate / (double)dftSize;
                        if (result.frequency >= 20.0 && result.frequency <= 150.0)
                            fillNoteInfo(result);
                    }
                }
            }
            else if (r == HatCym)
            {
                const size_t downFactor = 4;
                const size_t downLen = (size_t)segLen / downFactor;
                if (downLen > 512)
                {
                    std::vector<float> down(downLen);
                    for (size_t i = 0; i < downLen; ++i) down[i] = data[i * downFactor];

                    const int dftSize = juce::jmin((int)downLen, 4096);
                    float maxMagnitude = 0.0f;
                    int peakBin = -1;
                    for (int k = 1; k < dftSize / 2; ++k)
                    {
                        float re = 0.0f, im = 0.0f;
                        const float freqPhase = juce::MathConstants<float>::twoPi * (float)k / (float)dftSize;
                        for (int m = 0; m < dftSize; ++m)
                        {
                            re += down[(size_t)m] * std::cos(freqPhase * (float)m);
                            im -= down[(size_t)m] * std::sin(freqPhase * (float)m);
                        }
                        const float mag = re * re + im * im;
                        if (mag > maxMagnitude) { maxMagnitude = mag; peakBin = k; }
                    }
                    if (peakBin > 0)
                    {
                        result.frequency = (double)peakBin * (sampleRate / (double)downFactor) / (double)dftSize * (double)downFactor;
                        if (result.frequency > 20.0 && result.frequency < 10000.0)
                            fillNoteInfo(result);
                    }
                }
            }
            else if (r == Loop)
            {
                const int cutLen = juce::jmin(segLen, (int)(sampleRate * 0.3));
                result = analyzeAutocorrelation(data, cutLen, sampleRate, 30.0f, 0.35f);
            }
            else if (r == Micro)
            {
                const size_t repCount = 10;
                std::vector<float> padded((size_t)segLen * repCount);
                for (size_t i = 0; i < repCount; ++i)
                    std::memcpy(padded.data() + i * (size_t)segLen, data, (size_t)segLen * sizeof(float));
                result = analyzeAutocorrelation(padded.data(), (int)padded.size(), sampleRate, 40.0f, 0.25f);
            }

            if (result.isDetected)
                return result;
        }
        return {};
    };

    // --- 複数ウィンドウ解析 → 中央値 ---
    PitchInfo finalResult;
    std::vector<double> freqs;
    const double offsets[3] = { 0.0, 0.30, 0.60 };

    for (double off : offsets)
    {
        const int offset = (int)((double)length * off);
        const int segLen = juce::jmin(length - offset, (int)(sampleRate * 1.2));
        if (segLen < 1024) continue;

        const PitchInfo r = runPlan(audioData + offset, segLen);
        if (r.isDetected)
        {
            freqs.push_back(r.frequency);
            if (!finalResult.isDetected) finalResult = r;
        }
    }

    if (freqs.size() >= 2)
    {
        std::sort(freqs.begin(), freqs.end());
        finalResult.frequency = freqs[(freqs.size() - 1) / 2]; // 中央値（偶数は下側）
        fillNoteInfo(finalResult);
    }

    // --- クロマグラムで Root と Maj/Min を推定 ---
    if (finalResult.isDetected)
    {
        float chromagram[12] = { 0.0f };
        const int calcLen = juce::jmin(length, 4096);

        for (int oct = 2; oct <= 5; ++oct)
        {
            for (int note = 0; note < 12; ++note)
            {
                const float freq = 440.0f * std::pow(2.0f, (float)((oct * 12 + note) - 69) / 12.0f);
                float re = 0.0f, im = 0.0f;
                const float phaseInc = juce::MathConstants<float>::twoPi * freq / (float)sampleRate;
                for (int m = 0; m < calcLen; m += 4)
                {
                    re += audioData[m] * std::cos(phaseInc * (float)m);
                    im -= audioData[m] * std::sin(phaseInc * (float)m);
                }
                chromagram[note] += std::sqrt(re * re + im * im);
            }
        }

        int bestRoot = 0;
        float maxChroma = 0.0f;
        for (int i = 0; i < 12; ++i)
            if (chromagram[i] > maxChroma) { maxChroma = chromagram[i]; bestRoot = i; }

        finalResult.detectedRoot = bestRoot;
        finalResult.isMajor = (chromagram[(bestRoot + 4) % 12] >= chromagram[(bestRoot + 3) % 12]);
        finalResult.scaleName = juce::String(kNoteNames[bestRoot]) + (finalResult.isMajor ? " Major" : " Minor");
    }

    // --- ファイル名Keyヒント（最優先で確定） ---
    const FilenameHint hint = parseFilenameHint(filename);
    if (hint.isValid())
    {
        if (finalResult.isDetected)
        {
            // 解析は「高さ」だけに使い、音名はヒントで確定
            const double midiFloat = 69.0 + 12.0 * std::log2(finalResult.frequency / 440.0);
            int targetMidi;
            if (hint.octave > -100)
            {
                targetMidi = (hint.octave + 2) * 12 + hint.pitchClass; // Ableton: C3=60
                // 解析結果と6半音以上ズレていたら最寄りオクターブに再配置
                if (std::abs(midiFloat - (double)targetMidi) > 6.5)
                    targetMidi = hint.pitchClass + 12 * juce::roundToInt((midiFloat - (double)hint.pitchClass) / 12.0);
            }
            else
            {
                targetMidi = hint.pitchClass + 12 * juce::roundToInt((midiFloat - (double)hint.pitchClass) / 12.0);
            }
            targetMidi = juce::jlimit(0, 127, targetMidi);
            finalResult.midiNote = targetMidi;
            finalResult.centsOffset = (float)juce::jlimit(-600.0, 600.0, (midiFloat - (double)targetMidi) * 100.0);
            finalResult.noteName = midiNoteName(targetMidi);
        }
        else
        {
            // 解析不能素材でもヒントから音程を確定（オクターブ不明ならC3域）
            const int targetMidi = juce::jlimit(0, 127,
                (hint.octave > -100 ? (hint.octave + 2) * 12 : 60) + hint.pitchClass);
            finalResult.midiNote = targetMidi;
            finalResult.frequency = 440.0 * std::pow(2.0, ((double)targetMidi - 69.0) / 12.0);
            finalResult.centsOffset = 0.0f;
            finalResult.noteName = midiNoteName(targetMidi);
            finalResult.isDetected = true;
        }

        finalResult.detectedRoot = hint.pitchClass;
        if (hint.hasQuality) finalResult.isMajor = !hint.isMinor;
        finalResult.scaleName = juce::String(kNoteNames[hint.pitchClass])
                              + (finalResult.isMajor ? " Major" : " Minor")
                              + " (from filename)";
    }

    return finalResult;
}
