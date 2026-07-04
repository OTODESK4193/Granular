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
    constexpr float kMarkerHitPx = 7.0f;   // マーカーの当たり判定幅
    constexpr float kMinRegion   = 0.02f;  // Start/End の最小間隔
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

DotWaveformDisplay::RegionGeometry DotWaveformDisplay::getRegionGeometry() const
{
    RegionGeometry geo;
    const auto wave = getWaveArea();
    geo.waveX = wave.getX();
    geo.waveW = juce::jmax(1.0f, wave.getWidth());

    const int total = proc.validSampleCount.load();
    geo.autoSN = total > 0 ? (float)proc.trimStartSample.load() / (float)total : 0.0f;
    geo.autoEN = total > 0 ? (float)proc.trimEndSample.load() / (float)total : 1.0f;

    const float s01 = proc.apvts.getRawParameterValue("sampleStart")->load();
    const float e01 = proc.apvts.getRawParameterValue("sampleEnd")->load();
    const float span = geo.autoEN - geo.autoSN;
    geo.effSN = geo.autoSN + s01 * span;
    geo.effEN = geo.autoSN + e01 * span;

    geo.effX = geo.waveX + geo.effSN * geo.waveW;
    geo.effW = juce::jmax(1.0f, (geo.effEN - geo.effSN) * geo.waveW);
    return geo;
}

// ------------------------------------------
// グレインイベント取得（ロックフリー読み出し）
// ------------------------------------------
void DotWaveformDisplay::pullGrainEvents()
{
    auto& viz = proc.synth.visualizer;
    const uint32_t current = viz.counter.load(std::memory_order_acquire);
    uint32_t start = lastEventCount;

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

    g.setColour(GranularColors::grid);
    g.drawHorizontalLine((int)cy, wave.getX(), wave.getRight());

    const auto state = proc.ingestState.load();

    if (state != IngestState::Ready || !proc.waveformReady.load())
    {
        g.setColour(GranularColors::textDim);
        g.setFont(juce::Font(juce::FontOptions(18.0f)));

        juce::String msg;
        switch (state)
        {
            case IngestState::Empty:       msg = "DROP  SAMPLE  HERE"; break;
            case IngestState::Loading:     msg = "Loading..."; break;
            case IngestState::Analyzing:   msg = "Analyzing Pitch..."; break;
            case IngestState::Correcting:  msg = "Cents Correction..."; break;
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

    const auto geo = getRegionGeometry();

    // --- 波形ドット描画 ---
    const int numCols = juce::jmax(1, (int)(wave.getWidth() / kDotSpacingX));
    const float maxHalfH = wave.getHeight() * 0.5f - 4.0f;

    for (int col = 0; col < numCols; ++col)
    {
        const float colNorm = (float)col / (float)juce::jmax(1, numCols - 1);
        const float x = wave.getX() + kDotSpacingX * 0.5f + (float)col * kDotSpacingX;

        const int binStart = (int)(colNorm * 999.0f);
        const int binEnd = juce::jmin(999, (int)(((float)(col + 1) / (float)numCols) * 999.0f));
        float peak = 0.0f;
        for (int b = binStart; b <= binEnd; ++b)
            peak = juce::jmax(peak, proc.waveformPeaks[(size_t)b]);

        const float ampH = juce::jlimit(0.0f, maxHalfH, peak * 2.0f * maxHalfH);
        const int dotCount = juce::jmax(1, (int)(ampH / kDotSpacingY));

        const bool insideRegion = (colNorm >= geo.effSN && colNorm <= geo.effEN);
        auto baseColour = GranularColors::waveformGradient(colNorm);
        if (!insideRegion) baseColour = baseColour.withMultipliedAlpha(0.18f);

        for (int k = 0; k < dotCount; ++k)
        {
            const float fade = 1.0f - 0.55f * ((float)k / (float)juce::jmax(1, dotCount - 1));
            g.setColour(baseColour.withMultipliedAlpha(0.9f * fade));

            const float dy = (float)k * kDotSpacingY;
            g.fillEllipse(x - kDotRadius, cy - dy - kDotRadius, kDotRadius * 2.0f, kDotRadius * 2.0f);
            if (k > 0)
                g.fillEllipse(x - kDotRadius, cy + dy - kDotRadius, kDotRadius * 2.0f, kDotRadius * 2.0f);
        }
    }

    // --- Start/End マーカー ---
    auto drawMarker = [&g, &wave](float x, bool isStart, juce::Colour c)
    {
        g.setColour(c.withAlpha(0.85f));
        g.fillRect(x - 1.0f, wave.getY(), 2.0f, wave.getHeight());

        // 上部の三角ハンドル（内側向き）
        juce::Path tri;
        const float dir = isStart ? 1.0f : -1.0f;
        tri.addTriangle(x, wave.getY(),
                        x, wave.getY() + 12.0f,
                        x + dir * 9.0f, wave.getY() + 6.0f);
        g.fillPath(tri);
    };

    const float startX = geo.waveX + geo.effSN * geo.waveW;
    const float endX = geo.waveX + geo.effEN * geo.waveW;
    drawMarker(startX, true, GranularColors::peach);
    drawMarker(endX, false, GranularColors::peach);

    // Start/End への変調を白い細線で表示（実効マーカーはドラッグ用に据え置き）
    {
        const float span = geo.autoEN - geo.autoSN;
        const float s01 = proc.apvts.getRawParameterValue("sampleStart")->load();
        const float e01 = proc.apvts.getRawParameterValue("sampleEnd")->load();
        const float sMod = proc.modMatrix.get(ModMatrix::DstSampleStart);
        const float eMod = proc.modMatrix.get(ModMatrix::DstSampleEnd);
        if (std::abs(sMod) > 1.0e-4f)
        {
            const float x = geo.waveX + (geo.autoSN + juce::jlimit(0.0f, 1.0f, s01 + sMod) * span) * geo.waveW;
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.fillRect(x - 0.75f, wave.getY(), 1.5f, wave.getHeight());
        }
        if (std::abs(eMod) > 1.0e-4f)
        {
            const float x = geo.waveX + (geo.autoSN + juce::jlimit(0.0f, 1.0f, e01 + eMod) * span) * geo.waveW;
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.fillRect(x - 0.75f, wave.getY(), 1.5f, wave.getHeight());
        }
    }

    // --- Position / Spray インジケーター (Sprayは変調を反映して呼吸する) ---
    const float posNorm = proc.apvts.getRawParameterValue("position")->load();
    const float sprayNorm = juce::jlimit(0.0f, 1.0f,
        proc.apvts.getRawParameterValue("positionSpray")->load()
        + proc.modMatrix.get(ModMatrix::DstSpray));
    const float posX = geo.effX + posNorm * geo.effW;

    if (sprayNorm > 0.001f)
    {
        // SprayバンドはStart/Endの実効領域内にクリップ（グレインも領域外には出ない）
        const auto regionRect = juce::Rectangle<float>(geo.effX, wave.getY(), geo.effW, wave.getHeight());
        const float sprayW = sprayNorm * geo.effW;
        auto sprayRect = juce::Rectangle<float>(posX - sprayW, wave.getY(), sprayW * 2.0f, wave.getHeight());
        sprayRect = sprayRect.getIntersection(regionRect);
        g.setColour(GranularColors::pink.withAlpha(0.10f));
        g.fillRoundedRectangle(sprayRect, 4.0f);
    }

    g.setColour(GranularColors::pink.withAlpha(0.25f));
    g.fillRect(posX - 2.0f, wave.getY(), 4.0f, wave.getHeight());
    g.setColour(GranularColors::pink);
    g.fillRect(posX - 0.75f, wave.getY(), 1.5f, wave.getHeight());

    // --- Position への変調を反映（レンジ帯 + 動く変調後ライン） ---
    {
        const float rMin = proc.modMatrix.getRangeMin(ModMatrix::DstPosition);
        const float rMax = proc.modMatrix.getRangeMax(ModMatrix::DstPosition);
        if (std::abs(rMax - rMin) > 1.0e-4f)
        {
            // 変調で到達しうる範囲を淡いバンドで表示（実効領域内にクリップ）
            const float loN = juce::jlimit(0.0f, 1.0f, posNorm + rMin);
            const float hiN = juce::jlimit(0.0f, 1.0f, posNorm + rMax);
            const float bandX = geo.effX + loN * geo.effW;
            const float bandW = juce::jmax(1.0f, (hiN - loN) * geo.effW);
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.fillRoundedRectangle(juce::Rectangle<float>(bandX, wave.getY(), bandW, wave.getHeight()), 3.0f);

            // 現在の変調後 Position（白い明るいラインで動く）
            const float liveN = juce::jlimit(0.0f, 1.0f, posNorm + proc.modMatrix.get(ModMatrix::DstPosition));
            const float liveX = geo.effX + liveN * geo.effW;
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.fillRect(liveX - 1.0f, wave.getY(), 2.0f, wave.getHeight());
        }
    }

    // --- 動くプレイヘッド (発音中の実再生位置 / ミント色) ---
    const float ph = proc.synth.playheadDisplay.load(std::memory_order_relaxed);
    if (ph >= 0.0f)
    {
        const float phX = geo.effX + juce::jlimit(0.0f, 1.0f, ph) * geo.effW;
        g.setColour(GranularColors::mint.withAlpha(0.20f));
        g.fillRect(phX - 2.0f, wave.getY(), 4.0f, wave.getHeight());
        g.setColour(GranularColors::mint.withAlpha(0.9f));
        g.fillRect(phX - 0.75f, wave.getY(), 1.5f, wave.getHeight());
    }

    // --- ライブ・グレインドット ---
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    for (const auto& d : liveDots)
    {
        const float t = (float)juce::jlimit(0.0, 1.0, (nowMs - d.spawnTimeMs) / d.durationMs);
        const float alpha = (1.0f - t) * (1.0f - t);

        const float gx = geo.effX + d.normPos * geo.effW;
        const float gy = juce::jlimit(wave.getY() + 4.0f, wave.getBottom() - 4.0f,
                                      cy - d.semitones * 4.0f + (d.pan - 0.5f) * 10.0f);
        const float r = 2.5f + 2.5f * (1.0f - t);

        const auto c = GranularColors::grainColour(d.semitones);
        g.setColour(c.withAlpha(0.25f * alpha));
        g.fillEllipse(gx - r * 2.2f, gy - r * 2.2f, r * 4.4f, r * 4.4f);
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
// マウス操作: Start/End マーカー & Position
// ------------------------------------------
void DotWaveformDisplay::mouseMove(const juce::MouseEvent& e)
{
    if (proc.ingestState.load() != IngestState::Ready) { setMouseCursor(juce::MouseCursor::NormalCursor); return; }

    const auto geo = getRegionGeometry();
    const float x = (float)e.position.getX();
    const float startX = geo.waveX + geo.effSN * geo.waveW;
    const float endX = geo.waveX + geo.effEN * geo.waveW;

    const bool nearMarker = std::abs(x - startX) <= kMarkerHitPx || std::abs(x - endX) <= kMarkerHitPx;
    setMouseCursor(nearMarker ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

void DotWaveformDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (proc.ingestState.load() != IngestState::Ready) return;

    const auto geo = getRegionGeometry();
    const float x = (float)e.position.getX();
    const float startX = geo.waveX + geo.effSN * geo.waveW;
    const float endX = geo.waveX + geo.effEN * geo.waveW;

    if (std::abs(x - startX) <= kMarkerHitPx)
    {
        dragMode = DragMode::StartMarker;
        draggedParam = proc.apvts.getParameter("sampleStart");
    }
    else if (std::abs(x - endX) <= kMarkerHitPx)
    {
        dragMode = DragMode::EndMarker;
        draggedParam = proc.apvts.getParameter("sampleEnd");
    }
    else
    {
        dragMode = DragMode::Position;
        draggedParam = proc.apvts.getParameter("position");
    }

    if (draggedParam != nullptr)
    {
        draggedParam->beginChangeGesture();
        updateDraggedParam(e);
    }
}

void DotWaveformDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (dragMode != DragMode::None && draggedParam != nullptr)
        updateDraggedParam(e);
}

void DotWaveformDisplay::mouseUp(const juce::MouseEvent&)
{
    if (draggedParam != nullptr)
        draggedParam->endChangeGesture();
    draggedParam = nullptr;
    dragMode = DragMode::None;
}

void DotWaveformDisplay::updateDraggedParam(const juce::MouseEvent& e)
{
    const auto geo = getRegionGeometry();
    const float normFull = juce::jlimit(0.0f, 1.0f, ((float)e.position.getX() - geo.waveX) / geo.waveW);

    if (dragMode == DragMode::Position)
    {
        const float v = juce::jlimit(0.0f, 1.0f, (normFull * geo.waveW + geo.waveX - geo.effX) / geo.effW);
        draggedParam->setValueNotifyingHost(v);
        return;
    }

    // Start/End: 自動トリム領域内の相対値へ変換
    const float span = juce::jmax(1.0e-6f, geo.autoEN - geo.autoSN);
    float rel = juce::jlimit(0.0f, 1.0f, (normFull - geo.autoSN) / span);

    if (dragMode == DragMode::StartMarker)
    {
        const float endVal = proc.apvts.getRawParameterValue("sampleEnd")->load();
        rel = juce::jmin(rel, endVal - kMinRegion);
        draggedParam->setValueNotifyingHost(juce::jmax(0.0f, rel));
    }
    else if (dragMode == DragMode::EndMarker)
    {
        const float startVal = proc.apvts.getRawParameterValue("sampleStart")->load();
        rel = juce::jmax(rel, startVal + kMinRegion);
        draggedParam->setValueNotifyingHost(juce::jmin(1.0f, rel));
    }
}
