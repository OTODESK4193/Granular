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
    static constexpr int kMaxEvents = 1024;

    struct EventData
    {
        float normPos = 0.0f;      // サンプル内の正規化位置 0..1
        float pan = 0.5f;          // 0(L)..1(R)
        float semitones = 0.0f;    // C3基準の相対ピッチ（表示色に使用）
        float durationSec = 0.1f;  // グレイン長（ドットの寿命）
    };

    struct Event
    {
        std::atomic<uint32_t> seq { 0 }; // 偶数=確定, 奇数=書込中 (SeqLock)
        float normPos = 0.0f;
        float pan = 0.5f;
        float semitones = 0.0f;
        float durationSec = 0.1f;
    };

    std::array<Event, kMaxEvents> events {};
    std::atomic<uint32_t> counter { 0 };

    // 音声スレッドから呼ぶ（アロケーション・ロックなし）
    void push(float normPos, float pan, float semitones, float durationSec) noexcept
    {
        const auto c = counter.load(std::memory_order_relaxed);
        auto& e = events[(size_t)(c % kMaxEvents)];
        const uint32_t s = e.seq.load(std::memory_order_relaxed);
        e.seq.store((s + 1u) | 1u, std::memory_order_release);
        e.normPos = normPos;
        e.pan = pan;
        e.semitones = semitones;
        e.durationSec = durationSec;
        e.seq.store((s + 2u) & ~1u, std::memory_order_release);
        counter.store(c + 1, std::memory_order_release);
    }

    // GUIスレッドから呼ぶ（SeqLockによる破損・トーンリード防止）
    bool tryRead(uint32_t ringIndex, EventData& out) const noexcept
    {
        const auto& e = events[(size_t)(ringIndex % kMaxEvents)];
        for (int tries = 0; tries < 4; ++tries)
        {
            const uint32_t s1 = e.seq.load(std::memory_order_acquire);
            if ((s1 & 1u) != 0u) continue;
            EventData tmp;
            tmp.normPos = e.normPos;
            tmp.pan = e.pan;
            tmp.semitones = e.semitones;
            tmp.durationSec = e.durationSec;
            std::atomic_thread_fence(std::memory_order_acquire);
            const uint32_t s2 = e.seq.load(std::memory_order_acquire);
            if (s1 == s2)
            {
                out = tmp;
                return true;
            }
        }
        return false;
    }
};
