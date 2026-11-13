// ==========================================
// File: GranularEngine.cpp
// ==========================================
#include "GranularEngine.h"
#include "ModMatrix.h"

#include <algorithm>

// ------------------------------------------
// 4点 Hermite 補間（グレイン読み出しの音質の要）
// ------------------------------------------
static inline float hermite4(float frac, float y0, float y1, float y2, float y3) noexcept
{
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

static inline float readHermiteClamped(const float* src, int numSamples, double pos) noexcept
{
    if (numSamples <= 0 || src == nullptr) return 0.0f;
    const int i1 = (int)std::floor(pos);
    const float frac = (float)(pos - (double)i1);
    const int i0 = juce::jlimit(0, numSamples - 1, i1 - 1);
    const int ic = juce::jlimit(0, numSamples - 1, i1);
    const int i2 = juce::jlimit(0, numSamples - 1, i1 + 1);
    const int i3 = juce::jlimit(0, numSamples - 1, i1 + 2);
    return hermite4(frac, src[i0], src[ic], src[i2], src[i3]);
}

// ------------------------------------------
// SampleMipLevels: 31タップ・ゼロ位相 Blackman 窓付き sinc (fc=0.45π) による 2x デシメーションピラミッド
// ------------------------------------------
void SampleMipLevels::buildFrom(const juce::AudioBuffer<float>& base)
{
    clear();
    const int numCh = base.getNumChannels();
    const int baseLen = base.getNumSamples();
    if (numCh <= 0 || baseLen < 16) return;

    constexpr int kHalfTaps = 15;
    constexpr int kNumTaps = kHalfTaps * 2 + 1; // 31 taps
    std::array<float, kNumTaps> kernel {};
    {
        constexpr double cutoff = 0.45; // Nyquist比 (fs/4 を保護)
        double sum = 0.0;
        for (int n = 0; n < kNumTaps; ++n)
        {
            const int k = n - kHalfTaps;
            const double sinc = (k == 0) ? cutoff
                : std::sin(juce::MathConstants<double>::pi * cutoff * (double)k)
                  / (juce::MathConstants<double>::pi * (double)k);
            const double w = 0.42
                           - 0.50 * std::cos(2.0 * juce::MathConstants<double>::pi * (double)n / (double)(kNumTaps - 1))
                           + 0.08 * std::cos(4.0 * juce::MathConstants<double>::pi * (double)n / (double)(kNumTaps - 1));
            const double coeff = sinc * w;
            kernel[(size_t)n] = (float)coeff;
            sum += coeff;
        }
        const float invSum = (float)(1.0 / (sum > 1.0e-12 ? sum : 1.0));
        for (float& c : kernel) c *= invSum;
    }

    const juce::AudioBuffer<float>* prev = &base;
    for (int lvl = 0; lvl < kExtraLevels; ++lvl)
    {
        const int prevLen = prev->getNumSamples();
        const int nextLen = prevLen / 2;
        if (nextLen < 8) break;

        auto& dst = downsampled[(size_t)lvl];
        dst.setSize(numCh, nextLen, false, false, false);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* in = prev->getReadPointer(ch);
            float* out = dst.getWritePointer(ch);

            for (int j = 0; j < nextLen; ++j)
            {
                const int center = j * 2;
                float acc = 0.0f;
                if (center >= kHalfTaps && center + kHalfTaps < prevLen)
                {
                    const float* pIn = in + (center - kHalfTaps);
                    for (int t = 0; t < kNumTaps; ++t)
                        acc += pIn[t] * kernel[(size_t)t];
                }
                else
                {
                    for (int t = 0; t < kNumTaps; ++t)
                    {
                        const int idx = juce::jlimit(0, prevLen - 1, center + t - kHalfTaps);
                        acc += in[idx] * kernel[(size_t)t];
                    }
                }
                out[j] = acc;
            }
        }

        numExtraLevels = lvl + 1;
        prev = &dst;
    }
}

// ------------------------------------------
// Concatenative Grain Atlas (Breakthrough 1)
// ------------------------------------------
void GrainAtlas::buildFrom(const juce::AudioBuffer<float>& buf, double sr, int trimStart, int trimEnd)
{
    clear();
    const int numCh  = buf.getNumChannels();
    const int bufLen = buf.getNumSamples();
    if (numCh <= 0 || bufLen < 16) return;

    const double safeSr = sr > 1000.0 ? sr : 44100.0;
    const int ts = juce::jlimit(0, bufLen - 1, trimStart);
    const int te = juce::jlimit(ts + 1, bufLen, trimEnd);
    const int validSpan = te - ts;
    if (validSpan < 16) return;

    const float* lPtr = buf.getReadPointer(0);
    const float* rPtr = (numCh > 1) ? buf.getReadPointer(1) : lPtr;

    int hop = juce::jmax(64, juce::jmax((int)(0.015 * safeSr), validSpan / 2048));
    if (validSpan < 2048)
        hop = juce::jmax(4, validSpan / 16);

    hopSamples   = hop;
    trimStartRef = ts;

    const int winLen  = juce::jlimit(16, 1536, validSpan);
    const int halfWin = winLen / 2;
    const int zRad    = juce::jmin(256, juce::jmax(8, validSpan / 4));

    const int estFrames = (validSpan + hop - 1) / hop;
    frames.reserve((size_t)juce::jlimit(1, 2048, estFrames));

    std::vector<float> rawTimbres;
    rawTimbres.reserve((size_t)juce::jlimit(1, 2048, estFrames));

    std::vector<float> monoWin((size_t)winLen, 0.0f);
    float prevRms = 0.0f;
    float minTimbre = 1.0e9f;
    float maxTimbre = -1.0e9f;

    for (int pos = ts; pos < te && (int)frames.size() < 2048; pos += hop)
    {
        const int wStart = juce::jlimit(ts, te - winLen, pos - halfWin);

        double sqSum = 0.0;
        double absSum = 0.0;
        double diffSum = 0.0;
        int zcCount = 0;
        float prevS = 0.0f;

        for (int k = 0; k < winLen; ++k)
        {
            const int idx = wStart + k;
            const float m = 0.5f * (lPtr[idx] + rPtr[idx]);
            monoWin[(size_t)k] = m;
            sqSum += (double)m * (double)m;
            absSum += (double)std::abs(m);
            if (k > 0)
            {
                diffSum += (double)std::abs(m - prevS);
                if ((prevS <= 0.0f && m > 0.0f) || (prevS >= 0.0f && m < 0.0f))
                    ++zcCount;
            }
            prevS = m;
        }

        const float rms = (float)std::sqrt(sqSum / (double)winLen);
        const float hfRatio = (float)(diffSum / (absSum + 1.0e-6));
        const float zcrNorm = (float)zcCount / (float)juce::jmax(1, winLen - 1);
        const float rawT = 0.55f * (hfRatio / (0.25f + hfRatio)) + 0.45f * juce::jlimit(0.0f, 1.0f, zcrNorm * 3.5f);

        minTimbre = juce::jmin(minTimbre, rawT);
        maxTimbre = juce::jmax(maxTimbre, rawT);
        rawTimbres.push_back(rawT);

        // Positive flux onset score
        const float deltaRms = rms - prevRms;
        const float onset = (deltaRms > 0.0f) ? juce::jlimit(0.0f, 1.0f, deltaRms / (rms + prevRms + 1.0e-4f) * 1.6f) : 0.0f;
        prevRms = rms;

        // Nearest upward zero-crossing near pos inside [ts, te - 1]
        int bestZC = juce::jlimit(ts, te - 1, pos);
        int bestZDist = zRad + 1;
        const int zMin = juce::jmax(ts + 1, pos - zRad);
        const int zMax = juce::jmin(te - 1, pos + zRad);
        for (int sIdx = zMin; sIdx <= zMax; ++sIdx)
        {
            const float curM  = 0.5f * (lPtr[sIdx] + rPtr[sIdx]);
            const float prevM = 0.5f * (lPtr[sIdx - 1] + rPtr[sIdx - 1]);
            if (prevM <= 0.0f && curM > 0.0f)
            {
                const int d = std::abs(sIdx - pos);
                if (d < bestZDist) { bestZDist = d; bestZC = sIdx; }
            }
        }

        // Fast normalized autocorrelation F0 estimation
        float localMidi = 60.0f;
        bool hasPitch = false;
        const int corrN = juce::jmin(winLen, 1024);
        if (rms > 1.0e-4f && corrN >= 64)
        {
            const int minLag = juce::jmax(4, (int)(safeSr / 2000.0));
            const int maxLag = juce::jmin(corrN / 2 - 1, (int)(safeSr / 60.0));
            if (maxLag > minLag + 2)
            {
                const int step = (corrN > 512) ? 2 : 1;
                float globalMax = 0.0f;
                int globalTau = minLag;
                float firstStrong = 0.0f;
                int firstTau = -1;
                float prevC = 0.0f;
                bool rising = false;

                for (int tau = minLag; tau <= maxLag; ++tau)
                {
                    double cross = 0.0, e0 = 0.0, e1 = 0.0;
                    const int len = corrN - tau;
                    for (int n = 0; n < len; n += step)
                    {
                        const float x0 = monoWin[(size_t)n];
                        const float x1 = monoWin[(size_t)(n + tau)];
                        cross += (double)x0 * (double)x1;
                        e0    += (double)x0 * (double)x0;
                        e1    += (double)x1 * (double)x1;
                    }
                    const float c = (float)(cross / std::sqrt(e0 * e1 + 1.0e-12));
                    if (c > globalMax) { globalMax = c; globalTau = tau; }

                    if (tau > minLag)
                    {
                        if (rising && c < prevC && prevC > 0.65f && firstTau < 0)
                        {
                            firstStrong = prevC;
                            firstTau = tau - 1;
                        }
                        rising = (c > prevC);
                    }
                    prevC = c;
                }

                const int chosenTau = (firstTau >= minLag && firstStrong >= globalMax * 0.85f) ? firstTau : globalTau;
                const float chosenCorr = (firstTau >= minLag && firstStrong >= globalMax * 0.85f) ? firstStrong : globalMax;
                if (chosenCorr > 0.58f)
                {
                    auto calcCorr = [&](int tau) -> double
                    {
                        if (tau < 2 || tau >= corrN - 2) return 0.0;
                        double cross = 0.0, e0 = 0.0, e1 = 0.0;
                        const int len = corrN - tau;
                        for (int n = 0; n < len; ++n)
                        {
                            const float x0 = monoWin[(size_t)n];
                            const float x1 = monoWin[(size_t)(n + tau)];
                            cross += (double)x0 * (double)x1;
                            e0    += (double)x0 * (double)x0;
                            e1    += (double)x1 * (double)x1;
                        }
                        return cross / std::sqrt(e0 * e1 + 1.0e-12);
                    };

                    double lagF = (double)chosenTau;
                    if (chosenTau > minLag && chosenTau < maxLag)
                    {
                        const double ym = calcCorr(chosenTau - 1);
                        const double y0 = calcCorr(chosenTau);
                        const double yp = calcCorr(chosenTau + 1);
                        const double denom = 2.0 * (ym - 2.0 * y0 + yp);
                        if (std::abs(denom) > 1.0e-9)
                            lagF += juce::jlimit(-0.5, 0.5, (ym - yp) / denom);
                    }

                    const double freq = safeSr / juce::jmax(2.0, lagF);
                    const float midiF = (float)(69.0 + 12.0 * std::log2(freq / 440.0));
                    if (midiF >= 20.0f && midiF <= 115.0f)
                    {
                        localMidi = midiF;
                        hasPitch = true;
                    }
                }
            }
        }

        AtlasFrame f;
        f.samplePos    = pos;
        f.zeroCrossPos = bestZC;
        f.rms          = rms;
        f.onsetScore   = onset;
        f.localMidi    = localMidi;
        f.hasPitch     = hasPitch;
        frames.push_back(f);
    }

    const int numFrames = (int)frames.size();
    if (numFrames <= 0) return;

    const float spanT = maxTimbre - minTimbre;
    for (int i = 0; i < numFrames; ++i)
    {
        if (spanT > 0.02f)
            frames[(size_t)i].timbre = juce::jlimit(0.0f, 1.0f, (rawTimbres[(size_t)i] - minTimbre) / spanT);
        else
            frames[(size_t)i].timbre = 0.5f;
    }

    timbreOrder.resize((size_t)numFrames);
    for (int i = 0; i < numFrames; ++i) timbreOrder[(size_t)i] = i;
    std::sort(timbreOrder.begin(), timbreOrder.end(), [this](int a, int b)
    {
        return frames[(size_t)a].timbre < frames[(size_t)b].timbre;
    });

    valid = true;
}

const AtlasFrame* GrainAtlas::findBestFrame(float targetTimbre, int trimStart, int trimEnd, uint32_t seed) const noexcept
{
    if (!valid || timbreOrder.empty()) return nullptr;

    const int n = (int)timbreOrder.size();
    const float t = juce::jlimit(0.0f, 1.0f, targetTimbre);

    auto it = std::lower_bound(timbreOrder.begin(), timbreOrder.end(), t,
        [this](int frameIdx, float target) { return frames[(size_t)frameIdx].timbre < target; });
    const int center = juce::jlimit(0, n - 1, (int)(it - timbreOrder.begin()));

    // Bounded local search around binary search hit for valid frames inside [trimStart, trimEnd)
    int bestIdx = -1;
    float bestDiff = 1.0e9f;

    const int lo = juce::jmax(0, center - 16);
    const int hi = juce::jmin(n - 1, center + 16);
    for (int i = lo; i <= hi; ++i)
    {
        const int fIdx = timbreOrder[(size_t)i];
        const auto& f = frames[(size_t)fIdx];
        if (f.samplePos < trimStart || f.samplePos >= trimEnd) continue;

        const float d = std::abs(f.timbre - t);
        if (d < bestDiff)
        {
            bestDiff = d;
            bestIdx = fIdx;
        }
    }

    if (bestIdx >= 0)
    {
        std::array<int, 4> closeCandidates {};
        int closeCount = 0;
        for (int i = lo; i <= hi && closeCount < 4; ++i)
        {
            const int fIdx = timbreOrder[(size_t)i];
            const auto& f = frames[(size_t)fIdx];
            if (f.samplePos < trimStart || f.samplePos >= trimEnd) continue;
            if (std::abs(f.timbre - t) <= bestDiff + 0.015f)
                closeCandidates[(size_t)closeCount++] = fIdx;
        }
        if (closeCount > 1)
            return &frames[(size_t)closeCandidates[(size_t)(seed % (uint32_t)closeCount)]];
        return &frames[(size_t)bestIdx];
    }

    // Expanding fallback if [trimStart, trimEnd) excludes all frames near `center`
    for (int step = 17; step < n; ++step)
    {
        for (int dir = -1; dir <= 1; dir += 2)
        {
            const int i = center + dir * step;
            if (i < 0 || i >= n) continue;
            const int fIdx = timbreOrder[(size_t)i];
            const auto& f = frames[(size_t)fIdx];
            if (f.samplePos >= trimStart && f.samplePos < trimEnd)
                return &f;
        }
    }
    return &frames[(size_t)timbreOrder[(size_t)center]];
}

const AtlasFrame* GrainAtlas::findNearestPosFrame(double pos, int trimStart, int trimEnd) const noexcept
{
    if (!valid || frames.empty()) return nullptr;
    const int n = (int)frames.size();
    const int est = juce::jlimit(0, n - 1, (int)std::round((pos - (double)trimStartRef) / (double)juce::jmax(1, hopSamples)));
    const auto& f = frames[(size_t)est];
    if (f.samplePos >= trimStart && f.samplePos < trimEnd)
        return &f;
    return nullptr;
}

// ==========================================
// GranularVoice
// ==========================================
void GranularVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    env.setSampleRate(sampleRate);
    voiceInternalBuffer.setSize(2, samplesPerBlock, false, true, true);
    random.setSeed((uint32_t)(reinterpret_cast<uintptr_t>(this) & 0xFFFFFFFF));

    // TONE用コムバッファ (最低15Hzまで対応)
    const int combSize = (int)(sampleRate / 15.0) + 8;
    combBufL.assign((size_t)combSize, 0.0f);
    combBufR.assign((size_t)combSize, 0.0f);
    combWriteIdx = 0;
    combLpL = combLpR = 0.0f;
    combApXPrevL = combApYPrevL = 0.0f;
    combApXPrevR = combApYPrevR = 0.0f;
    combDcXPrevL = combDcYPrevL = 0.0f;
    combDcXPrevR = combDcYPrevR = 0.0f;
}

void GranularVoice::releaseResources()
{
    voiceInternalBuffer.setSize(0, 0);
}

void GranularVoice::startNote(int midiNote, float velocity, uint64_t stamp, const GranularParams& p)
{
    const bool wasPlaying = playing; // ボイススチール(発音中の再スタート)かどうか

    currentMidiNote = midiNote;
    playing = true;
    heldDown = true;
    noteStamp = stamp;
    velocityGain = 0.25f + 0.75f * juce::jlimit(0.0f, 1.0f, velocity);
    playheadOffset = 0.0;
    smoothedPos01 = (double)p.position; // ノート開始位置は即時（グライドは保持中のノブ移動のみ）
    samplesToNextSpawn = 0.0; // 即時に最初のグレインを発火
    randomJumpCountdown = 0.0; // Random: ノート頭で必ず1回抽選する
    wsolaExpectedNextPos = -1.0;

    if (!wasPlaying)
    {
        // 空きボイスからの新規発音: 停止時に凍結した古いグレインを掃除して清潔に開始
        for (auto& g : grains) g.isActive = false;
        std::fill(combBufL.begin(), combBufL.end(), 0.0f);
        std::fill(combBufR.begin(), combBufR.end(), 0.0f);
        combLpL = combLpR = 0.0f;
        combApXPrevL = combApYPrevL = 0.0f;
        combApXPrevR = combApYPrevR = 0.0f;
        combDcXPrevL = combDcYPrevL = 0.0f;
        combDcXPrevR = combDcYPrevR = 0.0f;
        currentCombDelay = 0.0f;
    }
    // ボイススチール時は古いグレインを即kill「しない」。
    //   → 古い粒は各自の窓で自然に減衰し、新しい粒が立ち上がるので実質クロスフェード。
    //     env も現在値から継続するため、Reverbを励起する広帯域クリックが出ない。

    env.setParameters(p.attack, p.decay, p.sustain, p.release);
    env.noteOn();
}

void GranularVoice::stopNote(bool allowTailOff)
{
    heldDown = false;
    if (allowTailOff) env.noteOff();
    else clearCurrentNote();
}

void GranularVoice::clearCurrentNote()
{
    playing = false;
    heldDown = false;
    currentMidiNote = -1;
    env.reset();
}

void GranularVoice::reset()
{
    clearCurrentNote();
    for (auto& g : grains) g.isActive = false;
    std::fill(combBufL.begin(), combBufL.end(), 0.0f);
    std::fill(combBufR.begin(), combBufR.end(), 0.0f);
    combLpL = combLpR = 0.0f;
    combApXPrevL = combApYPrevL = 0.0f;
    combApXPrevR = combApYPrevR = 0.0f;
    combDcXPrevL = combDcYPrevL = 0.0f;
    combDcXPrevR = combDcYPrevR = 0.0f;
    currentCombDelay = 0.0f;
    playheadOffset = 0.0;
    samplesToNextSpawn = 0.0;
    randomJumpCountdown = 0.0;
    currentSprayTarget = 0.0;
    currentSprayValue = 0.0;
    lastRelNorm = -1.0f;
    wsolaExpectedNextPos = -1.0;
}

namespace
{
    double wrapMirroredPos(double rawPos, double minPos, double maxPos) noexcept
    {
        const double range = maxPos - minPos;
        if (range <= 0.0) return minPos;
        double t = std::fmod(rawPos - minPos, 2.0 * range);
        if (t < 0.0) t += 2.0 * range;
        return (t <= range) ? (minPos + t) : (minPos + 2.0 * range - t);
    }

    double findWsolaAlignedPos(const juce::AudioBuffer<float>* src,
                               double nominalPos,
                               double refPos,
                               int trimStart, int trimEnd,
                               int corrLen, int searchRadius) noexcept
    {
        if (src == nullptr || corrLen < 8 || searchRadius < 2)
            return nominalPos;

        const int len = src->getNumSamples();
        const int ts  = juce::jlimit(0, len - 1, trimStart);
        const int te  = juce::jlimit(ts + 1, len, trimEnd);
        if (te - ts < corrLen + 8) return nominalPos;

        const float* l = src->getReadPointer(0);
        const float* r = src->getNumChannels() > 1 ? src->getReadPointer(1) : l;

        const int refIdx = juce::jlimit(ts, te - corrLen - 1, (int)std::round(refPos));
        const int nomIdx = juce::jlimit(ts, te - corrLen - 1, (int)std::round(nominalPos));
        const int minCand = juce::jmax(ts, nomIdx - searchRadius);
        const int maxCand = juce::jmin(te - corrLen - 1, nomIdx + searchRadius);
        if (minCand >= maxCand) return nominalPos;

        double refEnergy = 0.0;
        for (int k = 0; k < corrLen; k += 2)
        {
            const float x = 0.5f * (l[refIdx + k] + r[refIdx + k]);
            refEnergy += (double)x * (double)x;
        }
        if (refEnergy < 1.0e-10) return nominalPos;

        auto scoreAt = [&](int candIdx, int stride) noexcept -> double
        {
            double cross = 0.0, candE = 0.0;
            for (int k = 0; k < corrLen; k += stride)
            {
                const float xRef  = 0.5f * (l[refIdx + k]  + r[refIdx + k]);
                const float xCand = 0.5f * (l[candIdx + k] + r[candIdx + k]);
                cross += (double)xRef  * (double)xCand;
                candE += (double)xCand * (double)xCand;
            }
            return cross / std::sqrt(refEnergy * candE + 1.0e-12);
        };

        int bestIdx = nomIdx;
        double bestScore = -1.0e9;
        for (int c = minCand; c <= maxCand; c += 2)
        {
            const double s = scoreAt(c, 2);
            if (s > bestScore) { bestScore = s; bestIdx = c; }
        }

        // Fine +-1 sample refinement around coarse winner
        double fineBest = -1.0e9;
        int fineWinner = bestIdx;
        for (int d = -2; d <= 2; ++d)
        {
            const int c = juce::jlimit(minCand, maxCand, bestIdx + d);
            const double s = scoreAt(c, 1);
            if (s > fineBest) { fineBest = s; fineWinner = c; }
        }

        const double fracOffset = nominalPos - std::floor(nominalPos);
        return juce::jlimit((double)ts, (double)(te - 1), (double)fineWinner + fracOffset);
    }
}

void GranularVoice::spawnGrain(int trimStart, int trimEnd,
                               double currentPlayhead, const GranularParams& p,
                               float overlap, int baseLife, double expectedInterval,
                               const juce::AudioBuffer<float>* sharedSourceBuffer,
                               GrainVisualizerData* visualizer, int& spawnBudget,
                               const GrainAtlas* atlas, GrainSwarmScheduler* swarm)
{
    const int validLength = trimEnd - trimStart;
    if (spawnBudget <= 0 || validLength <= 0) return; // 全体バジェット超過・無効領域ガード

    Grain* slot = nullptr;
    for (auto& g : grains)
        if (!g.isActive) { slot = &g; break; }
    if (slot == nullptr) return; // 全スロット使用中

    Grain& grain = *slot;
    grain.isActive = true;
    grain.age = 0;
    grain.trimStart = (double)trimStart;
    grain.trimEnd = (double)trimEnd;

    const double minPos = (double)trimStart;
    const double maxPos = (double)trimEnd - 1.0;

    // --------------------------------------
    // Concatenative Atlas: Timbre Scrubbing & Position Selection
    // --------------------------------------
    const AtlasFrame* bestTimbreFrame = nullptr;
    double basePlayhead = currentPlayhead;
    if (atlas != nullptr && atlas->valid && p.timbreAmt > 0.001f)
    {
        bestTimbreFrame = atlas->findBestFrame(p.timbreScrub, trimStart, trimEnd, (uint32_t)random.nextInt());
        if (bestTimbreFrame != nullptr)
        {
            const double atlasTarget = (double)bestTimbreFrame->zeroCrossPos;
            basePlayhead = currentPlayhead * (1.0 - (double)p.timbreAmt) + atlasTarget * (double)p.timbreAmt;
        }
    }

    const double sprayOffset = currentSprayValue * (double)validLength * (double)p.positionSpray;
    const double rawPos = basePlayhead + sprayOffset;
    grain.position = wrapMirroredPos(rawPos, minPos, maxPos);

    // Zero-Cross Snap & Adaptive Onset Lookup
    const AtlasFrame* posAtlas = (bestTimbreFrame != nullptr && p.timbreAmt > 0.5f)
        ? bestTimbreFrame
        : ((atlas != nullptr && atlas->valid) ? atlas->findNearestPosFrame(grain.position, trimStart, trimEnd) : nullptr);

    double onsetSkewBias = 0.0;
    if (posAtlas != nullptr)
    {
        if ((p.timbreAmt > 0.01f || posAtlas->onsetScore > 0.35f || p.windowShape == 3 || p.engineMode == 2)
            && std::abs(grain.position - (double)posAtlas->zeroCrossPos) <= 160.0)
        {
            grain.position = juce::jlimit(minPos, maxPos, (double)posAtlas->zeroCrossPos);
        }
        if (posAtlas->onsetScore > 0.45f)
            onsetSkewBias = -(double)posAtlas->onsetScore * 0.35;
    }

    // --------------------------------------
    // Pitch Calculation + Per-Grain Atlas F0 Lock + Diatonic Swarm + Quantize
    // --------------------------------------
    const int scaleRoot = (p.key == 0) ? p.detectedRoot : (p.key - 1); // 0..11
    const float globalSemis = (float)p.octave * 12.0f + (float)p.pitchSt
                            + p.fineTune / 100.0f + p.pitchMod;
    const float jitterSemis = ((random.nextFloat() * 2.0f - 1.0f) * p.pitchJitter) / 100.0f;

    float baseSemis = (float)currentMidiNote + globalSemis + jitterSemis;
    bool isSwarmHarmonic = false;
    const float swarmOff = GrainSwarmScheduler::computeSwarmInterval(
        p.swarmMix, scaleRoot, p.scale, baseSemis, random, isSwarmHarmonic);
    float totalAbsSemis = baseSemis + swarmOff;

    if (p.scaleOn || isSwarmHarmonic)
        totalAbsSemis = ScaleQuantizer::quantize(totalAbsSemis, scaleRoot, p.scale);

    // --------------------------------------
    // Swarm Collision-Avoidance Repulsion across voices
    // --------------------------------------
    if (swarm != nullptr && validLength > 32)
    {
        const double range = juce::jmax(1.0, maxPos - minPos);
        const float normPos = (float)juce::jlimit(0.0, 1.0, (grain.position - minPos) / range);
        const bool allowRepel = (p.engineMode == 0)
                             && (p.density > 0.25f || p.swarmMix > 0.01f || p.positionSpray > 0.005f);
        const float resolvedNorm = swarm->resolveCollisionAndRecord(normPos, totalAbsSemis, 0.004f, allowRepel);
        if (allowRepel && std::abs(resolvedNorm - normPos) > 1.0e-6f)
            grain.position = juce::jlimit(minPos, maxPos, minPos + (double)resolvedNorm * range);
    }

    float effRootNote = (float)p.rootNote;
    if (posAtlas != nullptr && posAtlas->hasPitch && p.atlasPitchLock > 0.001f)
        effRootNote = effRootNote * (1.0f - p.atlasPitchLock) + posAtlas->localMidi * p.atlasPitchLock;

    const double pitchRatio = std::pow(2.0, ((double)totalAbsSemis - (double)effRootNote) / 12.0)
                            * (double)p.srComp;

    const double noteSemisForPeriod = (double)currentMidiNote + (double)globalSemis;
    const double noteFreqForPeriod = 440.0 * std::pow(2.0, (noteSemisForPeriod - 69.0) / 12.0);
    const double notePeriod = currentSampleRate / juce::jlimit(15.0, 8000.0, noteFreqForPeriod);

    // --------------------------------------
    // Engine Modes: 0=Granular, 1=WSOLA, 2=TD-PSOLA (+ Formant Shift)
    // --------------------------------------
    if (p.engineMode == 2) // TD-PSOLA: pitch period sets trigger interval; formantRatio sets internal step
    {
        const double formantRatio = std::pow(2.0, (double)p.formantShift / 12.0);
        const double dir = p.reverseGrain ? -1.0 : 1.0;
        const double srcMidi = (posAtlas != nullptr && posAtlas->hasPitch) ? (double)posAtlas->localMidi : (double)p.rootNote;
        const double srcHz   = juce::jlimit(30.0, 2000.0, 440.0 * std::pow(2.0, (srcMidi - 69.0) / 12.0));
        const double srcPeriod = currentSampleRate / srcHz;

        grain.life = juce::jlimit(48, (int)(currentSampleRate * 0.25),
                                  (int)std::round(2.0 * srcPeriod / juce::jlimit(0.35, 2.5, formantRatio)));
        grain.step = dir * formantRatio * (double)p.srComp;
        grain.mipLevel = (formantRatio > 1.0)
            ? juce::jlimit(0.0f, (float)SampleMipLevels::kExtraLevels, (float)std::log2(formantRatio))
            : 0.0f;
    }
    else
    {
        grain.life = p.pitchSync ? baseLife
                                 : baseLife + random.nextInt(juce::jmax(1, baseLife / 2));
        grain.step = p.reverseGrain ? -pitchRatio : pitchRatio;
        grain.mipLevel = (pitchRatio > 1.0)
            ? juce::jlimit(0.0f, (float)SampleMipLevels::kExtraLevels, (float)std::log2(pitchRatio))
            : 0.0f;

        if (p.engineMode == 1 && sharedSourceBuffer != nullptr) // WSOLA phase alignment
        {
            if (wsolaExpectedNextPos >= minPos && wsolaExpectedNextPos <= maxPos)
            {
                const int corrLen = juce::jmin(96, juce::jmax(16, validLength / 6));
                const int searchRadius = juce::jmin(384, juce::jmax(8, validLength / 4));
                grain.position = findWsolaAlignedPos(
                    sharedSourceBuffer, grain.position, wsolaExpectedNextPos,
                    trimStart, trimEnd, corrLen, searchRadius);
            }
            const double rawNext = grain.position + grain.step * juce::jmax(8.0, expectedInterval);
            wsolaExpectedNextPos = wrapMirroredPos(rawNext, minPos, maxPos);
        }
    }

    // Per-grain pitch chirp (start -> end glide across grain life)
    grain.stepStart = grain.step;
    if (std::abs(p.grainChirp) > 1.0e-3f)
        grain.stepEnd = grain.step * std::pow(2.0, (double)p.grainChirp / 12.0);
    else
        grain.stepEnd = grain.step;

    // Window Skew + Onset Adaptive bias
    grain.skew = juce::jlimit(-0.95f, 0.95f, p.grainSkew + (float)onsetSkewBias);

    // Per-Grain Zavalishin TPT SVF coefficients
    grain.ic1L = grain.ic2L = grain.ic1R = grain.ic2R = 0.0f;
    grain.svfMode = juce::jlimit(0, 4, p.grainFilterMode);
    if (grain.svfMode > 0)
    {
        const float randOct = (random.nextFloat() * 2.0f - 1.0f) * p.grainFilterJitter * 3.0f;
        const float fc = juce::jlimit(20.0f, (float)(currentSampleRate * 0.44),
                                      p.grainFilterFreq * std::pow(2.0f, randOct));
        const float q  = juce::jlimit(0.1f, 10.0f, p.grainFilterRes);
        const float g  = std::tan(juce::MathConstants<float>::pi * fc / (float)currentSampleRate);
        grain.svfK  = 1.0f / q;
        grain.svfA1 = 1.0f / (1.0f + g * (g + grain.svfK));
        grain.svfA2 = g * grain.svfA1;
        grain.svfA3 = g * grain.svfA2;
    }

    // --------------------------------------
    // Pan & Amplitude Normalization
    // --------------------------------------
    panSide = -panSide;
    float panOffset = random.nextFloat() * 0.5f * p.panSpread * panSide;
    if (isSwarmHarmonic)
        panOffset *= (1.0f + 0.5f * p.swarmMix);
    const float pan = juce::jlimit(0.0f, 1.0f, 0.5f + panOffset);
    grain.panL = std::sqrt(1.0f - pan);
    grain.panR = std::sqrt(pan);

    float gainDivisor = std::sqrt(juce::jmax(1.0f, overlap));
    if (p.engineMode == 2) // TD-PSOLA overlap-add normalization
    {
        const float psolaOverlap = (float)grain.life / (float)juce::jmax(4.0, notePeriod);
        gainDivisor = juce::jmax(1.0f, 0.82f * psolaOverlap);
    }
    else if (p.pitchSync)
    {
        const int periods = juce::jlimit(2, 16, juce::roundToInt((double)baseLife / notePeriod));
        gainDivisor = juce::jmax(1.0f, 1.25f * (float)periods);
    }
    grain.amp = velocityGain / gainDivisor;

    --spawnBudget;

    // GUIへ発火イベントを通知（ロックフリー）
    if (visualizer != nullptr && validLength > 0)
    {
        const float normPos = (float)((grain.position - (double)trimStart) / (double)validLength);
        visualizer->push(juce::jlimit(0.0f, 1.0f, normPos), pan,
                         totalAbsSemis - effRootNote,
                         (float)grain.life / (float)currentSampleRate);
    }
}

void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                    const juce::AudioBuffer<float>* sharedSourceBuffer,
                                    const SampleMipLevels* mipLevels,
                                    int trimStart, int trimEnd,
                                    const std::array<std::array<float, 4096>, 4>& windowTables,
                                    const GranularParams& p,
                                    GrainVisualizerData* visualizer,
                                    int& spawnBudget,
                                    const GrainAtlas* atlas,
                                    GrainSwarmScheduler* swarm)
{
    if (!env.isActive() || sharedSourceBuffer == nullptr)
    {
        if (playing) clearCurrentNote();
        return;
    }

    // 演奏中・リリース中のADSRノブ操作をリアルタイム反映
    env.setParameters(p.attack, p.decay, p.sustain, p.release);

    const int numSamples = outputBuffer.getNumSamples();
    const int validLength = trimEnd - trimStart;
    if (validLength <= 0 || numSamples <= 0) return; // マイクロ素材(>0サンプル)も再生可能
    if (numSamples > voiceInternalBuffer.getNumSamples())
        voiceInternalBuffer.setSize(2, numSamples, false, true, true);

    voiceInternalBuffer.clear();
    const int srcLen = sharedSourceBuffer->getNumSamples();
    const float* srcL = sharedSourceBuffer->getReadPointer(0);
    const float* srcR = sharedSourceBuffer->getNumChannels() > 1 ? sharedSourceBuffer->getReadPointer(1) : srcL;

    struct MipView { const float* l = nullptr; const float* r = nullptr; int len = 0; double scale = 1.0; };
    std::array<MipView, SampleMipLevels::kExtraLevels + 1> mips;
    mips[0] = { srcL, srcR, srcLen, 1.0 };
    int maxMipLvl = 0;
    if (mipLevels != nullptr)
    {
        const int avail = juce::jlimit(0, SampleMipLevels::kExtraLevels, mipLevels->numExtraLevels);
        for (int m = 0; m < avail; ++m)
        {
            const auto& b = mipLevels->downsampled[(size_t)m];
            if (b.getNumSamples() > 0 && b.getNumChannels() > 0)
            {
                const int mIdx = m + 1;
                mips[(size_t)mIdx].l = b.getReadPointer(0);
                mips[(size_t)mIdx].r = b.getNumChannels() > 1 ? b.getReadPointer(1) : mips[(size_t)mIdx].l;
                mips[(size_t)mIdx].len = b.getNumSamples();
                mips[(size_t)mIdx].scale = 1.0 / (double)(1 << mIdx);
                maxMipLvl = mIdx;
            }
            else break;
        }
    }

    // ノート周波数（オクターブ/セミ/ファイン/MOD込み、ジッター前）
    const double globalSemis = (double)p.octave * 12.0 + (double)p.pitchSt
                             + (double)p.fineTune / 100.0 + (double)p.pitchMod;
    const double noteFreq = 440.0 * std::pow(2.0, ((double)currentMidiNote + globalSemis - 69.0) / 12.0);
    const double clampedNoteFreq = juce::jlimit(15.0, 8000.0, noteFreq);
    const double notePeriod = currentSampleRate / clampedNoteFreq;

    // 決定論的スケジューラ:
    //   density → 同時オーバーラップ量 指数カーブ 0.25..64 (100グレイン/ボイス上限)
    //   interval = grainLife / overlap （±25% のゆらぎ付き）
    //   Pitch-Sync時: interval = ノート周期（小数アキュムレータで厳密同期）
    int baseLife = juce::jmax(64, (int)(currentSampleRate * (double)p.grainSize));
    const float overlap = 0.25f * std::pow(2.0f, p.density * 8.0f); // 0.25 .. 64

    if (p.pitchSync)
    {
        const int periods = juce::jlimit(2, 16, juce::roundToInt((double)baseLife / notePeriod));
        baseLife = juce::jmax(64, (int)std::round((double)periods * notePeriod));
    }

    const size_t shapeIdx = (size_t)juce::jlimit(0, 3, p.windowShape);
    const auto& window = windowTables[shapeIdx];

    // S-SYNC の音価テーブル (拍数): 1Beat/2Beats/1Bar/2Bars/4Bars/8Bars (4/4想定)
    static const double beatsTable[6] = { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 };
    const double bpmSafe = p.bpm > 1.0 ? p.bpm : 120.0;
    const double syncSecs = (60.0 / bpmSafe) * beatsTable[juce::jlimit(0, 5, p.speedSyncDiv)];

    // 実効プレイヘッド速度（S-SYNC時: 領域を指定音価でスキャンする速度）
    double speedMag;
    if (p.speedSync)
        speedMag = (double)validLength / juce::jmax(1.0, syncSecs * currentSampleRate);
    else
        speedMag = std::abs((double)p.speed) * (double)p.srComp;

    // Randomモード: ジャンプ間隔（サンプル）
    constexpr double kRandomJumpsPerSecAtUnitSpeed = 30.0;
    double randomJumpInterval = 0.0;
    if (p.playMode == 3)
    {
        if (p.speedSync)
            randomJumpInterval = syncSecs * currentSampleRate;
        else if (std::abs(p.speed) > 0.001f)
            randomJumpInterval = currentSampleRate
                               / (kRandomJumpsPerSecAtUnitSpeed * std::abs((double)p.speed));

        if (randomJumpInterval > 0.0)
        {
            randomJumpInterval = juce::jmax(32.0, randomJumpInterval);
            if (randomJumpCountdown > randomJumpInterval)
                randomJumpCountdown = randomJumpInterval;
        }
    }

    // TONE: ノート同調コムレゾネーター (SR補償LPF + 位相遅延精密補償 + Thiran AP + 5Hz DCブロッカー)
    const bool  toneOn = p.tone > 0.001f && !combBufL.empty();
    const int   combSize = (int)combBufL.size();
    const double dampCutoffHz = juce::jmin(currentSampleRate * 0.42, 3200.0);
    const float combDamp = 1.0f - (float)std::exp(-juce::MathConstants<double>::twoPi * dampCutoffHz / currentSampleRate);
    const double w0 = juce::MathConstants<double>::twoPi * clampedNoteFreq / currentSampleRate;
    double lpPhaseDelaySamples = 0.0;
    if (w0 > 1.0e-4)
    {
        const double pole = 1.0 - (double)combDamp;
        const double lpPhase = std::atan2(pole * std::sin(w0), 1.0 - pole * std::cos(w0));
        lpPhaseDelaySamples = lpPhase / w0;
    }
    else
    {
        lpPhaseDelaySamples = (1.0 - (double)combDamp) / juce::jmax(1.0e-5, (double)combDamp);
    }

    const float targetCombDelay = (float)juce::jlimit(1.5, (double)(combSize - 4), notePeriod - lpPhaseDelaySamples);
    if (currentCombDelay <= 0.0f) currentCombDelay = targetCombDelay; // 初回
    const float combGlideCoef = 1.0f - (float)std::exp(-1.0 / (0.0045 * currentSampleRate));
    const float combFb = 0.55f + 0.40f * p.tone;
    const float combDcR = 1.0f - (float)(juce::MathConstants<double>::twoPi * 5.0 / currentSampleRate);
    const float toneMix = p.tone;

    const double L = (double)validLength;
    const double normStep = speedMag / L;
    const double sprayStepPerSample = juce::jmax(0.01, (double)p.spraySpeed) * 40.0 / currentSampleRate;
    const double edgeFadeLen = (L >= 16.0) ? juce::jmax(1.0, juce::jmin(L * 0.15, currentSampleRate * 0.003)) : 0.0;
    const double posCoef = 1.0 - std::exp(-1.0 / (0.02 * currentSampleRate)); // Positionグライド ~20ms
    double lastRel = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedPos01 += posCoef * ((double)p.position - smoothedPos01);

        // 時間ベースのランダムウォークスプレー（Density や SR に依存しない）
        if (std::abs(currentSprayTarget - currentSprayValue) <= sprayStepPerSample)
        {
            currentSprayValue = currentSprayTarget;
            currentSprayTarget = ((double)random.nextFloat() * 2.0 - 1.0);
        }
        else
        {
            currentSprayValue += (currentSprayTarget > currentSprayValue ? sprayStepPerSample : -sprayStepPerSample);
        }

        // 正規化プレイヘッド移動 (0..1 / PingPong 0..2 — トリムサイズ変更に耐性)
        double relNorm = 0.0;
        switch (p.playMode)
        {
        case 1: // Backward
        {
            double t = std::fmod(smoothedPos01 + playheadOffset, 1.0);
            if (t < 0.0) t += 1.0;
            relNorm = t;
            playheadOffset = std::fmod(playheadOffset - normStep, 1.0);
            if (playheadOffset < 0.0) playheadOffset += 1.0;
            break;
        }

        case 2: // Ping-Pong (三角折り返し)
        {
            double t = std::fmod(smoothedPos01 + playheadOffset, 2.0);
            if (t < 0.0) t += 2.0;
            relNorm = (t <= 1.0) ? t : (2.0 - t);
            playheadOffset = std::fmod(playheadOffset + normStep, 2.0);
            if (playheadOffset < 0.0) playheadOffset += 2.0;
            break;
        }

        case 3: // Random
            if (randomJumpCountdown <= 0.0)
            {
                randomTargetRel = (double)random.nextFloat();
                randomJumpCountdown = (randomJumpInterval > 0.0) ? randomJumpInterval : 1.0e18;
            }
            randomJumpCountdown -= 1.0;
            relNorm = randomTargetRel;
            break;

        case 0: // Forward
        default:
        {
            double t = std::fmod(smoothedPos01 + playheadOffset, 1.0);
            if (t < 0.0) t += 1.0;
            relNorm = t;
            playheadOffset = std::fmod(playheadOffset + normStep, 1.0);
            if (playheadOffset < 0.0) playheadOffset += 1.0;
            break;
        }
        }

        const double rel = relNorm * L;
        const double wrappedPlayhead = (double)trimStart + juce::jlimit(0.0, juce::jmax(0.0, L - 1.0), rel);
        lastRel = rel;

        // 小数アキュムレータによる位相同期グレイン発火スケジューリング
        if (samplesToNextSpawn <= 0.0)
        {
            const double interval = (p.pitchSync || p.engineMode == 2)
                ? juce::jmax(4.0, notePeriod)
                : juce::jmax(16.0, ((double)baseLife / (double)overlap) * (0.75 + (double)random.nextFloat() * 0.5));

            spawnGrain(trimStart, trimEnd, wrappedPlayhead, p, overlap, baseLife, interval,
                       sharedSourceBuffer, visualizer, spawnBudget, atlas, swarm);

            samplesToNextSpawn += interval;
            if (samplesToNextSpawn < -interval)
                samplesToNextSpawn = interval;
        }
        samplesToNextSpawn -= 1.0;

        // アクティブグレインのミックス（窓スキュー + Per-Grain SVF + Pitch Chirp）
        float mixL = 0.0f, mixR = 0.0f;
        for (auto& grain : grains)
        {
            if (!grain.isActive) continue;

            const float linearPhase = (float)grain.age / (float)juce::jmax(1, grain.life);
            float winPhase = linearPhase;
            if (std::abs(grain.skew) > 1.0e-3f)
            {
                const float expPow = std::pow(2.0f, juce::jlimit(-0.95f, 0.95f, grain.skew) * 2.0f);
                winPhase = std::pow(juce::jlimit(0.0f, 1.0f, linearPhase), expPow);
            }
            const float fIdx = juce::jlimit(0.0f, 4095.0f, winPhase * 4095.0f);
            const int idx0 = (int)fIdx;
            const int idx1 = juce::jmin(4095, idx0 + 1);
            const float winFrac = fIdx - (float)idx0;
            const float windowValue = window[(size_t)idx0] * (1.0f - winFrac) + window[(size_t)idx1] * winFrac;

            float sL = 0.0f, sR = 0.0f;
            if (maxMipLvl > 0 && grain.mipLevel > 1.0e-3f)
            {
                const float lvlF = juce::jmin(grain.mipLevel, (float)maxMipLvl);
                const int lvl0 = (int)lvlF;
                const int lvl1 = juce::jmin(maxMipLvl, lvl0 + 1);
                const float frac = lvlF - (float)lvl0;

                const auto& m0 = mips[(size_t)lvl0];
                const double pos0 = grain.position * m0.scale;
                const float l0 = readHermiteClamped(m0.l, m0.len, pos0);
                const float r0 = readHermiteClamped(m0.r, m0.len, pos0);

                if (lvl1 != lvl0 && frac > 1.0e-3f)
                {
                    const auto& m1 = mips[(size_t)lvl1];
                    const double pos1 = grain.position * m1.scale;
                    const float l1 = readHermiteClamped(m1.l, m1.len, pos1);
                    const float r1 = readHermiteClamped(m1.r, m1.len, pos1);
                    sL = l0 + (l1 - l0) * frac;
                    sR = r0 + (r1 - r0) * frac;
                }
                else
                {
                    sL = l0;
                    sR = r0;
                }
            }
            else
            {
                sL = readHermiteClamped(srcL, srcLen, grain.position);
                sR = readHermiteClamped(srcR, srcLen, grain.position);
            }

            // Per-Grain Zavalishin TPT SVF (窓前段で処理するためフィルタ過渡のクリックが出ない)
            if (grain.svfMode > 0)
            {
                auto runSvf = [&grain](float x, float& ic1, float& ic2) noexcept -> float
                {
                    const float v3 = x - ic2;
                    const float v1 = grain.svfA1 * ic1 + grain.svfA2 * v3;
                    const float v2 = ic2 + grain.svfA2 * ic1 + grain.svfA3 * v3;
                    ic1 = (std::abs(2.0f * v1 - ic1) < 1.0e-18f) ? 0.0f : (2.0f * v1 - ic1);
                    ic2 = (std::abs(2.0f * v2 - ic2) < 1.0e-18f) ? 0.0f : (2.0f * v2 - ic2);
                    switch (grain.svfMode)
                    {
                    case 1: return v2;                                 // LP
                    case 2: return v1;                                 // BP
                    case 3: return x - grain.svfK * v1 - v2;           // HP
                    case 4: return x - grain.svfK * v1;                // Notch
                    default: return x;
                    }
                };
                sL = runSvf(sL, grain.ic1L, grain.ic2L);
                sR = runSvf(sR, grain.ic1R, grain.ic2R);
            }

            float edgeFade = 1.0f;
            if (edgeFadeLen > 0.0)
            {
                const double distStart = grain.position - grain.trimStart;
                const double distEnd   = grain.trimEnd - grain.position;
                if (distStart < edgeFadeLen) edgeFade *= (float)juce::jlimit(0.0, 1.0, distStart / edgeFadeLen);
                if (distEnd   < edgeFadeLen) edgeFade *= (float)juce::jlimit(0.0, 1.0, distEnd   / edgeFadeLen);
            }

            mixL += sL * windowValue * grain.panL * grain.amp * edgeFade;
            mixR += sR * windowValue * grain.panR * grain.amp * edgeFade;

            const double curStep = (std::abs(grain.stepEnd - grain.stepStart) > 1.0e-7)
                ? (grain.stepStart + (grain.stepEnd - grain.stepStart) * (double)linearPhase)
                : grain.step;
            grain.position += curStep;
            const double grainL = grain.trimEnd - grain.trimStart;
            if (grainL > 0.0 && (grain.position >= grain.trimEnd || grain.position < grain.trimStart))
            {
                double t = std::fmod(grain.position - grain.trimStart, grainL);
                if (t < 0.0) t += grainL;
                grain.position = grain.trimStart + t;
            }

            if (++grain.age >= grain.life) grain.isActive = false;
        }

        // TONE: Thiran 1次オールパス小数遅延 + SR補償LPF + 5Hz DCブロッカー
        if (toneOn)
        {
            currentCombDelay += combGlideCoef * (targetCombDelay - currentCombDelay);
            const int intDelay = juce::jlimit(1, combSize - 4, (int)std::floor((double)currentCombDelay - 0.5));
            const float fracDelay = juce::jlimit(0.5f, 1.5f, currentCombDelay - (float)intDelay);
            const float apCoeff = (1.0f - fracDelay) / (1.0f + fracDelay);

            auto processComb = [combSize, intDelay, apCoeff, combFb, combDamp, combDcR](
                std::vector<float>& buf, int writeIdx, float in,
                float& lpState, float& apXPrev, float& apYPrev, float& dcXPrev, float& dcYPrev) noexcept
            {
                int readIdx = writeIdx - intDelay;
                if (readIdx < 0) readIdx += combSize;
                const float xNow = buf[(size_t)readIdx];
                const float delayed = apCoeff * xNow + apXPrev - apCoeff * apYPrev;
                apXPrev = xNow;
                apYPrev = (std::abs(delayed) < 1.0e-18f) ? 0.0f : delayed;

                lpState += combDamp * (delayed - lpState);
                if (std::abs(lpState) < 1.0e-18f) lpState = 0.0f;

                const float fbSig = lpState - dcXPrev + combDcR * dcYPrev;
                dcXPrev = lpState;
                dcYPrev = (std::abs(fbSig) < 1.0e-18f) ? 0.0f : fbSig;

                const float w = in + fbSig * combFb;
                buf[(size_t)writeIdx] = 1.5f * std::tanh(w * 0.6667f);
                return fbSig;
            };

            const float wetL = processComb(combBufL, combWriteIdx, mixL,
                                           combLpL, combApXPrevL, combApYPrevL, combDcXPrevL, combDcYPrevL);
            const float wetR = processComb(combBufR, combWriteIdx, mixR,
                                           combLpR, combApXPrevR, combApYPrevR, combDcXPrevR, combDcYPrevR);
            combWriteIdx = (combWriteIdx + 1) % combSize;

            mixL = mixL * (1.0f - toneMix * 0.5f) + wetL * toneMix;
            mixR = mixR * (1.0f - toneMix * 0.5f) + wetR * toneMix;
        }

        // PadEnvelope をサンプル単位で適用（コムの余韻もリリースに含まれる）
        const float envVal = env.getNextSample();
        voiceInternalBuffer.setSample(0, i, mixL * envVal);
        voiceInternalBuffer.setSample(1, i, mixR * envVal);
    }

    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom(ch, 0, voiceInternalBuffer, ch % 2, 0, numSamples);

    lastRelNorm = (float)(lastRel / L); // GUI用の実プレイヘッド位置
}

// ==========================================
// GranularSynth
// ==========================================
GranularSynth::GranularSynth()
{
    for (int i = 0; i < NUM_VOICES; ++i)
        voices[(size_t)i].setVoiceIndex(i);
    generateWindowTables();
}

void GranularSynth::generateWindowTables()
{
    const double pi = juce::MathConstants<double>::pi;
    for (size_t i = 0; i < 4096; ++i)
    {
        const float phase = (float)i / 4095.0f;
        // 0: Hanning
        windowTables[0][i] = (float)(0.5 * (1.0 - std::cos(2.0 * pi * (double)phase)));
        // 1: Triangle
        windowTables[1][i] = 1.0f - std::abs(2.0f * phase - 1.0f);
        // 2: Square (1%ランプでクリック回避)
        windowTables[2][i] = (phase < 0.01f) ? (phase * 100.0f)
                            : ((phase > 0.99f) ? ((1.0f - phase) * 100.0f) : 1.0f);
        // 3: Asymmetric (速いアタック→遅いディケイ)
        windowTables[3][i] = (phase <= 0.2f) ? (phase / 0.2f)
                                             : (1.0f - ((phase - 0.2f) / 0.8f));
    }

    // RMSエネルギー正規化 (E_w = 1): 窓形状を替えても出力音量が一定に揃う
    for (auto& tbl : windowTables)
    {
        double sqSum = 0.0;
        for (float v : tbl) sqSum += (double)v * (double)v;
        const float rms = (float)std::sqrt(sqSum / (double)tbl.size());
        if (rms > 1.0e-6f)
        {
            const float inv = 1.0f / rms;
            for (float& v : tbl) v *= inv;
        }
    }
}

void GranularSynth::prepareToPlay(double sr, int spb)
{
    currentSampleRate = sr;
    for (int ch = 0; ch < 2; ++ch)
        for (int st = 0; st < 2; ++st)
        {
            svfHP[ch][st].prepare(sr);
            svfLP[ch][st].prepare(sr);
        }
    swarmScheduler.reset();
    modalResonator.prepare(sr);
    prevOutGain = juce::Decibels::decibelsToGain(0.0f);
    hasPrevParams = false;
    lastTriggeredNote = 60;
    for (auto& v : voices) v.prepareToPlay(sr, spb);
}

void GranularSynth::releaseResources()
{
    for (auto& v : voices) v.releaseResources();
}

void GranularSynth::handleMidiMessage(const juce::MidiMessage& msg, const GranularParams& p, ModMatrix* modMatrix)
{
    if (msg.isNoteOn())
    {
        lastTriggeredNote = msg.getNoteNumber();
        GranularVoice* target = nullptr;
        const int maxV = juce::jlimit(1, NUM_VOICES, p.poly); // 同時発音数の上限

        // 1) 空きボイス
        for (int i = 0; i < maxV; ++i)
            if (!voices[(size_t)i].isActive()) { target = &voices[(size_t)i]; break; }

        // 2) リリース中の最古ボイス
        if (target == nullptr)
        {
            uint64_t oldest = UINT64_MAX;
            for (int i = 0; i < maxV; ++i)
            {
                auto& v = voices[(size_t)i];
                if (v.isReleasing() && v.getNoteStamp() < oldest) { oldest = v.getNoteStamp(); target = &v; }
            }
        }

        // 3) 最古ノートを強制スチール
        if (target == nullptr)
        {
            uint64_t oldest = UINT64_MAX;
            for (int i = 0; i < maxV; ++i)
            {
                auto& v = voices[(size_t)i];
                if (v.getNoteStamp() < oldest) { oldest = v.getNoteStamp(); target = &v; }
            }
        }

        if (target != nullptr)
        {
            target->startNote(msg.getNoteNumber(), msg.getFloatVelocity(), ++noteCounter, p);
            if (modMatrix != nullptr)
                modMatrix->noteOn(target->getVoiceIndex(), msg.getNoteNumber(), msg.getFloatVelocity());
        }
    }
    else if (msg.isNoteOff())
    {
        const int n = msg.getNoteNumber();
        for (auto& v : voices)
        {
            if (v.getCurrentlyPlayingNote() == n && !v.isReleasing())
            {
                v.stopNote(true);
                if (modMatrix != nullptr)
                    modMatrix->noteOff(v.getVoiceIndex());
            }
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        for (auto& v : voices) v.stopNote(false);
        if (modMatrix != nullptr) modMatrix->allNotesOff();
    }
}

namespace
{
    GranularParams applyPolyVoiceMod(const GranularParams& subP, const ModMatrix* modMatrix, int voiceIdx) noexcept
    {
        GranularParams vp = subP;
        if (modMatrix == nullptr || !modMatrix->isVoiceActive(voiceIdx))
            return vp;

        using D = ModMatrix;
        auto delta = [modMatrix, voiceIdx](int dst) noexcept
        {
            return modMatrix->getForVoice(voiceIdx, dst) - modMatrix->get(dst);
        };

        vp.position          = juce::jlimit(0.0f, 1.0f,      vp.position          + delta(D::DstPosition));
        vp.grainSize         = juce::jlimit(0.02f, 0.5f,     vp.grainSize         + delta(D::DstSize) * 0.48f);
        vp.density           = juce::jlimit(0.0f, 1.0f,      vp.density           + delta(D::DstDensity));
        vp.positionSpray     = juce::jlimit(0.0f, 1.0f,      vp.positionSpray     + delta(D::DstSpray));
        vp.spraySpeed        = juce::jlimit(0.01f, 1.0f,     vp.spraySpeed        + delta(D::DstSpraySpeed) * 0.99f);
        vp.pitchJitter       = juce::jlimit(0.0f, 2400.0f,   vp.pitchJitter       + delta(D::DstJitter) * 2400.0f);
        vp.panSpread         = juce::jlimit(0.0f, 1.0f,      vp.panSpread         + delta(D::DstPanSpread));
        vp.speed             = juce::jlimit(0.0f, 2.0f,      vp.speed             + delta(D::DstSpeed) * 2.0f);
        vp.pitchMod          = juce::jlimit(-48.0f, 48.0f,   vp.pitchMod          + delta(D::DstOctave) * 24.0f + delta(D::DstSemi) * 12.0f);
        vp.fineTune          = juce::jlimit(-100.0f, 100.0f, vp.fineTune          + delta(D::DstFine) * 100.0f);
        vp.tone              = juce::jlimit(0.0f, 1.0f,      vp.tone              + delta(D::DstTone));
        vp.formantShift      = juce::jlimit(-12.0f, 12.0f,   vp.formantShift      + delta(D::DstFormantShift) * 12.0f);
        vp.timbreScrub       = juce::jlimit(0.0f, 1.0f,      vp.timbreScrub       + delta(D::DstTimbreScrub));
        vp.timbreAmt         = juce::jlimit(0.0f, 1.0f,      vp.timbreAmt         + delta(D::DstTimbreAmt));
        vp.atlasPitchLock    = juce::jlimit(0.0f, 1.0f,      vp.atlasPitchLock    + delta(D::DstAtlasPitchLock));
        vp.swarmMix          = juce::jlimit(0.0f, 1.0f,      vp.swarmMix          + delta(D::DstSwarmMix));
        vp.grainSkew         = juce::jlimit(-1.0f, 1.0f,     vp.grainSkew         + delta(D::DstGrainSkew));
        vp.grainChirp        = juce::jlimit(-12.0f, 12.0f,   vp.grainChirp        + delta(D::DstGrainChirp) * 12.0f);
        vp.grainFilterFreq   = juce::jlimit(20.0f, 20000.0f, vp.grainFilterFreq   * std::pow(2.0f, delta(D::DstGrainFilterFreq) * 5.0f));
        vp.grainFilterRes    = juce::jlimit(0.1f, 10.0f,     vp.grainFilterRes    + delta(D::DstGrainFilterRes) * 5.0f);
        vp.grainFilterJitter = juce::jlimit(0.0f, 1.0f,      vp.grainFilterJitter + delta(D::DstGrainFilterJitter));
        return vp;
    }
}

void GranularSynth::renderVoicesRange(juce::AudioBuffer<float>& subBuf,
                                      const juce::AudioBuffer<float>* src,
                                      const SampleMipLevels* mipLevels,
                                      const GrainAtlas* atlas,
                                      ModMatrix* modMatrix,
                                      int trimStart, int trimEnd,
                                      const GranularParams&,
                                      const GranularParams& subP)
{
    int activeTotal = 0;
    int activeVoices = 0;
    for (const auto& v : voices)
        if (v.isActive()) { activeTotal += v.getActiveGrainCount(); ++activeVoices; }
    if (activeVoices <= 0) return;

    int totalSpawnBudget = juce::jmax(0, MAX_TOTAL_GRAINS - activeTotal);

    for (auto& v : voices)
    {
        if (v.isActive())
        {
            int voiceBudget = juce::jmin(totalSpawnBudget,
                                         juce::jmax(4, (totalSpawnBudget + activeVoices - 1) / juce::jmax(1, activeVoices)));
            const int budgetBefore = voiceBudget;
            const GranularParams voiceP = applyPolyVoiceMod(subP, modMatrix, v.getVoiceIndex());
            v.renderNextBlock(subBuf, src, mipLevels, trimStart, trimEnd, windowTables,
                              voiceP, &visualizer, voiceBudget, atlas, &swarmScheduler);
            const int consumed = budgetBefore - voiceBudget;
            totalSpawnBudget = juce::jmax(0, totalSpawnBudget - consumed);
            --activeVoices;
        }
    }
}

void GranularSynth::processBlock(juce::AudioBuffer<float>& buf,
                                 juce::MidiBuffer& midi,
                                 const juce::AudioBuffer<float>* src,
                                 int trimStart, int trimEnd,
                                 const GranularParams& p,
                                 const SampleMipLevels* mipLevels,
                                 const GrainAtlas* atlas,
                                 ModMatrix* modMatrix)
{
    const int nSmp = buf.getNumSamples();
    constexpr int kModSubBlock = 64; // <= 64サンプル単位で変調補間＆MIDIサンプル分割

    auto lerp = [](float a, float b, float t) noexcept { return a + (b - a) * t; };

    auto advanceVoices = [&](int startSample, int chunkLen)
    {
        if (chunkLen <= 0) return;
        GranularParams subP = p;
        if (hasPrevParams)
        {
            const float tMid = ((float)startSample + 0.5f * (float)chunkLen) / (float)juce::jmax(1, nSmp);
            subP.attack            = lerp(prevParams.attack,            p.attack,            tMid);
            subP.decay             = lerp(prevParams.decay,             p.decay,             tMid);
            subP.sustain           = lerp(prevParams.sustain,           p.sustain,           tMid);
            subP.release           = lerp(prevParams.release,           p.release,           tMid);
            subP.grainSize         = lerp(prevParams.grainSize,         p.grainSize,         tMid);
            subP.density           = lerp(prevParams.density,           p.density,           tMid);
            subP.position          = lerp(prevParams.position,          p.position,          tMid);
            subP.positionSpray     = lerp(prevParams.positionSpray,     p.positionSpray,     tMid);
            subP.pitchJitter       = lerp(prevParams.pitchJitter,       p.pitchJitter,       tMid);
            subP.panSpread         = lerp(prevParams.panSpread,         p.panSpread,         tMid);
            subP.speed             = lerp(prevParams.speed,             p.speed,             tMid);
            subP.fineTune          = lerp(prevParams.fineTune,          p.fineTune,          tMid);
            subP.pitchMod          = lerp(prevParams.pitchMod,          p.pitchMod,          tMid);
            subP.spraySpeed        = lerp(prevParams.spraySpeed,        p.spraySpeed,        tMid);
            subP.tone              = lerp(prevParams.tone,              p.tone,              tMid);
            subP.formantShift      = lerp(prevParams.formantShift,      p.formantShift,      tMid);
            subP.timbreScrub       = lerp(prevParams.timbreScrub,       p.timbreScrub,       tMid);
            subP.timbreAmt         = lerp(prevParams.timbreAmt,         p.timbreAmt,         tMid);
            subP.atlasPitchLock    = lerp(prevParams.atlasPitchLock,    p.atlasPitchLock,    tMid);
            subP.swarmMix          = lerp(prevParams.swarmMix,          p.swarmMix,          tMid);
            subP.grainSkew         = lerp(prevParams.grainSkew,         p.grainSkew,         tMid);
            subP.grainChirp        = lerp(prevParams.grainChirp,        p.grainChirp,        tMid);
            subP.grainFilterFreq   = lerp(prevParams.grainFilterFreq,   p.grainFilterFreq,   tMid);
            subP.grainFilterRes    = lerp(prevParams.grainFilterRes,    p.grainFilterRes,    tMid);
            subP.grainFilterJitter = lerp(prevParams.grainFilterJitter, p.grainFilterJitter, tMid);
        }

        juce::AudioBuffer<float> subBuf(buf.getArrayOfWritePointers(), buf.getNumChannels(), startSample, chunkLen);
        renderVoicesRange(subBuf, src, mipLevels, atlas, modMatrix, trimStart, trimEnd, p, subP);
    };

    // サンプル単位MIDIイベント境界分割 ＋ サブブロック変調補間
    auto it = midi.cbegin();
    const auto itEnd = midi.cend();
    int cursor = 0;

    while (cursor < nSmp)
    {
        while (it != itEnd && juce::jlimit(0, nSmp, (*it).samplePosition) <= cursor)
        {
            handleMidiMessage((*it).getMessage(), p, modMatrix);
            ++it;
        }

        const int nextMidiSample = (it != itEnd) ? juce::jlimit(0, nSmp, (*it).samplePosition) : nSmp;
        const int chunkEnd = juce::jmin(nSmp, juce::jmin(nextMidiSample, cursor + kModSubBlock));
        advanceVoices(cursor, chunkEnd - cursor);
        cursor = chunkEnd;
    }

    while (it != itEnd)
    {
        handleMidiMessage((*it).getMessage(), p, modMatrix);
        ++it;
    }

    prevParams = p;
    hasPrevParams = true;

    // 最新ノートの実プレイヘッド位置をGUIへ公開
    {
        float ph = -1.0f;
        uint64_t newest = 0;
        for (const auto& v : voices)
            if (v.isActive() && v.getNoteStamp() >= newest)
            {
                newest = v.getNoteStamp();
                ph = v.getPlayheadNorm();
            }
        playheadDisplay.store(ph, std::memory_order_relaxed);
    }

    // --- Grain-Excited Scale-Tuned Modal Resonator Bank (MasterFilterの前段) ---
    {
        ModalResonatorBank::Params mp;
        mp.mix       = p.modalMix;
        mp.decaySec  = p.modalDecay;
        mp.damp      = p.modalDamp;
        mp.inharm    = p.modalInharm;
        mp.ratioMode = p.modalRatioMode;
        mp.baseMidi  = (float)lastTriggeredNote + (float)p.octave * 12.0f + (float)p.pitchSt
                     + p.fineTune / 100.0f + p.pitchMod;
        mp.scaleRoot = (p.key == 0) ? p.detectedRoot : (p.key - 1);
        mp.scaleIdx  = p.scale;
        modalResonator.processBlock(buf, mp);
    }

    // マスターフィルター (TPT SVF, 対数カットオフスムージング + Butterworth Q1/Q2 24dB/octカスケード)
    const float hpFreq = juce::jlimit(20.0f, 2000.0f, p.masterHPF);
    const float lpFreq = juce::jlimit(200.0f, 20000.0f, p.masterLPF);
    const bool twoStage = (p.filterSlope >= 1);          // 24dB/oct (4th-order Butterworth)
    const float qStage0 = twoStage ? 0.54119610f : 0.70710678f;
    const float qStage1 = 1.30656296f;
    const int nCh = juce::jmin(2, buf.getNumChannels());

    for (int ch = 0; ch < nCh; ++ch)
    {
        svfHP[ch][0].setTarget(hpFreq, qStage0);
        svfLP[ch][0].setTarget(lpFreq, qStage0);
        if (twoStage)
        {
            svfHP[ch][1].setTarget(hpFreq, qStage1);
            svfLP[ch][1].setTarget(lpFreq, qStage1);
        }
        else
        {
            svfHP[ch][1].reset();
            svfLP[ch][1].reset();
        }

        float* d = buf.getWritePointer(ch);
        for (int i = 0; i < nSmp; ++i)
        {
            float x = d[i];
            x = svfHP[ch][0].processHP(x);
            if (twoStage) x = svfHP[ch][1].processHP(x);
            x = svfLP[ch][0].processLP(x);
            if (twoStage) x = svfLP[ch][1].processLP(x);
            d[i] = x;
        }
    }

    // 出力ゲイン: ブロック間をランプしてジッパー音を防止
    const float targetGain = juce::Decibels::decibelsToGain(p.outGainDb);
    if (nSmp > 0)
    {
        buf.applyGainRamp(0, nSmp, prevOutGain, targetGain);
        prevOutGain = targetGain;
    }

    // ※ FXチェーンとマスターリミッターはこの後段 (Processor側) で適用される
    const int channels = buf.getNumChannels();

    if (buf.getNumSamples() > 0)
    {
        const int lastIdx = buf.getNumSamples() - 1;
        const float checkL = buf.getSample(0, lastIdx);
        const float checkR = channels > 1 ? buf.getSample(1, lastIdx) : checkL;
        if (!isFiniteFloat(checkL) || !isFiniteFloat(checkR))
            buf.clear(); // 破損ブロックを即ミュート
    }
}
