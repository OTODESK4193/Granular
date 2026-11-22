// ==========================================
// File: ArcDial.cpp
// ==========================================
#include "ArcDial.h"
#include "ColorPalette.h"
#include <cmath>

ArcDialLookAndFeel::ArcDialLookAndFeel()
{
    refreshColours();
}

void ArcDialLookAndFeel::refreshColours()
{
    setColour(juce::Slider::textBoxTextColourId, GranularColors::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::trackColourId, GranularColors::mint);
    setColour(juce::Slider::backgroundColourId, GranularColors::knobTrack);
    setColour(juce::Slider::thumbColourId, GranularColors::text);
    setColour(juce::ComboBox::backgroundColourId, GranularColors::panel);
    setColour(juce::ComboBox::textColourId, GranularColors::text);
    setColour(juce::ComboBox::outlineColourId, GranularColors::panelLine);
    setColour(juce::ComboBox::arrowColourId, GranularColors::textDim);
    setColour(juce::PopupMenu::backgroundColourId, GranularColors::panel);
    setColour(juce::PopupMenu::textColourId, GranularColors::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, GranularColors::lavender.withAlpha(0.3f));
    setColour(juce::PopupMenu::highlightedTextColourId, GranularColors::text);
    setColour(juce::ToggleButton::textColourId, GranularColors::text);
    setColour(juce::ToggleButton::tickColourId, GranularColors::mint);
    setColour(juce::ToggleButton::tickDisabledColourId, GranularColors::textDim);
    setColour(juce::Label::textColourId, GranularColors::textDim);
}

void ArcDialLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    const auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    if (radius <= 2.0f) return;

    const auto centreX = (float)x + (float)width * 0.5f;
    const auto centreY = (float)y + (float)height * 0.5f;
    const auto rx = centreX - radius;
    const auto ry = centreY - radius;
    const auto rw = radius * 2.0f;
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcThickness = 5.0f;

    // 1. 背景トラック
    g.setColour(GranularColors::knobTrack);
    g.drawEllipse(rx, ry, rw, rw, arcThickness);

    // 1.5 モジュレーション・レンジ帯（値アークの下にピンクのハロー）
    //     GUI側が mod_active / mod_min / mod_max / mod_live プロパティを毎フレーム更新
    const auto& props = slider.getProperties();
    const bool modActive = props.getWithDefault("mod_active", false);
    if (modActive)
    {
        const float mMin = juce::jlimit(0.0f, 1.0f, (float)props.getWithDefault("mod_min", 0.0f));
        const float mMax = juce::jlimit(0.0f, 1.0f, (float)props.getWithDefault("mod_max", 1.0f));
        const auto aLo = rotaryStartAngle + juce::jmin(mMin, mMax) * (rotaryEndAngle - rotaryStartAngle);
        const auto aHi = rotaryStartAngle + juce::jmax(mMin, mMax) * (rotaryEndAngle - rotaryStartAngle);

        if (std::abs(aHi - aLo) > 0.001f)
        {
            juce::Path band;
            band.addArc(rx, ry, rw, rw, aLo, aHi, true);
            g.setColour(GranularColors::pink.withAlpha(0.55f));
            g.strokePath(band, juce::PathStrokeType(arcThickness + 4.0f,
                         juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // 2. 値アーク（セクション色ベースのパステルグラデーション）
    juce::Path p;
    p.addArc(rx, ry, rw, rw, rotaryStartAngle, angle, true);

    const auto baseColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    const auto lightColour = baseColour.brighter(0.6f);
    const auto darkColour = baseColour.darker(0.35f);

    juce::ColourGradient gradient(darkColour, rx, centreY, lightColour, rx + rw, centreY, false);
    g.setGradientFill(gradient);
    g.strokePath(p, juce::PathStrokeType(arcThickness, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // 3. ソフトグロー
    g.setColour(baseColour.withAlpha(0.15f));
    g.strokePath(p, juce::PathStrokeType(arcThickness + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 4. ポインター
    juce::Path p2;
    const auto pointerLength = radius * 0.4f;
    p2.addRoundedRectangle(-1.5f, -radius + 1.5f, 3.0f, pointerLength, 1.5f);
    p2.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(GranularColors::text);
    g.fillPath(p2);

    // 5. ライブ変調ドット（現在の変調後の値をアーク上の明るいピンク点で表示）
    if (modActive)
    {
        const float live = juce::jlimit(0.0f, 1.0f, (float)props.getWithDefault("mod_live", sliderPos));
        const auto aLive = rotaryStartAngle + live * (rotaryEndAngle - rotaryStartAngle);
        const float dotX = centreX + std::sin(aLive) * radius;
        const float dotY = centreY - std::cos(aLive) * radius;
        g.setColour(GranularColors::pink.withAlpha(0.40f));
        g.fillEllipse(dotX - 5.0f, dotY - 5.0f, 10.0f, 10.0f);
        g.setColour(GranularColors::pink);
        g.fillEllipse(dotX - 2.6f, dotY - 2.6f, 5.2f, 5.2f);
    }
}

void ArcDialLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float minSliderPos, float maxSliderPos,
                                          juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                               minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const float cy = (float)y + (float)height * 0.5f;
    const float trackH = 4.0f;
    const auto track = juce::Rectangle<float>((float)x, cy - trackH * 0.5f, (float)width, trackH);

    g.setColour(GranularColors::knobTrack);
    g.fillRoundedRectangle(track, trackH * 0.5f);

    // 中央 (0) からの双方向バー (MOD Amount / 右クリックインライン編集用)
    const auto range = slider.getNormalisableRange();
    const bool bipolar = (range.start < 0.0 && range.end > 0.0);
    const float zeroX = bipolar
        ? (float)x + (float)width * (float)range.convertTo0to1(0.0)
        : (float)x;

    const auto fill = slider.findColour(juce::Slider::trackColourId);
    const float lo = juce::jmin(zeroX, sliderPos);
    const float hi = juce::jmax(zeroX, sliderPos);
    if (hi - lo > 0.5f)
    {
        g.setColour(fill);
        g.fillRoundedRectangle(juce::Rectangle<float>(lo, cy - trackH * 0.5f, hi - lo, trackH), trackH * 0.5f);
    }

    // 中央0の小さな目盛り
    if (bipolar)
    {
        g.setColour(GranularColors::textDim.withAlpha(0.45f));
        g.fillRect(zeroX - 0.5f, cy - 4.5f, 1.0f, 9.0f);
    }

    // つまみ
    g.setColour(fill.brighter(0.35f));
    g.fillEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
    g.setColour(GranularColors::bg.withAlpha(0.8f));
    g.drawEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f, 1.0f);
}
