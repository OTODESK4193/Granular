// ==========================================
// File: PitchAnalyzer.h
// Tessellator の Omni-Analyzer を移植した素材ピッチ解析。
// 自己相関 / Kick用DFT / Hat・Cym用ダウンサンプルDFT / ループ / マイクロ素材
// の複数ルートを素材に応じて試行し、クロマグラムで Key(Root) と Maj/Min を推定する。
// ==========================================
#pragma once

#include <JuceHeader.h>

struct PitchInfo
{
    double frequency = 0.0;
    juce::String noteName = "---";
    float centsOffset = 0.0f;
    int midiNote = 60;
    bool isDetected = false;
    int detectedRoot = 0;      // 0..11 (C..B)
    bool isMajor = true;
    juce::String scaleName = "";
};

class PitchAnalyzer
{
public:
    // materialMode: 0=Auto(Detect) 1=Percussive 2=Melodic 3=Vocal
    static PitchInfo analyzeOmni(const float* audioData, int length, double sampleRate,
                                 const juce::String& filename, int materialMode);

    // 基本の自己相関ピッチ検出（放物線補間つき）
    static PitchInfo analyzeAutocorrelation(const float* audioData, int length, double sampleRate,
                                            float minFreq, float corrThreshold);
};
