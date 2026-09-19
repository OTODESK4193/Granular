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
    samplesToNextSpawn = 0; // 即時に最初のグレインを発火
    randomJumpCountdown = 0.0; // Random: ノート頭で必ず1回抽選する

    if (!wasPlaying)
    {
        // 空きボイスからの新規発音: 停止時に凍結した古いグレインを掃除して清潔に開始
        for (auto& g : grains) g.isActive = false;
        std::fill(combBufL.begin(), combBufL.end(), 0.0f);
        std::fill(combBufR.begin(), combBufR.end(), 0.0f);
        combLpL = combLpR = 0.0f;
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
    lastRelNorm = -1.0f;
}

void GranularVoice::spawnGrain(int trimStart, int trimEnd,
                               double currentPlayhead, const GranularParams& p,
                               float overlap, int baseLife,
                               GrainVisualizerData* visualizer, int& spawnBudget)
{
    if (spawnBudget <= 0) return; // 全体バジェット超過（CPU保護）

    Grain* slot = nullptr;
    for (auto& g : grains)
        if (!g.isActive) { slot = &g; break; }
    if (slot == nullptr) return; // 全スロット使用中

    const int validLength = trimEnd - trimStart;

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

    // --------------------------------------
    // スポーン位置（スプレー + 境界反射）
    //  グレインは再生中に End→Start へラップするため、発火位置は
    //  領域全体 [trimStart, trimEnd) が使える（Positionが常に尊重される）
    // --------------------------------------
    const double maxDelta = juce::jmax(0.001, (double)p.spraySpeed);
    if (std::abs(currentSprayTarget - currentSprayValue) <= maxDelta)
    {
        currentSprayValue = currentSprayTarget;
        currentSprayTarget = ((double)random.nextFloat() * 2.0 - 1.0);
    }
    else
    {
        currentSprayValue += (currentSprayTarget > currentSprayValue ? maxDelta : -maxDelta);
    }

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

    // 高密度時・P-Sync時の音量爆発を防ぐ実効オーバーラップ正規化
    float effOverlap = overlap;
    if (p.pitchSync)
    {
        // P-Sync時はグレイン寿命が periods * notePeriod、発火間隔が notePeriod なので
        // 実効オーバーラップ数は常に periods (2..16)。
        // 周期整列による同位相加算を考慮し、実効エネルギー加算を適正化
        const double noteFreq = 440.0 * std::pow(2.0, ((double)currentMidiNote + (double)p.octave * 12.0 + (double)p.pitchSt - 69.0) / 12.0);
        const double notePeriod = currentSampleRate / juce::jlimit(15.0, 8000.0, noteFreq);
        const int periods = juce::jlimit(2, 16, juce::roundToInt((double)baseLife / notePeriod));
        effOverlap = juce::jmax(1.0f, (float)periods * 1.5f);
    }
    grain.amp = velocityGain / std::sqrt(juce::jmax(1.0f, effOverlap));
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

    const int numSamples = outputBuffer.getNumSamples();
    const int validLength = trimEnd - trimStart;
    if (validLength <= 512 || numSamples <= 0) return;
    if (numSamples > voiceInternalBuffer.getNumSamples())
        voiceInternalBuffer.setSize(2, numSamples, false, true, true);

    voiceInternalBuffer.clear();
    const int srcLen = sharedSourceBuffer->getNumSamples();
    const float* srcL = sharedSourceBuffer->getReadPointer(0);
    const float* srcR = sharedSourceBuffer->getNumChannels() > 1 ? sharedSourceBuffer->getReadPointer(1) : srcL;

    // ノート周波数（オクターブ/セミ/ファイン/MOD込み、ジッター前）
    const double globalSemis = (double)p.octave * 12.0 + (double)p.pitchSt
                             + (double)p.fineTune / 100.0 + (double)p.pitchMod;
    const double noteFreq = 440.0 * std::pow(2.0, ((double)currentMidiNote + globalSemis - 69.0) / 12.0);
    const double notePeriod = currentSampleRate / juce::jlimit(15.0, 8000.0, noteFreq);

    // 決定論的スケジューラ:
    //   density → 同時オーバーラップ量 指数カーブ 0.25..64 (100グレイン/ボイス上限)
    //   interval = grainLife / overlap （±25% のゆらぎ付き）
    //   Pitch-Sync時: interval = ノート周期（発火の周期性がピッチとして知覚される）
    int baseLife = juce::jmax(64, (int)(currentSampleRate * (double)p.grainSize));
    const float overlap = 0.25f * std::pow(2.0f, p.density * 8.0f); // 0.25 .. 64

    if (p.pitchSync)
    {
        // グレイン長をノート周期の整数倍に丸める（周期性を最大化）
        // 周期数は2..16にキャップ: 高音時に同時グレイン数(≒周期数)が
        // 爆発してバジェットを食い潰すのを防ぐ
        const int periods = juce::jlimit(2, 16, juce::roundToInt((double)baseLife / notePeriod));
        baseLife = juce::jmax(64, (int)((double)periods * notePeriod));
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
    {
        speedMag = (double)validLength / juce::jmax(1.0, syncSecs * currentSampleRate);
    }
    else
    {
        // srComp: SPEED=1で「原音の1倍速スキャン」をSRに依らず維持
        // (S-SYNC時は領域長/時間から算出するため補正不要)
        speedMag = std::abs((double)p.speed) * (double)p.srComp;
    }

    // --------------------------------------
    // Randomモード: ジャンプ間隔（サンプル）
    //
    // 他モードと違い Random はマーカーが「移動」しないため speedMag は使えない。
    // 代わりに SPEED を「1秒あたり何回飛ぶか」として解釈し、ジャンプ間隔を決める。
    // これにより DENSITY(粒の密度) と 飛ぶ速さ が完全に独立する:
    //   ・次のジャンプまでに発火した粒は全て同じ場所から出る → スタッター/グリッチ感
    //   ・SPEED=0 でフリーズ(1点に貼り付き)
    //   ・S-SYNC オン時は Scan の音価ごとにジャンプ(拍に同期したグリッチ)
    // 0以下 = フリーズ(再抽選しない)
    // --------------------------------------
    constexpr double kRandomJumpsPerSecAtUnitSpeed = 30.0; // SPEED=1 で毎秒30回
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
            // ノブを速く/フリーズ解除方向へ動かしたとき即座に追従させる
            if (randomJumpCountdown > randomJumpInterval)
                randomJumpCountdown = randomJumpInterval;
        }
    }

    // TONE: コムレゾネーター係数 (1次LPFと線形補間の位相遅れ~0.5サンプルを補正して音程精度を向上)
    const bool  toneOn = p.tone > 0.001f && !combBufL.empty();
    const int   combSize = (int)combBufL.size();
    const float targetCombDelay = (float)juce::jlimit(2.0, (double)(combSize - 4), notePeriod - 0.5);
    if (currentCombDelay == 0.0f) currentCombDelay = targetCombDelay; // 初回
    const float combFb = 0.55f + 0.40f * p.tone;   // tone↑でレゾナンス強化
    const float combDamp = 0.35f;                  // 高域減衰（金属的な響きを抑制）
    const float toneMix = p.tone;

    const double L = (double)validLength;
    const double edgeFadeLen = juce::jmax(1.0, juce::jmin(L * 0.25, currentSampleRate * 0.003)); // 端フェード ~3ms
    const double posCoef = 1.0 - std::exp(-1.0 / (0.02 * currentSampleRate));  // Positionグライド ~20ms
    double lastRel = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        // Position をサンプル単位でグライド（ノブの急ジャンプが大音量遷移=バーストを生むのを防ぐ）
        smoothedPos01 += posCoef * ((double)p.position - smoothedPos01);
        const double posBase = L * smoothedPos01;

        // --------------------------------------
        // プレイヘッド移動 (MODE: Forward/Backward/Ping-Pong/Random)
        //  Positionを起点に領域内を移動する
        // --------------------------------------
        double rel; // 領域内相対位置 0..L
        switch (p.playMode)
        {
        case 1: // Backward
            rel = std::fmod(posBase + playheadOffset, L);
            if (rel < 0.0) rel += L;
            playheadOffset -= speedMag;
            if (playheadOffset < -2.0 * L) playheadOffset += L; // 数値劣化防止
            break;

        case 2: // Ping-Pong (三角折り返し)
        {
            double t = std::fmod(posBase + playheadOffset, 2.0 * L);
            if (t < 0.0) t += 2.0 * L;
            rel = (t <= L) ? t : 2.0 * L - t;
            playheadOffset += speedMag;
            if (playheadOffset > 4.0 * L) playheadOffset -= 2.0 * L;
            break;
        }

        case 3: // Random (SPEED / S-SYNC で決まる一定間隔でジャンプ)
            if (randomJumpCountdown <= 0.0)
            {
                randomTargetRel = (double)random.nextFloat() * (L - 1.0);
                // interval が 0 = フリーズ: 以後は再抽選しない
                randomJumpCountdown = (randomJumpInterval > 0.0) ? randomJumpInterval : 1.0e18;
            }
            randomJumpCountdown -= 1.0;
            rel = randomTargetRel;
            break;

        case 0: // Forward
        default:
            rel = std::fmod(posBase + playheadOffset, L);
            if (rel < 0.0) rel += L;
            playheadOffset += speedMag;
            if (playheadOffset > 2.0 * L) playheadOffset -= L; // 数値劣化防止
            break;
        }

        const double wrappedPlayhead = (double)trimStart + juce::jlimit(0.0, L - 1.0, rel);
        lastRel = rel;

        // グレイン発火スケジューリング
        if (--samplesToNextSpawn <= 0)
        {
            // Random も含め、発火位置は常に現在のプレイヘッド位置
            // (Random のジャンプはプレイヘッド移動側で処理済み)
            const double spawnPos = wrappedPlayhead;

            spawnGrain(trimStart, trimEnd, spawnPos, p, overlap, baseLife, visualizer, spawnBudget);

            if (p.pitchSync)
            {
                // ノート周期に同期（ゆらぎなし＝ピッチ知覚が最大）
                samplesToNextSpawn = juce::jmax(8, (int)notePeriod);
            }
            else
            {
                const float jitter = 0.75f + random.nextFloat() * 0.5f; // ±25%
                samplesToNextSpawn = juce::jmax(32, (int)((float)baseLife / overlap * jitter));
            }
        }

        // アクティブグレインのミックス
        float mixL = 0.0f, mixR = 0.0f;
        for (auto& grain : grains)
        {
            if (!grain.isActive) continue;

            const float phaseRatio = (float)grain.age / (float)juce::jmax(1, grain.life);
            const size_t phaseIdx = (size_t)juce::jlimit(0, 4095, (int)(phaseRatio * 4095.0f));
            const float windowValue = window[phaseIdx];

            const float sL = readHermiteClamped(srcL, srcLen, grain.position);
            const float sR = readHermiteClamped(srcR, srcLen, grain.position);

            // 領域端フェード: End→Start の折り返し(下の position 補正)で生じる
            // 波形の不連続=クリック(複数グレイン同時折り返しで突発的大音量)を防ぐ。
            // 端から edgeFadeLen サンプル以内で読み出しを0へフェードする。
            float edgeFade = 1.0f;
            const double distStart = grain.position - grain.trimStart;
            const double distEnd   = grain.trimEnd - grain.position;
            if (distStart < edgeFadeLen) edgeFade *= (float)juce::jlimit(0.0, 1.0, distStart / edgeFadeLen);
            if (distEnd   < edgeFadeLen) edgeFade *= (float)juce::jlimit(0.0, 1.0, distEnd   / edgeFadeLen);

            mixL += sL * windowValue * grain.panL * grain.amp * edgeFade;
            mixR += sR * windowValue * grain.panR * grain.amp * edgeFade;

            // 領域内ラップ再生 (End→Start へ折り返し):
            // グレイン全長がEndまでに収まる必要がなくなり、Positionが常に尊重される
            grain.position += grain.step;
            const double grainL = grain.trimEnd - grain.trimStart;
            if (grain.position >= grain.trimEnd || grain.position < grain.trimStart)
            {
                double t = std::fmod(grain.position - grain.trimStart, grainL);
                if (t < 0.0) t += grainL;
                grain.position = grain.trimStart + t;
            }

            if (++grain.age >= grain.life) grain.isActive = false;
        }

        // TONE: ノート同調コムレゾネーター (Karplus-Strong系)
        if (toneOn)
        {
            // ボイススチール時にノートが切り替わった際、Delayの急変がバースト音を生むのを防ぐため高速グライド
            currentCombDelay += 0.005f * (targetCombDelay - currentCombDelay);
            auto processComb = [combSize, combDelay = currentCombDelay, combFb, combDamp](std::vector<float>& buf,
                                                                       int writeIdx, float in, float& lpState)
            {
                float readPos = (float)writeIdx - combDelay;
                if (readPos < 0.0f) readPos += (float)combSize;
                const int i1 = (int)readPos;
                const float frac = readPos - (float)i1;
                const int i2 = (i1 + 1) % combSize;
                const float delayed = buf[(size_t)i1] * (1.0f - frac) + buf[(size_t)i2] * frac;

                lpState += combDamp * (delayed - lpState);            // フィードバック内LPF
                // ソフトクリップで共鳴の暴走を抑制（加算FBは 1/(1-fb) 倍まで蓄積するため、
                // 高toneで連続入力するとレゾナンスが爆発し「バシャーン」の一因になる）
                const float w = in + lpState * combFb;
                buf[(size_t)writeIdx] = 1.5f * std::tanh(w * 0.6667f);  // ±1.5で滑らかに飽和
                return lpState;
            };

            const float wetL = processComb(combBufL, combWriteIdx, mixL, combLpL);
            const float wetR = processComb(combBufR, combWriteIdx, mixR, combLpR);
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
    for (auto& v : voices) v.prepareToPlay(sr, spb);
}

void GranularSynth::releaseResources()
{
    for (auto& v : voices) v.releaseResources();
}

void GranularSynth::handleMidi(const juce::MidiBuffer& midi, const GranularParams& p)
{
    for (const auto& m : midi)
    {
        const auto msg = m.getMessage();

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
}

void GranularSynth::processBlock(juce::AudioBuffer<float>& buf,
                                 juce::MidiBuffer& midi,
                                 const juce::AudioBuffer<float>* src,
                                 int trimStart, int trimEnd,
                                 const GranularParams& p)
{
    handleMidi(midi, p);

    // 全体グレインバジェット: 現在のアクティブ数を引いた残りをボイス間で公平に共有
    int activeTotal = 0;
    int activeVoices = 0;
    for (const auto& v : voices)
        if (v.isActive()) { activeTotal += v.getActiveGrainCount(); ++activeVoices; }
    int totalSpawnBudget = juce::jmax(0, MAX_TOTAL_GRAINS - activeTotal);

    for (auto& v : voices)
    {
        if (v.isActive())
        {
            // 1ボイスがバジェットを独占するのを防ぎ、和音演奏時でも全ボイス均等に発音
            int voiceBudget = juce::jmin(totalSpawnBudget,
                                         juce::jmax(4, (totalSpawnBudget + activeVoices - 1) / juce::jmax(1, activeVoices)));
            const int budgetBefore = voiceBudget;
            v.renderNextBlock(buf, src, trimStart, trimEnd, windowTables, p, &visualizer, voiceBudget);
            const int consumed = budgetBefore - voiceBudget;
            totalSpawnBudget = juce::jmax(0, totalSpawnBudget - consumed);
            --activeVoices;
        }
    }

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

    // マスターフィルター (TPT SVF, カットオフはper-sampleスムージング)
    const float hpFreq = juce::jlimit(20.0f, 2000.0f, p.masterHPF);
    const float lpFreq = juce::jlimit(200.0f, 20000.0f, p.masterLPF);
    const bool twoStage = (p.filterSlope >= 1);          // 24dB/oct
    const float qFilter = 0.70710678f;                    // Butterworth
    const int nCh = juce::jmin(2, buf.getNumChannels());
    const int nSmp = buf.getNumSamples();

    for (int ch = 0; ch < nCh; ++ch)
    {
        svfHP[ch][0].setTarget(hpFreq, qFilter);
        svfHP[ch][1].setTarget(hpFreq, qFilter);
        svfLP[ch][0].setTarget(lpFreq, qFilter);
        svfLP[ch][1].setTarget(lpFreq, qFilter);

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
        if (std::isnan(checkL) || std::isinf(checkL) || std::isnan(checkR) || std::isinf(checkR))
            buf.clear(); // 破損ブロックを即ミュート
    }
}
