// ==========================================
// File: ScaleQuantizer.h
// グレインピッチ量子化（変調・ランダム化の「後」に適用する）
//
// 使い方:
//   float snapped = ScaleQuantizer::quantize(absSemis, rootNote, scaleIndex);
//   absSemis  : 絶対ピッチ（MIDIノート番号スケール、小数可。60.0 = C3基準）
//   rootNote  : 0..11 (C..B)
//   scaleIndex: パラメータ"scale"のインデックスと一致
// ==========================================
#pragma once

#include <array>
#include <cmath>
#include <cstdlib>

namespace ScaleQuantizer
{
    struct ScaleDef
    {
        const char* name;
        int size;                       // 使用する度数の数
        std::array<int, 12> degrees;    // ルートからの半音オフセット (0..11)
    };

    // インデックスは AudioParameterChoice "scale" と1:1対応させること
    inline const std::array<ScaleDef, 17>& getScales()
    {
        static const std::array<ScaleDef, 17> scales = { {
            { "Free",             0,  { } },                                   // 0: 量子化なし
            { "Chromatic",        12, { 0,1,2,3,4,5,6,7,8,9,10,11 } },         // 1
            { "Major",            7,  { 0,2,4,5,7,9,11 } },                    // 2
            { "Natural Minor",    7,  { 0,2,3,5,7,8,10 } },                    // 3
            { "Major Pentatonic", 5,  { 0,2,4,7,9 } },                         // 4
            { "Minor Pentatonic", 5,  { 0,3,5,7,10 } },                        // 5
            { "Dorian",           7,  { 0,2,3,5,7,9,10 } },                    // 6
            { "Lydian",           7,  { 0,2,4,6,7,9,11 } },                    // 7
            { "Mixolydian",       7,  { 0,2,4,5,7,9,10 } },                    // 8
            { "Phrygian",         7,  { 0,1,3,5,7,8,10 } },                    // 9
            { "Harmonic Minor",   7,  { 0,2,3,5,7,8,11 } },                    // 10
            { "Melodic Minor",    7,  { 0,2,3,5,7,9,11 } },                    // 11
            { "Whole Tone",       6,  { 0,2,4,6,8,10 } },                      // 12
            { "Octaves & Fifths", 2,  { 0,7 } },                               // 13
            { "Quartal (4ths)",   3,  { 0,5,10 } },                            // 14
            { "Sus2/4 Cloud",     4,  { 0,2,5,7 } },                           // 15
            { "Diminished",       4,  { 0,3,6,9 } },                           // 16
        } };
        return scales;
    }

    // 絶対ピッチ(半音, 小数可)を Key+Scale の最も近い構成音へスナップする。
    // 隣接オクターブも含めて最短距離の音を選ぶため、スケール端でも自然に折り返す。
    inline float quantize(float absSemis, int rootNote, int scaleIndex) noexcept
    {
        const auto& scales = getScales();
        if (scaleIndex <= 0 || scaleIndex >= (int)scales.size())
            return absSemis; // Free

        const auto& s = scales[(size_t)scaleIndex];
        if (s.size <= 0)
            return absSemis;

        float best = absSemis;
        float bestDist = 1.0e9f;

        for (int i = 0; i < s.size; ++i)
        {
            const float base = (float)(rootNote + s.degrees[(size_t)i]);
            // absSemis に最も近いオクターブ位置を直接計算
            const float k = std::round((absSemis - base) / 12.0f);
            const float candidate = base + 12.0f * k;
            const float dist = std::abs(absSemis - candidate);
            if (dist < bestDist) { bestDist = dist; best = candidate; }
        }
        return best;
    }
}
