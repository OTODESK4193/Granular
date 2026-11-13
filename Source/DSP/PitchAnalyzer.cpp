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
// 内部ヘルパー: FFTサイズ決定 (2の冪乗)
// ------------------------------------------
namespace
{
    int nextPow2Order(int minSize) noexcept
    {
        int order = 0;
        int n = 1;
        while (n < minSize) { n <<= 1; ++order; }
        return order;
    }

    // Temperley (2001) オーディオ向け24調キープロファイルによるピアソン相関推定
    void estimateKeyFromChroma(const float chroma[12], int& outRoot, bool& outIsMajor) noexcept
    {
        static constexpr float kMajorProfile[12] = {
            5.0f, 2.0f, 3.5f, 2.0f, 4.5f, 4.0f, 2.0f, 4.5f, 2.0f, 3.5f, 1.5f, 4.0f
        };
        static constexpr float kMinorProfile[12] = {
            5.0f, 2.0f, 3.5f, 4.5f, 2.0f, 4.0f, 2.0f, 4.5f, 3.5f, 2.0f, 1.5f, 4.0f
        };

        float meanC = 0.0f;
        for (int i = 0; i < 12; ++i) meanC += chroma[i];
        meanC /= 12.0f;

        float normC = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            const float d = chroma[i] - meanC;
            normC += d * d;
        }
        if (normC <= 1.0e-12f)
        {
            outRoot = 0;
            outIsMajor = true;
            return;
        }
        const float invStdC = 1.0f / std::sqrt(normC);

        auto evalCorrelation = [&](const float profile[12], int root) -> float
        {
            float meanP = 0.0f;
            for (int i = 0; i < 12; ++i) meanP += profile[i];
            meanP /= 12.0f;

            float num = 0.0f;
            float normP = 0.0f;
            for (int i = 0; i < 12; ++i)
            {
                const float dc = chroma[(root + i) % 12] - meanC;
                const float dp = profile[i] - meanP;
                num += dc * dp;
                normP += dp * dp;
            }
            return (normP > 1.0e-12f) ? (num * invStdC / std::sqrt(normP)) : -1.0f;
        };

        float bestCorr = -2.0f;
        int bestRoot = 0;
        bool bestMajor = true;
        for (int r = 0; r < 12; ++r)
        {
            const float rMaj = evalCorrelation(kMajorProfile, r);
            if (rMaj > bestCorr) { bestCorr = rMaj; bestRoot = r; bestMajor = true; }

            const float rMin = evalCorrelation(kMinorProfile, r);
            if (rMin > bestCorr) { bestCorr = rMin; bestRoot = r; bestMajor = false; }
        }

        outRoot = bestRoot;
        outIsMajor = bestMajor;
    }

    float parabolicOffset(float alpha, float beta, float gamma) noexcept
    {
        const float denom = alpha - 2.0f * beta + gamma;
        if (std::abs(denom) <= 1.0e-9f)
            return 0.0f;
        return juce::jlimit(-0.5f, 0.5f, 0.5f * (alpha - gamma) / denom);
    }
}

// ------------------------------------------
// FFT高速化 YIN (CMNDF) + MPM (NSDF) ハイブリッドピッチ検出
// ・O(N log N) FFT相互相関で差分関数 d(tau) と NSDF(tau) を同時算出
// ・YINのfirst-dip規則 + MPMキーマキシマによりオクターブ上下誤検出を防止
// ・64サンプル以上のマイクロ素材も直接解析可能
// ------------------------------------------
PitchInfo PitchAnalyzer::analyzeAutocorrelation(const float* audioData, int length, double sampleRate,
                                                float minFreq, float corrThreshold)
{
    PitchInfo info;
    if (audioData == nullptr || length < 64 || sampleRate <= 0.0)
        return info;

    const float safeMinFreq = juce::jmax(15.0f, minFreq);
    const int minLag = juce::jmax(2, (int)std::floor(sampleRate / 2500.0));
    const int maxLagReq = (int)std::ceil(sampleRate / (double)safeMinFreq);
    const int maxLag = juce::jmin(maxLagReq, length / 2);
    if (maxLag <= minLag + 3)
        return info;

    const int windowLen = juce::jmin(4096, length - maxLag);
    if (windowLen < 32)
        return info;

    const int analysisLen = windowLen + maxLag;

    // DC除去（NSDFのゼロオフセット耐性向上）
    double mean = 0.0;
    for (int i = 0; i < analysisLen; ++i)
        mean += (double)audioData[i];
    mean /= (double)analysisLen;

    std::vector<float> x((size_t)analysisLen, 0.0f);
    std::vector<double> prefixSq((size_t)analysisLen + 1, 0.0);
    for (int i = 0; i < analysisLen; ++i)
    {
        const float v = audioData[i] - (float)mean;
        x[(size_t)i] = v;
        prefixSq[(size_t)i + 1] = prefixSq[(size_t)i] + (double)v * (double)v;
    }

    const double energy0 = prefixSq[(size_t)windowLen];
    if (energy0 < 1.0e-9 * (double)windowLen)
        return info; // 無音

    // FFTによる O(N log N) 相互相関 r(tau) = sum_{j=0}^{W-1} x[j] * x[j + tau]
    const int fftOrder = juce::jmax(4, nextPow2Order(analysisLen));
    const int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft(fftOrder);

    std::vector<float> aBuf((size_t)(2 * fftSize), 0.0f);
    std::vector<float> bBuf((size_t)(2 * fftSize), 0.0f);
    std::vector<float> prodBuf((size_t)(2 * fftSize), 0.0f);

    std::memcpy(aBuf.data(), x.data(), (size_t)windowLen * sizeof(float));
    std::memcpy(bBuf.data(), x.data(), (size_t)analysisLen * sizeof(float));

    fft.performRealOnlyForwardTransform(aBuf.data(), true);
    fft.performRealOnlyForwardTransform(bBuf.data(), true);

    const int halfBins = fftSize / 2;
    for (int k = 0; k <= halfBins; ++k)
    {
        const float reA = aBuf[(size_t)(2 * k)];
        const float imA = aBuf[(size_t)(2 * k + 1)];
        const float reB = bBuf[(size_t)(2 * k)];
        const float imB = bBuf[(size_t)(2 * k + 1)];
        // conj(A) * B
        prodBuf[(size_t)(2 * k)]     = reA * reB + imA * imB;
        prodBuf[(size_t)(2 * k + 1)] = reA * imB - imA * reB;
    }
    prodBuf[1] = 0.0f;
    prodBuf[(size_t)(2 * halfBins + 1)] = 0.0f;

    fft.performRealOnlyInverseTransform(prodBuf.data());

    std::vector<float> cmndf((size_t)maxLag + 1, 1.0f);
    std::vector<float> nsdf((size_t)maxLag + 1, 0.0f);

    double runningDiffSum = 0.0;
    float maxNsdf = -1.0f;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        const double energyTau = prefixSq[(size_t)(windowLen + tau)] - prefixSq[(size_t)tau];
        const double rTau = (double)prodBuf[(size_t)tau];
        const double dTau = std::max(0.0, energy0 + energyTau - 2.0 * rTau);

        runningDiffSum += dTau;
        cmndf[(size_t)tau] = (runningDiffSum > 1.0e-12)
            ? (float)((double)tau * dTau / runningDiffSum)
            : 1.0f;

        const double denomEnergy = energy0 + energyTau;
        nsdf[(size_t)tau] = (denomEnergy > 1.0e-12)
            ? (float)juce::jlimit(-1.0, 1.0, (2.0 * rTau) / denomEnergy)
            : 0.0f;

        if (tau >= minLag && nsdf[(size_t)tau] > maxNsdf)
            maxNsdf = nsdf[(size_t)tau];
    }

    if (maxNsdf < corrThreshold)
        return info;

    // 1. YIN First-Dip 規則: cmndf < 0.18 かつ nsdf >= 0.85 * maxNsdf となる最初の局所最小を選ぶ
    int bestLag = -1;
    for (int tau = minLag; tau < maxLag - 1; ++tau)
    {
        if (cmndf[(size_t)tau] < 0.18f && nsdf[(size_t)tau] >= 0.85f * maxNsdf)
        {
            while (tau + 1 < maxLag && cmndf[(size_t)(tau + 1)] <= cmndf[(size_t)tau])
                ++tau;
            bestLag = tau;
            break;
        }
    }

    // 2. フォールバック: YINしきい値未到達時は MPM (最初の 0.88 * maxNsdf ピーク) を採用
    if (bestLag < 0)
    {
        const float keyThresh = 0.88f * maxNsdf;
        for (int tau = minLag + 1; tau < maxLag - 1; ++tau)
        {
            if (nsdf[(size_t)tau] >= keyThresh
                && nsdf[(size_t)tau] >= nsdf[(size_t)(tau - 1)]
                && nsdf[(size_t)tau] >= nsdf[(size_t)(tau + 1)])
            {
                bestLag = tau;
                break;
            }
        }
        if (bestLag < 0)
        {
            float bestC = 10.0f;
            for (int tau = minLag + 1; tau < maxLag - 1; ++tau)
            {
                if (cmndf[(size_t)tau] < bestC)
                {
                    bestC = cmndf[(size_t)tau];
                    bestLag = tau;
                }
            }
        }
    }

    // 3. サブオクターブ誤検出ガード: bestLag が 2T (1オクターブ下) に落ちていたら T へ戻す
    if (bestLag >= minLag * 2)
    {
        for (int guard = 0; guard < 2; ++guard)
        {
            const int half = bestLag / 2;
            if (half < minLag + 1) break;

            int localBest = -1;
            float localMinC = cmndf[(size_t)bestLag];
            for (int t = juce::jmax(minLag + 1, half - 2); t <= juce::jmin(maxLag - 2, half + 2); ++t)
            {
                if (cmndf[(size_t)t] < localMinC * 1.15f && nsdf[(size_t)t] >= 0.90f * nsdf[(size_t)bestLag])
                {
                    localMinC = cmndf[(size_t)t];
                    localBest = t;
                }
            }
            if (localBest > 0) bestLag = localBest;
            else break;
        }
    }

    if (bestLag <= minLag || bestLag >= maxLag - 1)
        return info;

    // 放物線補間（CMNDF極小値のサブサンプル精密化）
    const float peakOffset = parabolicOffset(cmndf[(size_t)bestLag - 1],
                                             cmndf[(size_t)bestLag],
                                             cmndf[(size_t)bestLag + 1]);
    const double refinedLag = (double)bestLag + (double)peakOffset;
    if (refinedLag <= 1.0)
        return info;

    info.frequency = sampleRate / refinedLag;
    info.clarity = juce::jlimit(0.0f, 1.0f, nsdf[(size_t)bestLag]);

    if (info.frequency > 15.0 && info.frequency < 5000.0)
        fillNoteInfo(info);

    return info;
}

// ------------------------------------------
// Omni-Analyzer (FFTベース複数ルート × 複数ウィンドウ + Temperley 24キー推定 + Keyヒント)
// ------------------------------------------
PitchInfo PitchAnalyzer::analyzeOmni(const float* audioData, int length, double sampleRate,
                                     const juce::String& filename, int materialMode)
{
    if (audioData == nullptr || length < 16 || sampleRate <= 0.0)
        return {};

    enum Route { Standard, KickFFT, HatCym, Loop, Micro };
    std::vector<Route> plan;

    if (materialMode == 1)      plan = { KickFFT, HatCym, Standard };
    else if (materialMode == 2) plan = { Loop, Standard };
    else if (materialMode == 3) plan = { Standard, Micro };
    else
    {
        const juce::String fn = filename.toLowerCase();
        if (length < 2048)
            plan = { Micro, Standard, HatCym };
        else if (fn.contains("kick") || fn.contains("bass") || fn.contains("sub") || fn.contains("808") || fn.contains("drum"))
            plan = { KickFFT, Standard };
        else if (fn.contains("hat") || fn.contains("cym") || fn.contains("snare") || fn.contains("perc"))
            plan = { HatCym, Standard };
        else if (fn.contains("loop") || length > (int)(sampleRate * 2.0))
            plan = { Loop, Standard };
        else
            plan = { Standard, KickFFT, HatCym };
    }

    auto analyzeSpectrumFFT = [&](const float* data, int segLen, double minHz, double maxHz) -> PitchInfo
    {
        PitchInfo res;
        if (segLen < 64) return res;

        const int targetLen = juce::jmin(segLen, 8192);
        const int fftOrder = juce::jlimit(9, 13, nextPow2Order(targetLen));
        const int fftSize = 1 << fftOrder;
        const int copyLen = juce::jmin(segLen, fftSize);

        juce::dsp::FFT fft(fftOrder);
        std::vector<float> buf((size_t)(2 * fftSize), 0.0f);
        const float denomWin = (float)juce::jmax(1, copyLen - 1);
        for (int m = 0; m < copyLen; ++m)
        {
            const float w = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)m / denomWin));
            buf[(size_t)m] = data[m] * w;
        }

        fft.performFrequencyOnlyForwardTransform(buf.data(), true);

        const int halfBins = fftSize / 2;
        const int kMin = juce::jmax(1, (int)std::ceil(minHz * (double)fftSize / sampleRate));
        const int kMax = juce::jmin(halfBins - 2, (int)std::floor(maxHz * (double)fftSize / sampleRate));
        if (kMax <= kMin) return res;

        float maxMag = 0.0f;
        double sumMag = 0.0;
        int peakK = -1;
        for (int k = kMin; k <= kMax; ++k)
        {
            const float mag = buf[(size_t)k];
            sumMag += (double)mag;
            if (mag > maxMag) { maxMag = mag; peakK = k; }
        }

        if (peakK >= kMin && peakK <= kMax && maxMag > 1.0e-6f)
        {
            const float offset = (peakK > kMin && peakK < kMax)
                ? parabolicOffset(-buf[(size_t)peakK - 1],
                                  -buf[(size_t)peakK],
                                  -buf[(size_t)peakK + 1])
                : 0.0f;
            res.frequency = ((double)peakK + (double)offset) * sampleRate / (double)fftSize;
            const double meanMag = sumMag / (double)(kMax - kMin + 1);
            res.clarity = juce::jlimit(0.0f, 1.0f, (float)(maxMag / (maxMag + 4.0 * meanMag + 1.0e-9)));
            if (res.frequency >= minHz && res.frequency <= maxHz)
                fillNoteInfo(res);
        }
        return res;
    };

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
                if (segLen - skipSamples >= 512)
                    result = analyzeSpectrumFFT(data + skipSamples, segLen - skipSamples, 20.0, 150.0);
            }
            else if (r == HatCym)
            {
                // ダウンサンプリング(エイリアシング)を廃し、フルレートHann窓FFTで全帯域を解析
                const double upperHz = juce::jmin(12000.0, sampleRate * 0.45);
                result = analyzeSpectrumFFT(data, segLen, 20.0, upperHz);
            }
            else if (r == Loop)
            {
                const int cutLen = juce::jmin(segLen, (int)(sampleRate * 0.3));
                result = analyzeAutocorrelation(data, cutLen, sampleRate, 30.0f, 0.35f);
            }
            else if (r == Micro)
            {
                // マイクロ素材(<2048サンプル)を周期タイルしてYIN/MPMのラグ窓を確保
                const size_t minTiled = 4096;
                const size_t repCount = std::max<size_t>(10, (minTiled + (size_t)segLen - 1) / (size_t)segLen);
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

    // --- 複数ウィンドウ解析 → 中央値 (2048サンプル未満は全体1ショット解析) ---
    PitchInfo finalResult;
    if (length < 2048)
    {
        finalResult = runPlan(audioData, length);
    }
    else
    {
        std::vector<PitchInfo> candidates;
        const double offsets[3] = { 0.0, 0.30, 0.60 };
        for (double off : offsets)
        {
            const int offset = (int)((double)length * off);
            const int segLen = juce::jmin(length - offset, (int)(sampleRate * 1.2));
            if (segLen < 512) continue;

            const PitchInfo r = runPlan(audioData + offset, segLen);
            if (r.isDetected)
                candidates.push_back(r);
        }

        if (!candidates.empty())
        {
            std::sort(candidates.begin(), candidates.end(),
                      [](const PitchInfo& a, const PitchInfo& b) { return a.frequency < b.frequency; });
            finalResult = candidates[(candidates.size() - 1) / 2];
            fillNoteInfo(finalResult);
        }
    }

    // --- Fujishima PCP クロマグラム + Temperley 24キー検出 ---
    if (finalResult.isDetected)
    {
        float chromagram[12] = { 0.0f };
        constexpr int kChromaOrder = 13;
        constexpr int kChromaFFT = 1 << kChromaOrder; // 8192
        juce::dsp::FFT chromaFFT(kChromaOrder);
        std::vector<float> buf((size_t)(2 * kChromaFFT), 0.0f);

        auto accumulateChromaFrame = [&](const float* src, int frameLen)
        {
            std::fill(buf.begin(), buf.end(), 0.0f);
            if (frameLen < 512)
            {
                const int tiled = juce::jmin(4096, kChromaFFT);
                for (int i = 0; i < tiled; ++i)
                {
                    const float w = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / (float)(tiled - 1)));
                    buf[(size_t)i] = src[i % frameLen] * w;
                }
            }
            else
            {
                const int n = juce::jmin(frameLen, kChromaFFT);
                const float denomWin = (float)juce::jmax(1, n - 1);
                for (int i = 0; i < n; ++i)
                {
                    const float w = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)i / denomWin));
                    buf[(size_t)i] = src[i] * w;
                }
            }

            chromaFFT.performFrequencyOnlyForwardTransform(buf.data(), true);

            const int half = kChromaFFT / 2;
            const int binMin = juce::jmax(1, (int)std::ceil(60.0 * (double)kChromaFFT / sampleRate));
            const int binMax = juce::jmin(half - 1, (int)std::floor(2500.0 * (double)kChromaFFT / sampleRate));

            for (int k = binMin; k <= binMax; ++k)
            {
                const float mag = buf[(size_t)k];
                if (mag <= 1.0e-7f) continue;

                const double freq = (double)k * sampleRate / (double)kChromaFFT;
                const double midi = 69.0 + 12.0 * std::log2(freq / 440.0);
                const int nearest = juce::roundToInt(midi);
                const float dev = (float)(midi - (double)nearest);
                const float cosW = std::cos(dev * juce::MathConstants<float>::pi);
                const int pc = ((nearest % 12) + 12) % 12;
                chromagram[pc] += mag * (cosW * cosW);
            }
        };

        if (length <= kChromaFFT)
        {
            accumulateChromaFrame(audioData, length);
        }
        else
        {
            const double frameOffs[3] = { 0.0, 0.35, 0.70 };
            for (double off : frameOffs)
            {
                const int start = (int)((double)length * off);
                const int rem = length - start;
                if (rem >= 1024)
                    accumulateChromaFrame(audioData + start, rem);
            }
        }

        int bestRoot = 0;
        bool bestMajor = true;
        estimateKeyFromChroma(chromagram, bestRoot, bestMajor);

        finalResult.detectedRoot = bestRoot;
        finalResult.isMajor = bestMajor;
        finalResult.scaleName = juce::String(kNoteNames[bestRoot]) + (bestMajor ? " Major" : " Minor");
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
