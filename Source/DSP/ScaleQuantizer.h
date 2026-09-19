// ==========================================
// File: ScaleQuantizer.h
// Master Pitch のスケール量子化 (Wavetable / LIFT-X 完全移植)
//
//  使い方:
//    float snapped = ScaleQuantizer::quantize(absSemis, rootNote, scaleIndex);
//    absSemis  : 絶対ピッチ (MIDIノート番号スケール, 小数可)
//    rootNote  : 0..11 (C..B)
//    scaleIndex: パラメーター "scale" のインデックスと1:1対応 (0..81)
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace ScaleQuantizer
{
    static constexpr int kMaxDegrees = 12;
    static constexpr int kNumScales  = 82;

    struct ScaleDef
    {
        const char* name;
        int size;                                  // 使用する度数の数
        std::array<int, kMaxDegrees> degrees;      // ルートからの半音オフセット (0..11)
    };

    // ------------------------------------------------------------
    // インデックスは AudioParameterChoice "scale" と1:1対応させること。
    // 全82種 (Wavetable / LIFT-X 準拠)
    // ------------------------------------------------------------
    inline const std::array<ScaleDef, kNumScales>& getScales()
    {
        static const std::array<ScaleDef, kNumScales> scales = { {
            // ---- 基本 (Granular 互換) ----
            { "Chromatic",         12, { 0,1,2,3,4,5,6,7,8,9,10,11 } },
            { "Major (Ionian)",     7, { 0,2,4,5,7,9,11 } },
            { "Natural Minor",      7, { 0,2,3,5,7,8,10 } },
            { "Major Pentatonic",   5, { 0,2,4,7,9 } },
            { "Minor Pentatonic",   5, { 0,3,5,7,10 } },
            { "Dorian",             7, { 0,2,3,5,7,9,10 } },
            { "Lydian",             7, { 0,2,4,6,7,9,11 } },
            { "Mixolydian",         7, { 0,2,4,5,7,9,10 } },
            { "Phrygian",           7, { 0,1,3,5,7,8,10 } },
            { "Harmonic Minor",     7, { 0,2,3,5,7,8,11 } },
            { "Melodic Minor",      7, { 0,2,3,5,7,9,11 } },
            { "Whole Tone",         6, { 0,2,4,6,8,10 } },
            { "Octaves & Fifths",   2, { 0,7 } },
            { "Quartal (4ths)",     3, { 0,5,10 } },
            { "Sus2/4 Cloud",       4, { 0,2,5,7 } },
            { "Diminished 7th",     4, { 0,3,6,9 } },
            // ---- 教会旋法・派生モード ----
            { "Locrian",            7, { 0,1,3,5,6,8,10 } },
            { "Blues Minor",        6, { 0,3,5,6,7,10 } },
            { "Blues Major",        6, { 0,2,3,4,7,9 } },
            { "Harmonic Major",     7, { 0,2,4,5,7,8,11 } },
            { "Double Harmonic",    7, { 0,1,4,5,7,8,11 } },
            { "Phrygian Dominant",  7, { 0,1,4,5,7,8,10 } },
            { "Lydian Dominant",    7, { 0,2,4,6,7,9,10 } },
            { "Lydian Augmented",   7, { 0,2,4,6,8,9,11 } },
            { "Mixolydian b6",      7, { 0,2,4,5,7,8,10 } },
            { "Half Diminished",    7, { 0,2,3,5,6,8,10 } },
            { "Altered (Super Loc)",7, { 0,1,3,4,6,8,10 } },
            { "Dorian b2",          7, { 0,1,3,5,7,9,10 } },
            { "Dorian #4",          7, { 0,2,3,6,7,9,10 } },
            { "Lydian #2",          7, { 0,3,4,6,7,9,11 } },
            { "Ultra Locrian",      7, { 0,1,3,4,6,8,9 } },
            // ---- 民族音階 (欧州・中東) ----
            { "Hungarian Minor",    7, { 0,2,3,6,7,8,11 } },
            { "Hungarian Major",    7, { 0,3,4,6,7,9,10 } },
            { "Neapolitan Minor",   7, { 0,1,3,5,7,8,11 } },
            { "Neapolitan Major",   7, { 0,1,3,5,7,9,11 } },
            { "Enigmatic",          7, { 0,1,4,6,8,10,11 } },
            { "Persian",            7, { 0,1,4,5,6,8,11 } },
            { "Oriental",           7, { 0,1,4,5,6,9,10 } },
            { "Nine-Tone",          9, { 0,2,3,4,6,7,8,9,11 } },
            { "Spanish 8-Tone",     8, { 0,1,3,4,5,6,8,10 } },
            // ---- インド音階 ----
            { "Todi (Indian)",      7, { 0,1,3,6,7,8,11 } },
            { "Marva (Indian)",     7, { 0,1,4,6,7,9,11 } },
            { "Purvi (Indian)",     7, { 0,1,4,6,7,8,11 } },
            { "Ahir Bhairav (Ind)", 7, { 0,1,4,5,7,9,10 } },
            // ---- 日本・アジア音階 ----
            { "Hirajoshi (JP)",     5, { 0,2,3,7,8 } },
            { "In-Sen (JP)",        5, { 0,1,5,7,10 } },
            { "Iwato (JP)",         5, { 0,1,5,6,10 } },
            { "Kumoi (JP)",         5, { 0,2,3,7,9 } },
            { "Yo / Ryo (JP)",      5, { 0,2,5,7,9 } },
            { "Miyako-Bushi (JP)",  5, { 0,1,5,7,8 } },
            { "Ryukyu (JP)",        5, { 0,4,5,7,11 } },
            { "Chinese Jiao",       5, { 0,3,5,8,10 } },
            { "Egyptian Pent",      5, { 0,2,5,7,10 } },
            { "Balinese Pelog",     5, { 0,1,3,7,8 } },
            // ---- 対称・シンセ向き ----
            { "Prometheus",         6, { 0,2,4,6,9,10 } },
            { "Tritone",            6, { 0,1,4,6,7,10 } },
            { "Augmented",          6, { 0,3,4,7,8,11 } },
            { "Half-Whole Dim",     8, { 0,1,3,4,6,7,9,10 } },
            { "Whole-Half Dim",     8, { 0,2,3,5,6,8,9,11 } },
            { "Bebop Dominant",     8, { 0,2,4,5,7,9,10,11 } },
            { "Bebop Major",        8, { 0,2,4,5,7,8,9,11 } },
            // ---- コードトーン (アルペジオ的ライザー向け) ----
            { "Major Triad",        3, { 0,4,7 } },
            { "Minor Triad",        3, { 0,3,7 } },
            { "Sus4 Triad",         3, { 0,5,7 } },
            { "Major 7th",          4, { 0,4,7,11 } },
            { "Minor 7th",          4, { 0,3,7,10 } },
            { "Dominant 7th",       4, { 0,4,7,10 } },
            { "Minor 9th",          5, { 0,2,3,7,10 } },
            { "Major 9th",          5, { 0,2,4,7,11 } },
            { "Octaves Only",       1, { 0 } },
            // ---- ジャズ・派生 ----
            { "Bebop Minor",        8, { 0,2,3,4,5,7,9,10 } },
            { "Bebop Dorian",       8, { 0,2,3,4,5,7,9,11 } },
            // ---- 民族音階 ----
            { "Romanian Minor",     7, { 0,2,3,6,7,9,10 } },
            { "Byzantine",          7, { 0,1,4,5,7,8,11 } },
            { "Slendro (Java)",     5, { 0,2,5,7,9 } },
            // ---- メシアン・移調の限られた旋法 ----
            { "Messiaen Mode 3",    9, { 0,2,3,4,6,7,8,10,11 } },
            { "Messiaen Mode 4",    8, { 0,1,2,5,6,7,8,11 } },
            { "Messiaen Mode 6",    8, { 0,2,4,5,6,8,10,11 } },
            // ---- 拡張コードトーン ----
            { "Major 6th",          4, { 0,4,7,9 } },
            { "Minor 6th",          4, { 0,3,7,9 } },
            { "Major Add9",         4, { 0,2,4,7 } },
            { "Minor Add9",         4, { 0,2,3,7 } },
        } };
        return scales;
    }

    inline int numScales() noexcept { return (int)getScales().size(); }

    inline const char* scaleName(int idx) noexcept
    {
        const auto& s = getScales();
        if (idx < 0 || idx >= (int)s.size()) return "Chromatic";
        return s[(size_t)idx].name;
    }

    inline juce::StringArray getAllScaleNames()
    {
        juce::StringArray names;
        for (const auto& s : getScales())
            names.add(s.name);
        return names;
    }

    // ------------------------------------------------------------
    // ツリー方式（カテゴリ別階層メニュー）構築ヘルパー
    // ------------------------------------------------------------
    inline juce::PopupMenu buildScaleMenu(int currentScaleIndex)
    {
        juce::PopupMenu rootMenu;
        const auto& s = getScales();

        auto addItems = [&](juce::PopupMenu& m, const std::vector<int>& indices)
        {
            for (int idx : indices)
            {
                if (idx >= 0 && idx < (int)s.size())
                    m.addItem(idx + 1, s[(size_t)idx].name, true, idx == currentScaleIndex);
            }
        };

        // 1. Basic / Western
        juce::PopupMenu basicMenu;
        addItems(basicMenu, { 0, 1, 2, 3, 4, 9, 10, 19, 20 });
        rootMenu.addSubMenu("Basic / Western", basicMenu);

        // 2. Church Modes
        juce::PopupMenu churchMenu;
        addItems(churchMenu, { 1, 5, 8, 6, 7, 2, 16 });
        rootMenu.addSubMenu("Church Modes", churchMenu);

        // 3. Jazz & Modern
        juce::PopupMenu jazzMenu;
        addItems(jazzMenu, { 17, 18, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 57, 58, 59, 60, 70, 71 });
        rootMenu.addSubMenu("Jazz & Modern", jazzMenu);

        // 4. World & Ethnic
        juce::PopupMenu worldMenu;
        juce::PopupMenu midEastMenu;
        addItems(midEastMenu, { 31, 32, 33, 34, 36, 37, 39, 72, 73 });
        worldMenu.addSubMenu("Europe & Mid-East", midEastMenu);

        juce::PopupMenu indMenu;
        addItems(indMenu, { 40, 41, 42, 43 });
        worldMenu.addSubMenu("Indian Ragas", indMenu);

        juce::PopupMenu asiaMenu;
        addItems(asiaMenu, { 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 74 });
        worldMenu.addSubMenu("Japanese & Asian", asiaMenu);
        rootMenu.addSubMenu("World & Ethnic", worldMenu);

        // 5. Synthetic & Exotic
        juce::PopupMenu synthMenu;
        addItems(synthMenu, { 11, 35, 38, 54, 55, 56, 75, 76, 77 });
        rootMenu.addSubMenu("Synthetic & Exotic", synthMenu);

        // 6. Chord Tones & Arpeggios
        juce::PopupMenu chordMenu;
        addItems(chordMenu, { 61, 62, 63, 14, 64, 65, 66, 15, 67, 68, 78, 79, 80, 81, 13, 12, 69 });
        rootMenu.addSubMenu("Chord Tones & Arps", chordMenu);

        return rootMenu;
    }

    // ルート名 (0..11)
    inline const char* keyName(int root) noexcept
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B" };
        return names[((root % 12) + 12) % 12];
    }

    // Key + Scale の構成音名を "C D E ..." の形で返す（GUI 表示用）
    inline juce::String getNoteNames(int rootNote, int scaleIndex)
    {
        const auto& scales = getScales();
        if (scaleIndex < 0 || scaleIndex >= (int)scales.size()) return {};

        const auto& s = scales[(size_t)scaleIndex];
        const int root = ((rootNote % 12) + 12) % 12;

        juce::StringArray names;
        for (int i = 0; i < s.size; ++i)
            names.add(keyName(root + s.degrees[(size_t)i]));

        return names.joinIntoString(" ");
    }

    // 絶対ピッチ(半音, 小数可) を Key+Scale の最も近い構成音へスナップ
    inline float quantize(float absSemis, int rootNote, int scaleIndex) noexcept
    {
        const auto& scales = getScales();
        if (scaleIndex < 0 || scaleIndex >= (int)scales.size())
            return absSemis;

        const auto& s = scales[(size_t)scaleIndex];
        if (s.size <= 0)
            return absSemis;

        const float root = (float)(((rootNote % 12) + 12) % 12);

        float best = absSemis;
        float bestDist = 1.0e9f;

        for (int i = 0; i < s.size; ++i)
        {
            const float base = root + (float)s.degrees[(size_t)i];
            const float k = std::round((absSemis - base) / 12.0f);
            const float candidate = base + 12.0f * k;
            const float dist = std::abs(absSemis - candidate);
            if (dist < bestDist) { bestDist = dist; best = candidate; }
        }
        return best;
    }
}
