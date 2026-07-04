// ==========================================
// File: ColorPalette.h
// パステル・カラーテーマ（ダーク背景 × パステルアクセント）
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <array>

namespace GranularColors
{
    // 背景・基本色
    inline const juce::Colour bg        { 0xff17141f };  // ダークプラム
    inline const juce::Colour panel     { 0xff201c2b };
    inline const juce::Colour panelLine { 0x22ffffff };
    inline const juce::Colour grid      { 0x14ffffff };
    inline const juce::Colour text      { 0xffe9e3f2 };
    inline const juce::Colour textDim   { 0xff8d86a0 };
    inline const juce::Colour knobTrack { 0xff2a2536 };

    // パステルパレット
    inline const juce::Colour mint      { 0xffb5ead7 };
    inline const juce::Colour pink      { 0xffffb7c5 };
    inline const juce::Colour lavender  { 0xffc7ceea };
    inline const juce::Colour peach     { 0xffffdac1 };
    inline const juce::Colour babyBlue  { 0xffaed9f7 };
    inline const juce::Colour sage      { 0xffe2f0cb };
    inline const juce::Colour rose      { 0xffffb7b2 };
    inline const juce::Colour lilac     { 0xffe0c3fc };

    // セクションアクセント
    inline const juce::Colour accentGrain  = mint;
    inline const juce::Colour accentPitch  = lavender;
    inline const juce::Colour accentEnv    = peach;
    inline const juce::Colour accentMaster = pink;

    // 波形ドットの横方向グラデーション（mint → babyBlue → lavender → pink）
    inline juce::Colour waveformGradient(float t01) noexcept
    {
        t01 = juce::jlimit(0.0f, 1.0f, t01);
        if (t01 < 0.3333f)  return mint.interpolatedWith(babyBlue, t01 / 0.3333f);
        if (t01 < 0.6667f)  return babyBlue.interpolatedWith(lavender, (t01 - 0.3333f) / 0.3334f);
        return lavender.interpolatedWith(pink, (t01 - 0.6667f) / 0.3333f);
    }

    // グレインドット色: ピッチ(C3基準の半音差)→ 色相へマッピング
    inline juce::Colour grainColour(float semitones) noexcept
    {
        // -24..+24 半音 を 0..1 に正規化
        const float t = juce::jlimit(0.0f, 1.0f, (semitones + 24.0f) / 48.0f);
        static const std::array<juce::Colour, 5> ramp { rose, peach, sage, babyBlue, lilac };
        const float f = t * (float)(ramp.size() - 1);
        const int idx = juce::jlimit(0, (int)ramp.size() - 2, (int)f);
        return ramp[(size_t)idx].interpolatedWith(ramp[(size_t)idx + 1], f - (float)idx);
    }
}
