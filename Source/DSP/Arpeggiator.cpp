// ==========================================
// File: Arpeggiator.cpp
// ==========================================
#include "Arpeggiator.h"

#include <algorithm>
#include <cmath>

void Arpeggiator::reset()
{
    held.clear();
    physical.fill(false);
    physicalCount = 0;
    orderCounter = 0;
    seq.clear();
    seqPos = 0;
    walkPos = 0;
    samplesToNextStep = 0.0;
    running = false;
    active.clear();
}

// ------------------------------------------
// ステップ長（サンプル）
// ------------------------------------------
double Arpeggiator::getStepSamples(const ArpParams& p) const
{
    if (!p.sync)
        return sampleRate / (double)juce::jlimit(0.5f, 30.0f, p.rateFree);

    // 1拍 = 1/4音符。付点=×1.5、三連=×2/3
    static const double beatsPerStep[16] = {
        32.0,       // 8/1
        16.0,       // 4/1
        8.0,        // 2/1
        4.0,        // 1/1
        2.0,        // 1/2
        4.0 / 3.0,  // 1/2T
        1.0,        // 1/4
        1.5,        // 1/4.
        2.0 / 3.0,  // 1/4T
        0.5,        // 1/8
        0.75,       // 1/8.
        1.0 / 3.0,  // 1/8T
        0.25,       // 1/16
        0.375,      // 1/16.
        1.0 / 6.0,  // 1/16T
        0.125       // 1/32
    };
    const int idx = juce::jlimit(0, 15, p.rateSync);
    const double bpm = p.bpm > 1.0 ? p.bpm : 120.0;
    return (60.0 / bpm) * beatsPerStep[idx] * sampleRate;
}

int Arpeggiator::getGateSamples(double stepSamples, const ArpParams& p) const
{
    double g;
    switch (p.durMode)
    {
        case 1:  g = (60.0 / (p.bpm > 1.0 ? p.bpm : 120.0)) * 0.125 * sampleRate; break; // 1/32
        case 2:  g = (60.0 / (p.bpm > 1.0 ? p.bpm : 120.0)) * 0.25  * sampleRate; break; // 1/16
        case 3:  g = (60.0 / (p.bpm > 1.0 ? p.bpm : 120.0)) * 0.5   * sampleRate; break; // 1/8
        case 4:  g = (60.0 / (p.bpm > 1.0 ? p.bpm : 120.0)) * 1.0   * sampleRate; break; // 1/4
        default: g = stepSamples * (double)juce::jlimit(0.05f, 1.0f, p.gate); break;     // Gate%
    }
    // 同一ノート再発火との衝突を避けるため、ステップ長-8サンプルでクランプ
    return juce::jlimit(64, juce::jmax(64, (int)stepSamples - 8), (int)g);
}

// ------------------------------------------
// Quantize Key/Scale へスナップ
// ------------------------------------------
int Arpeggiator::quantizeNote(int note, const ArpParams& p) const
{
    if (p.scale <= 0) return note;
    const int root = (p.key == 0) ? p.detectedRoot : (p.key - 1);
    return juce::roundToInt(ScaleQuantizer::quantize((float)note, root, p.scale));
}

// ------------------------------------------
// 入力イベント処理（heldプールの更新）
// ------------------------------------------
void Arpeggiator::handleInputEvent(const juce::MidiMessage& msg, const ArpParams& p)
{
    if (msg.isNoteOn())
    {
        const int note = msg.getNoteNumber();

        // Latch: 全鍵離した後の新しい打鍵でコードを入れ替える
        if (p.latch && physicalCount == 0)
            held.clear();

        if (!physical[(size_t)note]) { physical[(size_t)note] = true; ++physicalCount; }

        bool exists = false;
        for (auto& h : held)
            if (h.note == note) { h.vel = msg.getFloatVelocity(); exists = true; break; }
        if (!exists)
            held.push_back({ note, msg.getFloatVelocity(), orderCounter++ });
    }
    else if (msg.isNoteOff())
    {
        const int note = msg.getNoteNumber();
        if (physical[(size_t)note]) { physical[(size_t)note] = false; --physicalCount; }

        if (!p.latch)
            held.erase(std::remove_if(held.begin(), held.end(),
                       [note](const HeldNote& h) { return h.note == note; }), held.end());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        held.clear();
        physical.fill(false);
        physicalCount = 0;
    }
}

// ------------------------------------------
// パターン展開: held → seq
// ------------------------------------------
void Arpeggiator::buildSequence(const ArpParams& p)
{
    seq.clear();
    if (held.empty()) return;

    const int octaves = juce::jlimit(1, 4, p.octaves);

    // 音高順プール（オクターブ展開込み・スクラッチ再利用でアロケーション回避）
    auto& sorted = scratchSorted;
    sorted.clear();
    sorted.insert(sorted.end(), held.begin(), held.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

    auto& asc = scratchAsc;
    asc.clear();
    for (int o = 0; o < octaves; ++o)
        for (const auto& h : sorted)
            asc.push_back({ juce::jlimit(0, 127, h.note + 12 * o), h.vel });

    const int n = (int)asc.size();
    auto pushRange = [this, &asc](int from, int to, int step)
    {
        for (int i = from; step > 0 ? i <= to : i >= to; i += step)
            seq.push_back(asc[(size_t)i]);
    };

    switch (p.pattern)
    {
    case Up:
        seq = asc;
        break;

    case Down:
        pushRange(n - 1, 0, -1);
        break;

    case UpDown: // 端を繰り返さない折り返し
        seq = asc;
        if (n > 2) pushRange(n - 2, 1, -1);
        break;

    case DownUp:
        pushRange(n - 1, 0, -1);
        if (n > 2) pushRange(1, n - 2, 1);
        break;

    case UpDownIncl: // 端も繰り返す
        seq = asc;
        pushRange(n - 1, 0, -1);
        break;

    case Converge: // 外→内 (低,高,低+1,高-1...)
    {
        int i = 0, j = n - 1;
        while (i <= j)
        {
            seq.push_back(asc[(size_t)i++]);
            if (i <= j) seq.push_back(asc[(size_t)j--]);
        }
        break;
    }

    case Diverge: // 内→外 (Convergeの逆順)
    {
        int i = 0, j = n - 1;
        auto& conv = scratchConv;
        conv.clear();
        while (i <= j)
        {
            conv.push_back(asc[(size_t)i++]);
            if (i <= j) conv.push_back(asc[(size_t)j--]);
        }
        seq.assign(conv.rbegin(), conv.rend());
        break;
    }

    case PedalLow: // 最低音を挟む (L,n1,L,n2...)
        if (n == 1) { seq = asc; }
        else
            for (int k = 1; k < n; ++k)
            {
                seq.push_back(asc[0]);
                seq.push_back(asc[(size_t)k]);
            }
        break;

    case PedalHigh: // 最高音を挟む (H,n1,H,n2...)
        if (n == 1) { seq = asc; }
        else
            for (int k = 0; k < n - 1; ++k)
            {
                seq.push_back(asc[(size_t)(n - 1)]);
                seq.push_back(asc[(size_t)k]);
            }
        break;

    case AsPlayed: // 弾いた順（オクターブはパス毎に上へ）
    {
        auto& byOrder = scratchSorted; // 再利用 (ソート順を変えるだけ)
        byOrder.clear();
        byOrder.insert(byOrder.end(), held.begin(), held.end());
        std::sort(byOrder.begin(), byOrder.end(),
                  [](const HeldNote& a, const HeldNote& b) { return a.order < b.order; });
        for (int o = 0; o < octaves; ++o)
            for (const auto& h : byOrder)
                seq.push_back({ juce::jlimit(0, 127, h.note + 12 * o), h.vel });
        break;
    }

    case Random:
    case RandomWalk:
    case Chord:
    default:
        seq = asc; // これらはfireStep側で選択方法を変える
        break;
    }
}

// ------------------------------------------
// 1ステップ発火
// ------------------------------------------
void Arpeggiator::fireStep(juce::MidiBuffer& out, int offset, int gateSamples, const ArpParams& p)
{
    buildSequence(p);
    if (seq.empty()) return;

    const int n = (int)seq.size();

    auto emit = [&](const PoolNote& pn)
    {
        const int note = juce::jlimit(0, 127, quantizeNote(pn.note, p));

        // 安全リトリガー: 同一ノートがまだ発音中(active)なら、先に noteOff を出す。
        // noteOffのオフセットを「offset - 1」（最低0）にすることで、同一サンプル内での
        // noteOff -> noteOn の処理順序を確実に保証し、ボイス詰まりを完全に防ぎます。
        for (auto it = active.begin(); it != active.end(); )
        {
            if (it->note == note) { out.addEvent(juce::MidiMessage::noteOff(1, note), juce::jmax(0, offset - 1)); it = active.erase(it); }
            else ++it;
        }

        out.addEvent(juce::MidiMessage::noteOn(1, note, pn.vel), offset);
        active.push_back({ note, offset + gateSamples });
    };

    if (p.pattern == Chord)
    {
        // 全ノート同時（重複量子化ノートは1回だけ）
        int lastNote = -1;
        for (const auto& pn : seq)
        {
            const int q = juce::jlimit(0, 127, quantizeNote(pn.note, p));
            if (q != lastNote) { emit(pn); lastNote = q; }
        }
    }
    else if (p.pattern == Random)
    {
        emit(seq[(size_t)random.nextInt(n)]);
    }
    else if (p.pattern == RandomWalk)
    {
        walkPos += random.nextBool() ? 1 : -1;
        if (walkPos < 0) walkPos = juce::jmin(1, n - 1);       // 反射
        if (walkPos >= n) walkPos = juce::jmax(0, n - 2);
        emit(seq[(size_t)walkPos]);
    }
    else
    {
        emit(seq[(size_t)(seqPos % n)]);
        ++seqPos;
    }
}

// ------------------------------------------
// メイン処理
// ------------------------------------------
void Arpeggiator::process(juce::MidiBuffer& midi, int numSamples, const ArpParams& p)
{
    if (!p.enable)
    {
        if (wasEnabled)
        {
            // OFF切替: 鳴っているアルペジオノートを止めて入力をスルーに戻す
            // noteOffをブロックの末尾(numSamples-1)に配置することで、同一ブロック内の
            // 直前の発音（noteOn）が先に処理されることを保証し、ノート詰まりを防止します。
            juce::MidiBuffer out;
            for (const auto& a : active)
                out.addEvent(juce::MidiMessage::noteOff(1, a.note), juce::jmax(0, numSamples - 1));
            active.clear();
            held.clear();
            physical.fill(false);
            physicalCount = 0;
            running = false;
            for (const auto& m : midi)
                out.addEvent(m.getMessage(), m.samplePosition);
            midi.swapWith(out);
            wasEnabled = false;
        }
        return;
    }
    wasEnabled = true;

    // 1) 入力を消費して held を更新（ノート以外のCC/ピッチベンド等は素通し）
    juce::MidiBuffer passthrough;
    const bool hadNotes = !held.empty();
    for (const auto& m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn() || msg.isNoteOff() || msg.isAllNotesOff() || msg.isAllSoundOff())
            handleInputEvent(msg, p);
        else
            passthrough.addEvent(msg, m.samplePosition);
    }
    midi.clear();

    if (!hadNotes && !held.empty())
    {
        samplesToNextStep = 0.0; // 打鍵直後に即発火
        seqPos = 0;
        walkPos = 0;
        running = true;
    }
    if (held.empty())
        running = false;

    juce::MidiBuffer out;
    out.addEvents(passthrough, 0, -1, 0); // CC等を先に戻す

    // 2) ステップ発火（サンプル精度）
    if (running)
    {
        const double stepLen = juce::jmax(64.0, getStepSamples(p));
        const int gateSamples = getGateSamples(stepLen, p);

        double pos = samplesToNextStep;
        while (pos < (double)numSamples)
        {
            fireStep(out, (int)pos, gateSamples, p);
            pos += stepLen;
        }
        samplesToNextStep = pos - (double)numSamples;
    }

    // 3) スケジュール済み noteOff の放出
    for (auto it = active.begin(); it != active.end();)
    {
        if (it->samplesLeft < numSamples)
        {
            out.addEvent(juce::MidiMessage::noteOff(1, it->note), juce::jmax(0, it->samplesLeft));
            it = active.erase(it);
        }
        else
        {
            it->samplesLeft -= numSamples;
            ++it;
        }
    }

    midi.swapWith(out);
}
