// ==========================================
// File: GrainVisualizerData.h
// オーディオスレッド → GUI へのグレイン発火イベント受け渡し（ロックフリー）
// GUI側はカウンタ差分だけ読み、ドットとしてアニメーション表示する。
// ==========================================
#pragma once

#include <array>
#include <atomic>
#include <cstdint>

struct GrainVisualizerData
{
    static constexpr int kMaxEvents = 256;

    struct Event
    {
        float normPos = 0.0f;      // サンプル内の正規化位置 0..1
        float pan = 0.5f;          // 0(L)..1(R)
        float semitones = 0.0f;    // C3基準の相対ピッチ（表示色に使用）
        float durationSec = 0.1f;  // グレイン長（ドットの寿命）
    };

    std::array<Event, kMaxEvents> events {};
    std::atomic<uint32_t> counter { 0 };

    // 音声スレッドから呼ぶ（アロケーション・ロックなし）
    void push(float normPos, float pan, float semitones, float durationSec) noexcept
    {
        const auto c = counter.load(std::memory_order_relaxed);
        auto& e = events[(size_t)(c % kMaxEvents)];
        e.normPos = normPos;
        e.pan = pan;
        e.semitones = semitones;
        e.durationSec = durationSec;
        counter.store(c + 1, std::memory_order_release);
    }
};
