// ==========================================
// File: DotWaveformDisplay.cpp
// ==========================================
#include "DotWaveformDisplay.h"
#include "ColorPalette.h"

#include <algorithm>

namespace
{
    constexpr float kDotSpacingX = 7.0f;   // ドット列の横間隔
    constexpr float kDotSpacingY = 6.0f;   // ドットの縦間隔
    constexpr float kDotRadius   = 2.2f;   // 波形ドット半径
}

DotWaveformDisplay::DotWaveformDisplay(GranularAudioProcessor& processor)
    : proc(processor)
{
    liveDots.reserve(GrainVisualizerData::kMaxEvents);
    lastEventCount = proc.synth.visualizer.counter.load(std::memory_order_acquire);
    startTimerHz(60);
    setOpaque(false);
}

DotWaveformDisplay::~DotWaveformDisplay() { stopTimer(); }

juce::Rectangle<float> DotWaveformDisplay::getWaveArea() const
{
    return getLocalBounds().toFloat().reduced(14.0f, 12.0f);
}

// ------------------------------------------
// グレインイベント取得（ロックフリー読み出し）
// ------------------------------------------
void DotWaveformDisplay::pullGrainEvents()
{
    auto& viz = proc.synth.visualizer;
    const uint32_t current = viz.counter.load(std::memory_order_acquire);
    uint32_t start = lastEventCount;

    // 大量に溜まっていたら直近分のみ
    if (current - start > (uint32_t)GrainVisualizerData::kMaxEvents)
        start = current - (uint32_t)GrainVisualizerData::kMaxEvents;

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    for (uint32_t i = start; i != current; ++i)
    {
        const auto& e = viz.events[(size_t)(i % GrainVisualizerData::kMaxEvents)];
        LiveDot dot;
        dot.normPos = e.normPos;
        dot.pan = e.pan;
        dot.semitones = e.semitones;
        dot.spawnTimeMs = nowMs;
        dot.durationMs = juce::jlimit(60.0, 1200.0, (double)e.durationSec * 1000.0);
        liveDots.push_back(dot);
    }
    lastEventCount = current;

    // 期限切れドットを除去
    liveDots.erase(std::remove_if(liveDots.begin(), liveDots.end(),
        [nowMs](const LiveDot& d) { return (nowMs - d.spawnTimeMs) > d.durationMs; }),
        liveDots.end());

    if (liveDots.size() > 512)
        liveDots.erase(liveDots.begin(), liveDots.begin() + (long)(liveDots.size() - 512));
}

void DotWaveformDisplay::timerCallback()
{
    pullGrainEvents();
    repaint();
}

// ------------------------------------------
// 描画
// ------------------------------------------
void DotWaveformDisplay::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto wave = getWaveArea();
    const float cy = wave.getCentreY();

    // パネル背景
    g.setColour(GranularColors::panel);
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(dragHover ? GranularColors::mint.withAlpha(0.8f) : GranularColors::panelLine);
    g.drawRoundedRectangle(bounds.reduced(0.75f), 12.0f, dragHover ? 2.0f : 1.5f);

    // センターライン（うっすら）
    g.setColour(GranularColors::grid);
    g.drawHorizontalLine((int)cy, wave.getX(), wave.getRight());

    const auto state = proc.ingestState.load();

    if (state != IngestState::Ready || !proc.waveformReady.load())
    {
        // --- ロード前 / ロード中の表示 ---
        g.setColour(GranularColors::textDim);
        g.setFont(juce::Font(juce::FontOptions(18.0f)));

        juce::String msg;
        switch (state)
        {
            case IngestState::Empty:       msg = "DROP  SAMPLE  HERE"; break;
            case IngestState::Loading:     msg = "Loading..."; break;
            case IngestState::Analyzing:   msg = "Analyzing Pitch..."; break;
            case IngestState::Correcting:  msg = "Pitch Correction..."; break;
            case IngestState::Normalizing: msg = "Normalizing..."; break;
            case IngestState::Failed:      msg = "Load Failed - Try Another File"; break;
            default:                       msg = "..."; break;
        }
        g.drawText(msg, bounds, juce::Justification::centred);

        if (state == IngestState::Loading || state == IngestState::Analyzing
            || state == IngestState::Correcting || state == IngestState::Normalizing)
        {
            const float prog = proc.ingestProgress.load();
            auto bar = juce::Rectangle<float>(wave.getX(), wave.getBottom() - 6.0f, wave.getWidth(), 4.0f);
            g.setColour(GranularColors::knobTrack);
            g.fillRoundedRectangle(bar, 2.0f);
            g.setColour(GranularColors::mint);
            g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * juce::jlimit(0.0f, 1.0f, prog)), 2.0f);
        }
        return;
    }

    // --- 波形ドット描画 ---
    const int totalSamples = proc.validSampleCount.load();
    const int trimS = proc.trimStartSample.load();
    const int trimE = proc.trimEndSample.load();
    const float trimSNorm = totalSamples > 0 ? (float)trimS / (float)totalSamples : 0.0f;
    const float trimENorm = totalSamples > 0 ? (float)trimE / (float)totalSamples : 1.0f;

    const int numCols = juce::jmax(1, (int)(wave.getWidth() / kDotSpacingX));
    const float maxHalfH = wave.getHeight() * 0.5f - 4.0f;

    for (int col = 0; col < numCols; ++col)
    {
        const float colNorm = (float)col / (float)juce::jmax(1, numCols - 1);
        const float x = wave.getX() + kDotSpacingX * 0.5f + (float)col * kDotSpacingX;

        // waveformPeaks(1000ビン)から該当範囲の最大値を取る
        const int binStart = (int)(colNorm * 999.0f);
        const int binEnd = juce::jmin(999, (int)(((float)(col + 1) / (float)numCols) * 999.0f));
        float peak = 0.0f;
        for (int b = binStart; b <= binEnd; ++b)
            peak = juce::jmax(peak, proc.waveformPeaks[(size_t)b]);

        const float ampH = juce::jlimit(0.0f, maxHalfH, peak * 2.0f * maxHalfH);
        const int dotCount = juce::jmax(1, (int)(ampH / kDotSpacingY));

        const bool insideTrim = (colNorm >= trimSNorm && colNorm <= trimENorm);
        auto baseColour = GranularColors::waveformGradient(colNorm);
        if (!insideTrim) baseColour = baseColour.withMultipliedAlpha(0.18f);

        for (int k = 0; k < dotCount; ++k)
        {
            // 中心から外側へ向けて少しずつ薄く
            const float fade = 1.0f - 0.55f * ((float)k / (float)juce::jmax(1, dotCount - 1));
            g.setColour(baseColour.withMultipliedAlpha(0.9f * fade));

            const float dy = (float)k * kDotSpacingY;
            g.fillEllipse(x - kDotRadius, cy - dy - kDotRadius, kDotRadius * 2.0f, kDotRadius * 2.0f);
            if (k > 0)
                g.fillEllipse(x - kDotRadius, cy + dy - kDotRadius, kDotRadius * 2.0f, kDotRadius * 2.0f);
        }
    }

    // --- Position / Spray インジケーター ---
    const float posNorm = proc.apvts.getRawParameterValue("position")->load();
    const float sprayNorm = proc.apvts.getRawParameterValue("positionSpray")->load();

    const float trimX = wave.getX() + trimSNorm * wave.getWidth();
    const float trimW = juce::jmax(1.0f, (trimENorm - trimSNorm) * wave.getWidth());
    const float posX = trimX + posNorm * trimW;

    if (sprayNorm > 0.001f)
    {
        const float sprayW = sprayNorm * trimW;
        auto sprayRect = juce::Rectangle<float>(posX - sprayW, wave.getY(), sprayW * 2.0f, wave.getHeight());
        sprayRect = sprayRect.getIntersection(wave);
        g.setColour(GranularColors::pink.withAlpha(0.10f));
        g.fillRoundedRectangle(sprayRect, 4.0f);
    }

    g.setColour(GranularColors::pink.withAlpha(0.25f));
    g.fillRect(posX - 2.0f, wave.getY(), 4.0f, wave.getHeight());
    g.setColour(GranularColors::pink);
    g.fillRect(posX - 0.75f, wave.getY(), 1.5f, wave.getHeight());

    // --- ライブ・グレインドット ---
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    for (const auto& d : liveDots)
    {
        const float t = (float)juce::jlimit(0.0, 1.0, (nowMs - d.spawnTimeMs) / d.durationMs);
        const float alpha = (1.0f - t) * (1.0f - t); // 二乗減衰で余韻を演出

        const float gx = trimX + d.normPos * trimW;
        const float gy = juce::jlimit(wave.getY() + 4.0f, wave.getBottom() - 4.0f,
                                      cy - d.semitones * 4.0f + (d.pan - 0.5f) * 10.0f);
        const float r = 2.5f + 2.5f * (1.0f - t);

        const auto c = GranularColors::grainColour(d.semitones);
        // グロー
        g.setColour(c.withAlpha(0.25f * alpha));
        g.fillEllipse(gx - r * 2.2f, gy - r * 2.2f, r * 4.4f, r * 4.4f);
        // コア
        g.setColour(c.withAlpha(alpha));
        g.fillEllipse(gx - r, gy - r, r * 2.0f, r * 2.0f);
    }
}

// ------------------------------------------
// D&D
// ------------------------------------------
bool DotWaveformDisplay::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aif") || f.endsWithIgnoreCase(".aiff")
            || f.endsWithIgnoreCase(".mp3") || f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".ogg"))
            return true;
    return false;
}

void DotWaveformDisplay::filesDropped(const juce::StringArray& files, int, int)
{
    dragHover = false;
    if (files.isEmpty()) return;
    proc.loadFileForIngest(juce::File(files[0]));
    repaint();
}

// ------------------------------------------
// クリック / ドラッグで Position 操作
// ------------------------------------------
void DotWaveformDisplay::setPositionFromMouse(const juce::MouseEvent& e, bool isStart)
{
    if (proc.ingestState.load() != IngestState::Ready) return;

    auto* param = proc.apvts.getParameter("position");
    if (param == nullptr) return;

    const int totalSamples = proc.validSampleCount.load();
    const float trimSNorm = totalSamples > 0 ? (float)proc.trimStartSample.load() / (float)totalSamples : 0.0f;
    const float trimENorm = totalSamples > 0 ? (float)proc.trimEndSample.load() / (float)totalSamples : 1.0f;

    const auto wave = getWaveArea();
    const float trimX = wave.getX() + trimSNorm * wave.getWidth();
    const float trimW = juce::jmax(1.0f, (trimENorm - trimSNorm) * wave.getWidth());

    const float norm = juce::jlimit(0.0f, 1.0f, ((float)e.position.getX() - trimX) / trimW);

    if (isStart) param->beginChangeGesture();
    param->setValueNotifyingHost(norm); // position は 0..1 なので正規化値と一致
}

void DotWaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    draggingPosition = true;
    setPositionFromMouse(e, true);
}

void DotWaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingPosition) setPositionFromMouse(e, false);
}

void DotWaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    if (draggingPosition)
    {
        if (auto* param = proc.apvts.getParameter("position"))
            param->endChangeGesture();
        draggingPosition = false;
    }
}
