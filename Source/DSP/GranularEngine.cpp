// ==========================================
// File: GranularEngine.cpp
// ==========================================
#include "GranularEngine.h"

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
}

void GranularVoice::spawnGrain(int trimStart, int trimEnd,
                               double currentPlayhead, const GranularParams& p,
                               float overlap, int baseLife,
                               GrainVisualizerData* visualizer, int& spawnBudget)
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
    // Pitch-Sync時は周期整数倍の長さを維持（ランダム化すると周期性が壊れる）
    grain.life = p.pitchSync ? baseLife
                             : baseLife + random.nextInt(juce::jmax(1, baseLife / 2));

    // --------------------------------------
    // グレインピッチ量子化（変調後に適用）
    //   最終ピッチ = ノート + オクターブ/セミ/ファイン + ランダムジッター
    //   これを丸ごと Key/Scale へスナップする
    // --------------------------------------
    const float globalSemis = (float)p.octave * 12.0f + (float)p.pitchSt
                            + p.fineTune / 100.0f + p.pitchMod; // MOD変調も量子化前
    const float jitterSemis = ((random.nextFloat() * 2.0f - 1.0f) * p.pitchJitter) / 100.0f;

    float totalAbsSemis = (float)currentMidiNote + globalSemis + jitterSemis; // 絶対ピッチ(MIDI)

    if (p.scaleOn)
    {
        const int root = (p.key == 0) ? p.detectedRoot : (p.key - 1); // 0..11
        totalAbsSemis = ScaleQuantizer::quantize(totalAbsSemis, root, p.scale);
    }

    // Root Note Tracking: サンプルの元ピッチ(rootNote)を弾くと原音で再生
    // srComp: バッファSRとホストSRの差を補正（DAWのSR変更でもピッチ不変）
    const double pitchRatio = std::pow(2.0, ((double)totalAbsSemis - (double)p.rootNote) / 12.0)
                            * (double)p.srComp;
    grain.step = p.reverseGrain ? -pitchRatio : pitchRatio;
    grain.mipLevel = (pitchRatio > 1.0) ? juce::jlimit(0.0f, (float)SampleMipLevels::kExtraLevels, (float)std::log2(pitchRatio))
                                        : 0.0f;

    // --------------------------------------
    // スポーン位置（時間ベースランダムウォークスプレー + 境界反射）
    //  renderNextBlock 側でサンプル時刻ベース更新済みの currentSprayValue を使用
    // --------------------------------------
    const double sprayOffset = currentSprayValue * (double)validLength * (double)p.positionSpray;
    const double rawPos = currentPlayhead + sprayOffset;

    const double minPos = (double)trimStart;
    const double maxPos = (double)trimEnd - 1.0;

    // 範囲外へ飛んだSprayは境界で反射（ミラー折り返し）して範囲内に分布させる
    {
        const double range = maxPos - minPos;
        if (range <= 0.0)
            grain.position = minPos;
        else
        {
            double t = std::fmod(rawPos - minPos, 2.0 * range);
            if (t < 0.0) t += 2.0 * range;
            grain.position = (t <= range) ? (minPos + t) : (minPos + 2.0 * range - t);
        }
    }

    // --------------------------------------
    // パン & 音量
    //  交互配置: グレイン毎にL/R側を反転させ、高Density時でも
    //  中央に均されず常にステレオ幅が出るようにする
    // --------------------------------------
    panSide = -panSide;
    const float panOffset = random.nextFloat() * 0.5f * p.panSpread * panSide;
    const float pan = juce::jlimit(0.0f, 1.0f, 0.5f + panOffset);
    grain.panL = std::sqrt(1.0f - pan);
    grain.panR = std::sqrt(pan);

    // 高密度時・P-Sync時の音量爆発を防ぐ実効オーバーラップ正規化 (RMS窓 E_w=1 前提)
    float gainDivisor = std::sqrt(juce::jmax(1.0f, overlap));
    if (p.pitchSync)
    {
        // P-Sync時は周期整列による同位相加算 (amplitude ∝ periods * mean(window)) のため
        // periods に比例した除数 (1.25 * periods) で音量を一定に保つ
        const double noteSemis = (double)currentMidiNote + (double)globalSemis;
        const double noteFreq = 440.0 * std::pow(2.0, (noteSemis - 69.0) / 12.0);
        const double notePeriod = currentSampleRate / juce::jlimit(15.0, 8000.0, noteFreq);
        const int periods = juce::jlimit(2, 16, juce::roundToInt((double)baseLife / notePeriod));
        gainDivisor = juce::jmax(1.0f, 1.25f * (float)periods);
    }
    grain.amp = velocityGain / gainDivisor;
    grain.trimStart = (double)trimStart;
    grain.trimEnd = (double)trimEnd;

    --spawnBudget;

    // GUIへ発火イベントを通知（ロックフリー）
    if (visualizer != nullptr && validLength > 0)
    {
        const float normPos = (float)((grain.position - (double)trimStart) / (double)validLength);
        visualizer->push(juce::jlimit(0.0f, 1.0f, normPos), pan,
                         totalAbsSemis - (float)p.rootNote,
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
                                    int& spawnBudget)
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
            spawnGrain(trimStart, trimEnd, wrappedPlayhead, p, overlap, baseLife, visualizer, spawnBudget);

            const double interval = p.pitchSync
                ? juce::jmax(4.0, notePeriod)
                : juce::jmax(16.0, ((double)baseLife / (double)overlap) * (0.75 + (double)random.nextFloat() * 0.5));
            samplesToNextSpawn += interval;
            if (samplesToNextSpawn < -interval)
                samplesToNextSpawn = interval;
        }
        samplesToNextSpawn -= 1.0;

        // アクティブグレインのミックス（窓テーブル線形補間）
        float mixL = 0.0f, mixR = 0.0f;
        for (auto& grain : grains)
        {
            if (!grain.isActive) continue;

            const float phaseRatio = (float)grain.age / (float)juce::jmax(1, grain.life);
            const float fIdx = juce::jlimit(0.0f, 4095.0f, phaseRatio * 4095.0f);
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

            grain.position += grain.step;
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
    prevOutGain = juce::Decibels::decibelsToGain(0.0f);
    hasPrevParams = false;
    for (auto& v : voices) v.prepareToPlay(sr, spb);
}

void GranularSynth::releaseResources()
{
    for (auto& v : voices) v.releaseResources();
}

void GranularSynth::handleMidiMessage(const juce::MidiMessage& msg, const GranularParams& p)
{
    if (msg.isNoteOn())
    {
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
            target->startNote(msg.getNoteNumber(), msg.getFloatVelocity(), ++noteCounter, p);
    }
    else if (msg.isNoteOff())
    {
        const int n = msg.getNoteNumber();
        for (auto& v : voices)
            if (v.getCurrentlyPlayingNote() == n && !v.isReleasing()) v.stopNote(true);
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        for (auto& v : voices) v.stopNote(false);
    }
}

void GranularSynth::renderVoicesRange(juce::AudioBuffer<float>& subBuf,
                                      const juce::AudioBuffer<float>* src,
                                      const SampleMipLevels* mipLevels,
                                      int trimStart, int trimEnd,
                                      const GranularParams& p)
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
            v.renderNextBlock(subBuf, src, mipLevels, trimStart, trimEnd, windowTables, p, &visualizer, voiceBudget);
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
                                 const SampleMipLevels* mipLevels)
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
            subP.attack        = lerp(prevParams.attack,        p.attack,        tMid);
            subP.decay         = lerp(prevParams.decay,         p.decay,         tMid);
            subP.sustain       = lerp(prevParams.sustain,       p.sustain,       tMid);
            subP.release       = lerp(prevParams.release,       p.release,       tMid);
            subP.grainSize     = lerp(prevParams.grainSize,     p.grainSize,     tMid);
            subP.density       = lerp(prevParams.density,       p.density,       tMid);
            subP.position      = lerp(prevParams.position,      p.position,      tMid);
            subP.positionSpray = lerp(prevParams.positionSpray, p.positionSpray, tMid);
            subP.pitchJitter   = lerp(prevParams.pitchJitter,   p.pitchJitter,   tMid);
            subP.panSpread     = lerp(prevParams.panSpread,     p.panSpread,     tMid);
            subP.speed         = lerp(prevParams.speed,         p.speed,         tMid);
            subP.fineTune      = lerp(prevParams.fineTune,      p.fineTune,      tMid);
            subP.pitchMod      = lerp(prevParams.pitchMod,      p.pitchMod,      tMid);
            subP.spraySpeed    = lerp(prevParams.spraySpeed,    p.spraySpeed,    tMid);
            subP.tone          = lerp(prevParams.tone,          p.tone,          tMid);
        }

        juce::AudioBuffer<float> subBuf(buf.getArrayOfWritePointers(), buf.getNumChannels(), startSample, chunkLen);
        renderVoicesRange(subBuf, src, mipLevels, trimStart, trimEnd, subP);
    };

    // サンプル単位MIDIイベント境界分割 ＋ サブブロック変調補間
    auto it = midi.cbegin();
    const auto itEnd = midi.cend();
    int cursor = 0;

    while (cursor < nSmp)
    {
        while (it != itEnd && juce::jlimit(0, nSmp, (*it).samplePosition) <= cursor)
        {
            handleMidiMessage((*it).getMessage(), p);
            ++it;
        }

        const int nextMidiSample = (it != itEnd) ? juce::jlimit(0, nSmp, (*it).samplePosition) : nSmp;
        const int chunkEnd = juce::jmin(nSmp, juce::jmin(nextMidiSample, cursor + kModSubBlock));
        advanceVoices(cursor, chunkEnd - cursor);
        cursor = chunkEnd;
    }

    while (it != itEnd)
    {
        handleMidiMessage((*it).getMessage(), p);
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
