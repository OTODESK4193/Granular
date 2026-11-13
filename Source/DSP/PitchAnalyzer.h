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
    float clarity = 0.0f;
    int midiNote = 60;
    bool isDetected = false;
    int detectedRoot = 0;      // 0..11 (C..B)
    bool isMajor = true;
    juce::String scaleName = "";
};

// ファイル名から抽出したKeyヒント ("Pad-C4" / "Bass(D)" / "loop_Cm" 等)
struct FilenameHint
{
    int  pitchClass = -1;    // 0..11 (C..B), -1=なし
    int  octave = -128;      // Ableton表記のオクターブ (-128=なし)
    bool isMinor = false;
    bool hasQuality = false; // maj/min 表記があったか

    bool isValid() const noexcept { return pitchClass >= 0; }
};

class PitchAnalyzer
{
public:
    // materialMode: 0=Auto(Detect) 1=Percussive 2=Melodic 3=Vocal
    // ファイル名にKeyヒントがあれば解析は「高さの特定」に専念し、KeyはヒントでFixする
    static PitchInfo analyzeOmni(const float* audioData, int length, double sampleRate,
                                 const juce::String& filename, int materialMode);

    // 正規化自己相関ピッチ検出（放物線補間＋オクターブエラーガード）
    static PitchInfo analyzeAutocorrelation(const float* audioData, int length, double sampleRate,
                                            float minFreq, float corrThreshold);

    // ファイル名からのKey抽出
    static FilenameHint parseFilenameHint(const juce::String& filename);

    // MIDIノート番号 → Ableton表記 (C3 = MIDI 60)
    static juce::String midiNoteName(int midiNote);
};
