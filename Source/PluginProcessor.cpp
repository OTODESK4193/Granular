// ==========================================
// File: PluginProcessor.cpp
// ==========================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets/PresetData.h"

// サードパーティ(Signalsmith)内部の無害な警告を抑制
// C4244: double→float変換(DSPライブラリの意図的設計) / C4456,4458: 変数の隠蔽(スタイル)
#if defined(_MSC_VER)
 #pragma warning(push)
 #pragma warning(disable: 4244 4456 4458)
#endif
#include "signalsmith-stretch/signalsmith-stretch.h"
#if defined(_MSC_VER)
 #pragma warning(pop)
#endif

#include <algorithm>
#include <fstream>
#include <cstring>
#include <cmath>

// 80-bit IEEE-754 拡張精度浮動小数点数をパースするヘルパー (AIFF-Cサンプルレート用)
static double readExtendedFloat(const uint8_t* bytes) noexcept
{
    uint16_t exp = ((uint16_t)bytes[0] << 8) | bytes[1];
    uint64_t mant = 0;
    for (int i = 0; i < 8; ++i)
        mant = (mant << 8) | bytes[2 + i];

    if (exp == 0 && mant == 0) return 0.0;
    
    // サンプルレートは常に正と仮定
    double val = (double)mant * std::pow(2.0, (double)(int)exp - 16383.0 - 63.0);
    return val;
}

// 4バイトのビッグエンディアンをリトルエンディアンの float にスワップする
static float swapFloat(float f) noexcept
{
    union { float f; uint8_t b[4]; } uIn, uOut;
    uIn.f = f;
    uOut.b[0] = uIn.b[3];
    uOut.b[1] = uIn.b[2];
    uOut.b[2] = uIn.b[1];
    uOut.b[3] = uIn.b[0];
    return uOut.f;
}

static uint32_t readBigEndian32(const uint8_t* b) noexcept
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

static uint16_t readBigEndian16(const uint8_t* b) noexcept
{
    return ((uint16_t)b[0] << 8) | b[1];
}

// Ableton Live が内部結合 (Ctrl+J) やファクトリー Pack 等で使用する 32-bit float / 16-bit / 24-bit AIFF-C (able / fl32) ファイルを、
// JUCE の組み込み AiffAudioFormat に代わって独自に解析・ロードするフォールバック・デコーダー
bool GranularAudioProcessor::readAbletonAif(const juce::File& file, juce::AudioBuffer<float>& destBuffer, double& sampleRate)
{
    std::ifstream fs(file.getFullPathName().toWideCharPointer(), std::ios::binary);
    if (!fs.is_open()) return false;

    char header[12];
    if (!fs.read(header, 12)) return false;

    // "FORM" と "AIFC" のシグネチャをチェック
    if (std::memcmp(header, "FORM", 4) != 0 || std::memcmp(header + 8, "AIFC", 4) != 0)
        return false;

    uint32_t fileSize = readBigEndian32((const uint8_t*)header + 4);

    uint32_t commOffset = 0;
    uint32_t commSize = 0;
    uint32_t ssndOffset = 0;
    uint32_t ssndSize = 0;

    // IFF チャンクを走査
    while (fs.tellg() < (std::streampos)(fileSize + 8) && !fs.eof())
    {
        char chunkId[4];
        char chunkSizeBuf[4];
        if (!fs.read(chunkId, 4) || !fs.read(chunkSizeBuf, 4)) break;

        uint32_t chunkSize = readBigEndian32((const uint8_t*)chunkSizeBuf);
        const std::streampos chunkDataPos = fs.tellg();
        std::streampos nextChunk = chunkDataPos + (std::streamoff)chunkSize;
        if (chunkSize % 2 != 0) nextChunk += 1; // 奇数サイズ時のアライメント
        if (nextChunk <= chunkDataPos) break;   // 破損チャンクサイズによる無限ループ防止

        if (std::memcmp(chunkId, "COMM", 4) == 0)
        {
            commOffset = (uint32_t)chunkDataPos;
            commSize = chunkSize;
        }
        else if (std::memcmp(chunkId, "SSND", 4) == 0)
        {
            ssndOffset = (uint32_t)chunkDataPos;
            ssndSize = chunkSize;
        }

        fs.seekg(nextChunk);
    }

    if (commOffset == 0 || ssndOffset == 0 || commSize < 22 || ssndSize < 8) return false;

    // COMM チャンク解析
    fs.seekg(commOffset);
    uint8_t commData[18];
    if (!fs.read((char*)commData, 18)) return false;

    uint16_t numChannels = readBigEndian16(commData);
    uint32_t numSampleFrames = readBigEndian32(commData + 2);
    uint16_t sampleSize = readBigEndian16(commData + 6);
    double sr = readExtendedFloat(commData + 8);

    if (numChannels == 0 || numChannels > 8 || numSampleFrames == 0) return false;
    if (!std::isfinite(sr) || sr < 8000.0 || sr > 384000.0) return false;

    // 圧縮タイプの判定
    char compType[4] = {0};
    fs.read(compType, 4);

    // 'able' は Ableton Pack 独自のプロプライエタリ暗号化/圧縮コンテナのためデコード不可。即座に拒否
    if (std::memcmp(compType, "able", 4) == 0)
        return false;

    // デコード対応判定: 'fl32', 'FL32', 'NONE', 'none', 'sowt'
    bool isFloat32 = (std::memcmp(compType, "fl32", 4) == 0 || std::memcmp(compType, "FL32", 4) == 0);
    bool isPcmStandard = (std::memcmp(compType, "NONE", 4) == 0 || std::memcmp(compType, "none", 4) == 0
                          || std::memcmp(compType, "sowt", 4) == 0);

    // いずれの対応タイプでもない場合は、JUCEの標準デコーダーにフォールバックさせるため処理を中断
    if (!isFloat32 && !isPcmStandard)
        return false;

    // SSND チャンク解析
    fs.seekg(ssndOffset);
    uint8_t ssndHeader[8];
    if (!fs.read((char*)ssndHeader, 8)) return false;

    uint32_t offset = readBigEndian32(ssndHeader);
    if (offset > ssndSize - 8) return false;

    const int bytesPerSample = sampleSize / 8;
    if (bytesPerSample < 2 || bytesPerSample > 4 || (sampleSize % 8) != 0) return false; // 安全ガード

    const uint32_t bytesPerFrame = (uint32_t)numChannels * (uint32_t)bytesPerSample;
    const uint32_t availAudioBytes = ssndSize - 8u - offset;
    const uint32_t maxChunkFrames = availAudioBytes / juce::jmax(1u, bytesPerFrame);
    const uint32_t maxSixtySecFrames = (uint32_t)(sr * 60.0) + 1u;
    numSampleFrames = juce::jmin(numSampleFrames, juce::jmin(maxChunkFrames, maxSixtySecFrames));
    if (numSampleFrames == 0) return false;

    fs.seekg(ssndOffset + 8 + offset);

    // バッファにデータを展開
    destBuffer.setSize((int)numChannels, (int)numSampleFrames);
    sampleRate = sr;

    // --- エンディアンの自動判定ロジック ---
    bool useLittleEndian = false;
    if (numSampleFrames > 10 && (sampleSize == 16 || sampleSize == 24))
    {
        std::streampos currentPos = fs.tellg();
        uint32_t testFramesCount = juce::jmin<uint32_t>(100, numSampleFrames);
        std::vector<uint8_t> testBuf((size_t)(testFramesCount * bytesPerFrame));
        fs.read((char*)testBuf.data(), (std::streamsize)testBuf.size());

        double diffBig = 0.0;
        double diffLit = 0.0;

        for (uint16_t ch = 0; ch < numChannels; ++ch)
        {
            float lastBig = 0.0f;
            float lastLit = 0.0f;
            for (uint32_t f = 0; f < testFramesCount; ++f)
            {
                const uint8_t* p = testBuf.data() + (f * bytesPerFrame) + (ch * bytesPerSample);
                
                // ビッグエンディアン仮定
                float fBig = 0.0f;
                if (sampleSize == 16) {
                    fBig = (float)(int16_t)(((uint16_t)p[0] << 8) | p[1]) / 32768.0f;
                } else {
                    int32_t val = ((int32_t)p[0] << 16) | ((int32_t)p[1] << 8) | p[2];
                    if (val & 0x800000) val |= 0xFF000000;
                    fBig = (float)val / 8388608.0f;
                }
                if (f > 0) diffBig += std::abs(fBig - lastBig);
                lastBig = fBig;

                // リトルエンディアン仮定
                float fLit = 0.0f;
                if (sampleSize == 16) {
                    fLit = (float)(int16_t)(((uint16_t)p[1] << 8) | p[0]) / 32768.0f;
                } else {
                    int32_t val = ((int32_t)p[2] << 16) | ((int32_t)p[1] << 8) | p[0];
                    if (val & 0x800000) val |= 0xFF000000;
                    fLit = (float)val / 8388608.0f;
                }
                if (f > 0) diffLit += std::abs(fLit - lastLit);
                lastLit = fLit;
            }
        }

        if (diffLit < diffBig)
            useLittleEndian = true;

        fs.clear();           // ★ 読み込みエラー状態（EOFフラグなど）をクリア
        fs.seekg(currentPos); // ファイルポインタを戻す
    }

    std::vector<uint8_t> frameBuf((size_t)bytesPerFrame);

    for (uint32_t f = 0; f < numSampleFrames; ++f)
    {
        if (!fs.read((char*)frameBuf.data(), bytesPerFrame)) break;

        for (uint16_t ch = 0; ch < numChannels; ++ch)
        {
            const uint8_t* samplePtr = frameBuf.data() + (ch * bytesPerSample);
            float outSample = 0.0f;

            if (sampleSize == 16)
            {
                int16_t val = 0;
                if (useLittleEndian)
                    val = (int16_t)(((uint16_t)samplePtr[1] << 8) | samplePtr[0]);
                else
                    val = (int16_t)(((uint16_t)samplePtr[0] << 8) | samplePtr[1]);
                outSample = (float)val / 32768.0f;
            }
            else if (sampleSize == 24)
            {
                int32_t val = 0;
                if (useLittleEndian)
                    val = ((int32_t)samplePtr[2] << 16) | ((int32_t)samplePtr[1] << 8) | samplePtr[0];
                else
                    val = ((int32_t)samplePtr[0] << 16) | ((int32_t)samplePtr[1] << 8) | samplePtr[2];
                if (val & 0x800000) val |= 0xFF000000;
                outSample = (float)val / 8388608.0f;
            }
            else if (sampleSize == 32)
            {
                if (isFloat32)
                {
                    float rawF;
                    std::memcpy(&rawF, samplePtr, 4);
                    outSample = swapFloat(rawF);
                }
                else
                {
                    // 32-bit 整数の場合
                    int32_t val = 0;
                    if (useLittleEndian)
                        val = ((int32_t)samplePtr[3] << 24) | ((int32_t)samplePtr[2] << 16) | ((int32_t)samplePtr[1] << 8) | samplePtr[0];
                    else
                        val = ((int32_t)samplePtr[0] << 24) | ((int32_t)samplePtr[1] << 16) | ((int32_t)samplePtr[2] << 8) | samplePtr[3];
                    outSample = (float)val / 2147483648.0f;
                }
            }

            destBuffer.setSample((int)ch, (int)f, outSample);
        }
    }

    // サニティチェック (NaN/Inf または異常過大振幅の検出)
    for (int ch = 0; ch < destBuffer.getNumChannels(); ++ch)
    {
        auto range = destBuffer.findMinMax(ch, 0, destBuffer.getNumSamples());
        if (!isFiniteFloat(range.getStart()) || !isFiniteFloat(range.getEnd())
            || range.getStart() < -4.0f || range.getEnd() > 4.0f)
        {
            destBuffer.clear();
            return false;
        }
    }

    return true;
}

// ------------------------------------------
// パラメータレイアウト
// ------------------------------------------
juce::AudioProcessorValueTreeState::ParameterLayout GranularAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- エンベロープ (PadEnvelope: Attack最大10秒/最短0.5ms) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack",  "Attack",  juce::NormalisableRange<float>(0.0005f, 10.0f, 0.0001f, 0.22f), 0.02f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay",   "Decay",   juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release", juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.5f));

    // --- グレイン ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("grainSize",     "Grain Size", 0.02f, 0.5f, 0.15f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("density",       "Density",    0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("position",      "Position",   0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("positionSpray", "Spray",      0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pitchJitter",   "Pitch Jitter", 0.0f, 2400.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("panSpread",     "Pan Spread", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("speed",         "Speed",      0.0f, 2.0f, 1.0f));

    // --- プレイヘッドの動き (MOTION) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>("playMode", "Motion",
        juce::StringArray { "Forward", "Backward", "Ping-Pong", "Random" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("speedSync", "Speed Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("speedSyncDiv", "Scan Length",
        juce::StringArray { "1 Beat", "2 Beats", "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterBool> ("reverseGrain",  "Rev Grain",  false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("windowShape",  "Window Shape",
        juce::StringArray { "Hanning", "Triangle", "Square", "Asymmetric" }, 0));

    // --- ピッチ ---
    params.push_back(std::make_unique<juce::AudioParameterInt>  ("octave",   "Octave",    -3, 3, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>  ("pitchSt",  "Semi (st)", -24, 24, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fineTune", "Fine Tune", -100.0f, 100.0f, 0.0f));

    // --- Key / Scale 量子化 ---
    params.push_back(std::make_unique<juce::AudioParameterBool>("scaleOn", "Scale Quantize", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("key", "Key",
        juce::StringArray { "Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", ScaleQuantizer::getAllScaleNames(), 0));

    // --- 音程感強調 ---
    params.push_back(std::make_unique<juce::AudioParameterBool>("pitchSync", "Pitch Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("spraySpeed", "Spray Speed",
        juce::NormalisableRange<float>(0.01f, 1.0f, 0.0f, 0.5f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Tone", 0.0f, 1.0f, 0.0f));

    // --- 素材解析モード ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>("materialMode", "Material",
        juce::StringArray { "Auto (Detect)", "Percussive", "Melodic", "Vocal" }, 0));

    // --- サンプル範囲 (自動トリム領域内の相対 0..1) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sampleStart", "Start", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sampleEnd",   "End",   0.0f, 1.0f, 1.0f));

    // --- リミッター天井 (Mainタブ) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ceiling", "Ceiling",
        juce::NormalisableRange<float>(-6.0f, 0.0f, 0.01f), -0.1f));
    // --- リミッター・リリース (Configタブ) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("limRelease", "Limiter Release",
        juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f, 0.4f), 50.0f));

    // --- フィルタースロープ / 同時発音数 (Configタブ) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filterSlope", "Filter Slope",
        juce::StringArray { "12 dB/oct", "24 dB/oct" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("poly", "Polyphony", 1, 24, 24));

    // --- FX Dry/Wet (Masterエリア, MOD対象) 1.0=full wet ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fxMix", "FX Dry/Wet", 0.0f, 1.0f, 1.0f));

    // --- カラーテーマ (Config, 表示のみ) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>("colorTheme", "Color Theme",
        GranularColors::getThemeNames(), 0));

    // --- FXチェーン (5スロット、順序はD&Dで入替=値のスワップ) ---
    for (int i = 1; i <= FxChain::kNumSlots; ++i)
    {
        const juce::String idx(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>("fx" + idx + "Type", "FX " + idx + " Type",
            FxChain::getTypeNames(), 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("fx" + idx + "Amount", "FX " + idx + " Amount",
            0.0f, 100.0f, 0.0f));
    }

    // --- FX詳細 (スロット選択時に表示されるパラメータ群) ---
    // Saturation
    params.push_back(std::make_unique<juce::AudioParameterChoice>("satAlgo", "Sat Algo",
        FxChain::getSatAlgoNames(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("satDrive", "Sat Drive",
        juce::NormalisableRange<float>(1.0f, 12.0f, 0.01f, 0.5f), 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("satPreHz", "Sat Pre-HPF",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("satTrim", "Sat Trim",
        -12.0f, 12.0f, 0.0f));
    // Chorus
    params.push_back(std::make_unique<juce::AudioParameterFloat>("choRate", "Chorus Rate",
        juce::NormalisableRange<float>(0.05f, 4.0f, 0.01f, 0.5f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("choDepth", "Chorus Depth", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("choWidth", "Chorus Width", 0.0f, 1.0f, 1.0f));
    // Delay
    params.push_back(std::make_unique<juce::AudioParameterChoice>("dlyTime", "Delay Time",
        FxChain::getDelayTimeNames(), 4)); // 1/8.
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dlyFeedback", "Delay Feedback", 0.0f, 0.95f, 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dlyDuck", "Delay Duck", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dlyDamp", "Delay Damp", 0.0f, 1.0f, 0.3f));
    // Freeze
    params.push_back(std::make_unique<juce::AudioParameterFloat>("frzSize", "Freeze Size",
        juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.4f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("frzFeedback", "Freeze Feedback", 0.0f, 0.99f, 0.9f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("frzDamp", "Freeze Damp", 0.0f, 1.0f, 0.2f));
    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>("revDecay",   "Reverb Decay",   0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("revShimmer", "Reverb Shimmer", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("revDamp",    "Reverb Damp",    0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("revMod",     "Reverb Mod",     0.0f, 1.0f, 0.4f));

    // Gate (SPECTRA8 完全一致)
    params.push_back(std::make_unique<juce::AudioParameterChoice>("gateRate", "Gate Rate",
        gfx::FormantGate::getRateNames(), 4)); // 1/16
    params.push_back(std::make_unique<juce::AudioParameterChoice>("gatePattern", "Gate Pattern",
        gfx::FormantGate::getPatternNames(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gateDepth", "Gate Depth",
        0.0f, 100.0f, 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gateDecay", "Gate Decay",
        10.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gateVowel", "Gate Vowel",
        0.0f, 100.0f, 50.0f));

    // --- MODマトリクス: LFO×4 ---
    for (int i = 1; i <= ModMatrix::kNumLfos; ++i)
    {
        const juce::String idx(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo" + idx + "Rate", "LFO " + idx + " Rate",
            juce::NormalisableRange<float>(0.02f, 20.0f, 0.01f, 0.4f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("lfo" + idx + "Sync", "LFO " + idx + " Sync", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + idx + "SyncRate", "LFO " + idx + " Sync Rate",
            ModMatrix::getSyncRateNames(), 6));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo" + idx + "Wave", "LFO " + idx + " Wave",
            ModMatrix::getWaveNames(), 0));
    }

    // --- MODマトリクス: ENV×2 (ADSR / Loop時はA-D循環) ---
    for (int i = 1; i <= ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Attack", "ENV " + idx + " Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.05f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Decay", "ENV " + idx + " Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Sustain", "ENV " + idx + " Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Release", "ENV " + idx + " Release",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("env" + idx + "Loop", "ENV " + idx + " Loop", false));
    }

    // --- MODマトリクス: 16スロット ---
    for (int i = 1; i <= ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i);
        params.push_back(std::make_unique<juce::AudioParameterChoice>("mod" + idx + "Src", "Mod " + idx + " Source",
            ModMatrix::getSourceNames(), 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>("mod" + idx + "Dst", "Mod " + idx + " Dest",
            ModMatrix::getDestNames(), 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("mod" + idx + "Amt", "Mod " + idx + " Amount",
            -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("mod" + idx + "Uni", "Mod " + idx + " Unipolar", false));
    }

    // --- アルペジエイター ---
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpEnable", "Arp On", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpPattern", "Arp Pattern",
        Arpeggiator::getPatternNames(), 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpOctaves", "Arp Octaves", 1, 4, 1));
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpSync", "Arp Sync", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpRateSync", "Arp Rate",
        Arpeggiator::getRateNames(), 6)); // 1/8
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arpRateFree", "Arp Rate (Hz)",
        juce::NormalisableRange<float>(0.5f, 30.0f, 0.01f, 0.5f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpDurMode", "Arp Duration",
        Arpeggiator::getDurationNames(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("arpGate", "Arp Gate",
        0.05f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpLatch", "Arp Latch", false));

    // --- マスター ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterHPF", "HPF (Hz)", juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterLPF", "LPF (Hz)", juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outGain",   "Out Gain", -36.0f, 12.0f, 0.0f));

    return { params.begin(), params.end() };
}

// ------------------------------------------
// コンストラクタほか
// ------------------------------------------
void GranularAudioProcessor::initParamCache()
{
    auto getPtr = [this](const char* id)
    {
        auto* p = apvts.getRawParameterValue(id);
        jassert(p != nullptr);
        return p;
    };

    cp.attack  = getPtr("attack");
    cp.decay   = getPtr("decay");
    cp.sustain = getPtr("sustain");
    cp.release = getPtr("release");

    cp.grainSize     = getPtr("grainSize");
    cp.density       = getPtr("density");
    cp.position      = getPtr("position");
    cp.positionSpray = getPtr("positionSpray");
    cp.pitchJitter   = getPtr("pitchJitter");
    cp.panSpread     = getPtr("panSpread");
    cp.speed         = getPtr("speed");

    cp.playMode     = getPtr("playMode");
    cp.speedSync    = getPtr("speedSync");
    cp.speedSyncDiv = getPtr("speedSyncDiv");
    cp.reverseGrain = getPtr("reverseGrain");
    cp.windowShape  = getPtr("windowShape");

    cp.octave   = getPtr("octave");
    cp.pitchSt  = getPtr("pitchSt");
    cp.fineTune = getPtr("fineTune");

    cp.scaleOn = getPtr("scaleOn");
    cp.key     = getPtr("key");
    cp.scale   = getPtr("scale");

    cp.pitchSync  = getPtr("pitchSync");
    cp.spraySpeed = getPtr("spraySpeed");
    cp.tone       = getPtr("tone");

    cp.materialMode = getPtr("materialMode");
    cp.sampleStart  = getPtr("sampleStart");
    cp.sampleEnd    = getPtr("sampleEnd");

    cp.ceiling     = getPtr("ceiling");
    cp.limRelease  = getPtr("limRelease");
    cp.filterSlope = getPtr("filterSlope");
    cp.poly        = getPtr("poly");
    cp.fxMix       = getPtr("fxMix");
    cp.colorTheme  = getPtr("colorTheme");

    cp.masterHPF = getPtr("masterHPF");
    cp.masterLPF = getPtr("masterLPF");
    cp.outGain   = getPtr("outGain");

    for (int i = 0; i < FxChain::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);
        cp.fxType[(size_t)i]   = apvts.getRawParameterValue("fx" + idx + "Type");
        cp.fxAmount[(size_t)i] = apvts.getRawParameterValue("fx" + idx + "Amount");
        jassert(cp.fxType[(size_t)i] != nullptr && cp.fxAmount[(size_t)i] != nullptr);
    }

    cp.satAlgo  = getPtr("satAlgo");
    cp.satDrive = getPtr("satDrive");
    cp.satPreHz = getPtr("satPreHz");
    cp.satTrim  = getPtr("satTrim");

    cp.choRate  = getPtr("choRate");
    cp.choDepth = getPtr("choDepth");
    cp.choWidth = getPtr("choWidth");

    cp.dlyTime     = getPtr("dlyTime");
    cp.dlyFeedback = getPtr("dlyFeedback");
    cp.dlyDuck     = getPtr("dlyDuck");
    cp.dlyDamp     = getPtr("dlyDamp");

    cp.frzSize     = getPtr("frzSize");
    cp.frzFeedback = getPtr("frzFeedback");
    cp.frzDamp     = getPtr("frzDamp");

    cp.revDecay   = getPtr("revDecay");
    cp.revShimmer = getPtr("revShimmer");
    cp.revDamp    = getPtr("revDamp");
    cp.revMod     = getPtr("revMod");

    cp.gateRate    = getPtr("gateRate");
    cp.gatePattern = getPtr("gatePattern");
    cp.gateDepth   = getPtr("gateDepth");
    cp.gateDecay   = getPtr("gateDecay");
    cp.gateVowel   = getPtr("gateVowel");

    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const juce::String idx(i + 1);
        cp.lfoRate[(size_t)i]     = apvts.getRawParameterValue("lfo" + idx + "Rate");
        cp.lfoSync[(size_t)i]     = apvts.getRawParameterValue("lfo" + idx + "Sync");
        cp.lfoSyncRate[(size_t)i] = apvts.getRawParameterValue("lfo" + idx + "SyncRate");
        cp.lfoWave[(size_t)i]     = apvts.getRawParameterValue("lfo" + idx + "Wave");
    }

    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i + 1);
        cp.envAttack[(size_t)i]  = apvts.getRawParameterValue("env" + idx + "Attack");
        cp.envDecay[(size_t)i]   = apvts.getRawParameterValue("env" + idx + "Decay");
        cp.envSustain[(size_t)i] = apvts.getRawParameterValue("env" + idx + "Sustain");
        cp.envRelease[(size_t)i] = apvts.getRawParameterValue("env" + idx + "Release");
        cp.envLoop[(size_t)i]    = apvts.getRawParameterValue("env" + idx + "Loop");
    }

    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);
        cp.modSrc[(size_t)i] = apvts.getRawParameterValue("mod" + idx + "Src");
        cp.modDst[(size_t)i] = apvts.getRawParameterValue("mod" + idx + "Dst");
        cp.modAmt[(size_t)i] = apvts.getRawParameterValue("mod" + idx + "Amt");
        cp.modUni[(size_t)i] = apvts.getRawParameterValue("mod" + idx + "Uni");
    }

    cp.arpEnable   = getPtr("arpEnable");
    cp.arpPattern  = getPtr("arpPattern");
    cp.arpOctaves  = getPtr("arpOctaves");
    cp.arpSync     = getPtr("arpSync");
    cp.arpRateSync = getPtr("arpRateSync");
    cp.arpRateFree = getPtr("arpRateFree");
    cp.arpDurMode  = getPtr("arpDurMode");
    cp.arpGate     = getPtr("arpGate");
    cp.arpLatch    = getPtr("arpLatch");
}

void GranularAudioProcessor::resetPhysicalToSnapped() noexcept
{
    for (auto& chArr : physicalToSnapped)
        chArr.fill(-1);
    pendingSnappedCount = 0;
}

juce::File GranularAudioProcessor::getFileToIngestCopy() const
{
    const juce::ScopedLock sl(stateLock);
    return fileToIngest;
}

void GranularAudioProcessor::reclaimRetiredSamplesLocked()
{
    const int inFlight = audioCallbacksInFlight.load(std::memory_order_seq_cst);
    const uint64_t curEpoch = audioCompletedEpoch.load(std::memory_order_seq_cst);
    retiredSamples.erase(
        std::remove_if(retiredSamples.begin(), retiredSamples.end(),
                       [inFlight, curEpoch](const RetiredSample& r)
                       {
                           return inFlight == 0 || curEpoch >= r.minSafeEpoch;
                       }),
        retiredSamples.end());
}

void GranularAudioProcessor::retireCurrentSample()
{
    const juce::ScopedLock sl(sampleOwnerLock);
    publishedSample.store(nullptr, std::memory_order_seq_cst);
    if (currentOwnedSample)
    {
        const int inFlight = audioCallbacksInFlight.load(std::memory_order_seq_cst);
        const uint64_t curEpoch = audioCompletedEpoch.load(std::memory_order_seq_cst);
        if (inFlight == 0)
            currentOwnedSample.reset();
        else
            retiredSamples.push_back({ std::move(currentOwnedSample), curEpoch + (uint64_t)inFlight });
    }
    reclaimRetiredSamplesLocked();
}

void GranularAudioProcessor::publishIngestedSample(std::shared_ptr<const IngestedSample> next)
{
    const juce::ScopedLock sl(sampleOwnerLock);
    publishedSample.store(next.get(), std::memory_order_seq_cst);
    if (currentOwnedSample)
    {
        const int inFlight = audioCallbacksInFlight.load(std::memory_order_seq_cst);
        const uint64_t curEpoch = audioCompletedEpoch.load(std::memory_order_seq_cst);
        if (inFlight == 0)
            currentOwnedSample.reset();
        else
            retiredSamples.push_back({ std::move(currentOwnedSample), curEpoch + (uint64_t)inFlight });
    }
    currentOwnedSample = std::move(next);
    reclaimRetiredSamplesLocked();
}

GranularAudioProcessor::GranularAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread("SampleIngestThread"),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    initParamCache();
    formatManager.registerBasicFormats();
    waveformPeaks.fill(0.0f);
    waveformBright.fill(0.0f);
    resetPhysicalToSnapped();

    // 保存済みカラーテーマをUI構築前に適用（ノブ等のアクセント色を正しくキャッシュ）
    GranularColors::setTheme((int)cp.colorTheme->load());
}

GranularAudioProcessor::~GranularAudioProcessor()
{
    stopThread(4000);
    const juce::ScopedLock sl(sampleOwnerLock);
    publishedSample.store(nullptr, std::memory_order_seq_cst);
    currentOwnedSample.reset();
    retiredSamples.clear();
}

const juce::String GranularAudioProcessor::getName() const { return juce::String(JucePlugin_Name); }
bool GranularAudioProcessor::acceptsMidi() const { return true; }
bool GranularAudioProcessor::producesMidi() const { return false; }
bool GranularAudioProcessor::isMidiEffect() const { return false; }

double GranularAudioProcessor::getTailLengthSeconds() const
{
    double tail = (double)cp.release->load();
    const float fxMix = cp.fxMix->load();
    if (fxMix > 0.001f)
    {
        for (int i = 0; i < FxChain::kNumSlots; ++i)
        {
            const int t = (int)cp.fxType[(size_t)i]->load();
            const float amt = cp.fxAmount[(size_t)i]->load();
            if (amt <= 0.01f) continue;
            if (t == FxChain::Reverb)
                tail = juce::jmax(tail, 1.5 + 6.5 * (double)cp.revDecay->load());
            else if (t == FxChain::Delay)
                tail = juce::jmax(tail, 1.0 + 4.0 * (double)cp.dlyFeedback->load());
            else if (t == FxChain::Freeze)
                tail = juce::jmax(tail, 1.0 + 5.0 * (double)cp.frzFeedback->load());
        }
    }
    return tail;
}

int GranularAudioProcessor::getNumPrograms() { return 1; }
int GranularAudioProcessor::getCurrentProgram() { return 0; }
void GranularAudioProcessor::setCurrentProgram(int) {}
const juce::String GranularAudioProcessor::getProgramName(int) { return {}; }
void GranularAudioProcessor::changeProgramName(int, const juce::String&) {}

void GranularAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    synth.prepareToPlay(sampleRate, samplesPerBlock);
    arp.prepare(sampleRate);
    fxChain.prepare(sampleRate);
    masterLimiter.prepare(sampleRate);
    modMatrix.prepare(sampleRate);
    fxDryBuffer.setSize(2, samplesPerBlock, false, false, true);
    quantizedMidi.ensureSize(2048);

    // ルックアヘッド等の遅延は無いためレイテンシ0を明示的にDAWへ報告
    setLatencySamples(0);

    // DAWのSRが変わった場合は新SRへ自動再インジェスト（最高音質へ復帰）
    // 完了までの間は processBlock 側の srComp 補正がピッチを守る
    if (ingestState.load() == IngestState::Ready
        && std::abs(bufferSampleRate.load() - sampleRate) > 0.5
        && hasSampleFile()
        && !isThreadRunning())
    {
        reingestCurrentFile();
    }
}

void GranularAudioProcessor::releaseResources() { synth.releaseResources(); }

bool GranularAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// ------------------------------------------
// processBlock
// ------------------------------------------
void GranularAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int numSm = buffer.getNumSamples();
    for (int i = 0; i < numCh; ++i)
        buffer.clear(i, 0, numSm);

    if (numCh <= 0 || numSm <= 0)
        return;

    struct AudioFlightGuard
    {
        std::atomic<int>& inFlight;
        std::atomic<uint64_t>& epoch;
        explicit AudioFlightGuard(std::atomic<int>& f, std::atomic<uint64_t>& e) noexcept
            : inFlight(f), epoch(e)
        {
            inFlight.fetch_add(1, std::memory_order_seq_cst);
        }
        ~AudioFlightGuard() noexcept
        {
            epoch.fetch_add(1, std::memory_order_seq_cst);
            inFlight.fetch_sub(1, std::memory_order_seq_cst);
        }
    } flightGuard(audioCallbacksInFlight, audioCompletedEpoch);

    // 非同期 PANIC 要求をオーディオブロック冒頭でゼロアロケーション実行
    if (panicRequested.exchange(false, std::memory_order_acquire))
    {
        midiMessages.clear();
        resetPhysicalToSnapped();
        synth.reset();
        fxChain.reset();
        arp.reset();
        modMatrix.reset();
        masterLimiter.reset();
        dryPreviewActive.store(false, std::memory_order_release);
        dryPreviewResetPos.store(false, std::memory_order_release);
        dryPreviewPos = -1.0;
        prevArpEnabled = false;
        return;
    }

    // --- ホストBPM / 再生情報取得 ---
    double bpm = 120.0;
    double ppqPos = 0.0;
    bool isPlaying = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (pos->getBpm()) bpm = *pos->getBpm();
            if (pos->getPpqPosition()) ppqPos = *pos->getPpqPosition();
            isPlaying = pos->getIsPlaying();
        }
    }

    const IngestedSample* activeSample = publishedSample.load(std::memory_order_seq_cst);
    const int detRoot = activeSample != nullptr ? activeSample->detectedRoot : currentDetectedRoot.load();

    const bool scaleOn = cp.scaleOn->load() > 0.5f;
    const int scaleIdx = (int)cp.scale->load();
    const int keyIdx   = (int)cp.key->load();
    const int rootNote = (keyIdx == 0) ? detRoot : (keyIdx - 1);

    // --- アルペジエイター (MIDIを置き換え) ---
    ArpParams arpP;
    arpP.enable       = cp.arpEnable->load() > 0.5f;
    arpP.pattern      = (int)cp.arpPattern->load();
    arpP.octaves      = (int)cp.arpOctaves->load();
    arpP.sync         = cp.arpSync->load() > 0.5f;
    arpP.rateSync     = (int)cp.arpRateSync->load();
    arpP.rateFree     = cp.arpRateFree->load();
    arpP.durMode      = (int)cp.arpDurMode->load();
    arpP.gate         = cp.arpGate->load();
    arpP.latch        = cp.arpLatch->load() > 0.5f;
    arpP.scaleEnable  = scaleOn;
    arpP.key          = keyIdx;
    arpP.scale        = scaleIdx;
    arpP.detectedRoot = detRoot;
    arpP.bpm          = bpm;

    // Arp系への変調（modMatrixは1ブロック前の値。アルペジオはmodより前段のため）
    {
        arpP.octaves = juce::jlimit(1, 4,
            arpP.octaves + (int)std::round(modMatrix.get(ModMatrix::DstArpOct) * 3.0f));
        arpP.gate = juce::jlimit(0.05f, 1.0f,
            arpP.gate + modMatrix.get(ModMatrix::DstArpGate));
        const float rateMod = modMatrix.get(ModMatrix::DstArpRateFree);
        if (arpP.sync)
            arpP.rateSync = juce::jlimit(0, 15, arpP.rateSync + (int)std::round(rateMod * 7.5f));
        else
            arpP.rateFree = juce::jlimit(0.5f, 30.0f, arpP.rateFree * std::pow(2.0f, rateMod * 2.0f));
    }

    if (arpP.enable)
    {
        quantizedMidi.clear();
        if (pendingSnappedCount > 0)
        {
            for (int ch = 0; ch < 16; ++ch)
            {
                for (int n = 0; n < 128; ++n)
                {
                    const int snapped = (int)physicalToSnapped[(size_t)ch][(size_t)n];
                    if (snapped >= 0)
                        quantizedMidi.addEvent(juce::MidiMessage::noteOff(ch + 1, snapped, (uint8_t)0), 0);
                }
            }
            resetPhysicalToSnapped();
        }
        arp.process(midiMessages, numSm, arpP);
        if (!quantizedMidi.isEmpty())
            midiMessages.addEvents(quantizedMidi, 0, numSm, 0);
    }
    else
    {
        if (scaleOn || pendingSnappedCount > 0)
        {
            // ARP Off の通常演奏時でも、入力NoteOn/NoteOffをScaleにスナップ
            // physicalToSnapped で NoteOn 時のスナップ先を記憶し、scaleOn切替後もNoteOff 時に同一ノートへ送る
            quantizedMidi.clear();
            for (const auto metadata : midiMessages)
            {
                const auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                {
                    const int ch = juce::jlimit(0, 15, msg.getChannel() - 1);
                    const int origNote = juce::jlimit(0, 127, msg.getNoteNumber());
                    if (physicalToSnapped[(size_t)ch][(size_t)origNote] >= 0)
                    {
                        quantizedMidi.addEvent(
                            juce::MidiMessage::noteOff(msg.getChannel(), (int)physicalToSnapped[(size_t)ch][(size_t)origNote], (uint8_t)0),
                            metadata.samplePosition);
                        physicalToSnapped[(size_t)ch][(size_t)origNote] = -1;
                        --pendingSnappedCount;
                    }
                    if (scaleOn)
                    {
                        const int snapped = juce::jlimit(0, 127, juce::roundToInt(ScaleQuantizer::quantize((float)origNote, rootNote, scaleIdx)));
                        physicalToSnapped[(size_t)ch][(size_t)origNote] = (int8_t)snapped;
                        ++pendingSnappedCount;
                        quantizedMidi.addEvent(juce::MidiMessage::noteOn(msg.getChannel(), snapped, msg.getVelocity()), metadata.samplePosition);
                    }
                    else
                    {
                        quantizedMidi.addEvent(msg, metadata.samplePosition);
                    }
                }
                else if (msg.isNoteOff())
                {
                    const int ch = juce::jlimit(0, 15, msg.getChannel() - 1);
                    const int origNote = juce::jlimit(0, 127, msg.getNoteNumber());
                    int snapped = (int)physicalToSnapped[(size_t)ch][(size_t)origNote];
                    if (snapped >= 0)
                    {
                        physicalToSnapped[(size_t)ch][(size_t)origNote] = -1;
                        --pendingSnappedCount;
                        quantizedMidi.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), snapped, msg.getVelocity()), metadata.samplePosition);
                    }
                    else if (scaleOn)
                    {
                        snapped = juce::jlimit(0, 127, juce::roundToInt(ScaleQuantizer::quantize((float)origNote, rootNote, scaleIdx)));
                        quantizedMidi.addEvent(juce::MidiMessage::noteOff(msg.getChannel(), snapped, msg.getVelocity()), metadata.samplePosition);
                    }
                    else
                    {
                        quantizedMidi.addEvent(msg, metadata.samplePosition);
                    }
                }
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                {
                    resetPhysicalToSnapped();
                    quantizedMidi.addEvent(msg, metadata.samplePosition);
                }
                else
                {
                    quantizedMidi.addEvent(msg, metadata.samplePosition);
                }
            }
            midiMessages.swapWith(quantizedMidi);
        }

        // ArpがON→OFFへ遷移した場合、ここでArp内部の残存ノートに対するNoteOffが放出される
        arp.process(midiMessages, numSm, arpP);
    }
    prevArpEnabled = arpP.enable;

    // --- MODマトリクス (アルペジオ後のMIDIでトリガー) ---
    ModMatrix::Params mp;
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        mp.lfo[(size_t)i].rateHz   = juce::jlimit(0.02f, 20.0f, cp.lfoRate[(size_t)i]->load() + modMatrix.get(ModMatrix::DstLfo1Rate + i) * 10.0f);
        mp.lfo[(size_t)i].sync     = cp.lfoSync[(size_t)i]->load() > 0.5f;
        mp.lfo[(size_t)i].rateSync = (int)cp.lfoSyncRate[(size_t)i]->load();
        mp.lfo[(size_t)i].wave     = (int)cp.lfoWave[(size_t)i]->load();
    }
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        mp.env[(size_t)i].attack  = juce::jlimit(0.001f, 10.0f, cp.envAttack[(size_t)i]->load()  + modMatrix.get(ModMatrix::DstEnv1Attack  + i * 4) * 2.5f);
        mp.env[(size_t)i].decay   = juce::jlimit(0.001f, 10.0f, cp.envDecay[(size_t)i]->load()   + modMatrix.get(ModMatrix::DstEnv1Decay   + i * 4) * 2.5f);
        mp.env[(size_t)i].sustain = juce::jlimit(0.0f, 1.0f,    cp.envSustain[(size_t)i]->load() + modMatrix.get(ModMatrix::DstEnv1Sustain + i * 4));
        mp.env[(size_t)i].release = juce::jlimit(0.001f, 10.0f, cp.envRelease[(size_t)i]->load() + modMatrix.get(ModMatrix::DstEnv1Release + i * 4) * 2.5f);
        mp.env[(size_t)i].loop    = cp.envLoop[(size_t)i]->load() > 0.5f;
    }
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        mp.slot[(size_t)i].src = (int)cp.modSrc[(size_t)i]->load();
        mp.slot[(size_t)i].dst = (int)cp.modDst[(size_t)i]->load();
        mp.slot[(size_t)i].amt = cp.modAmt[(size_t)i]->load();
        mp.slot[(size_t)i].uni = cp.modUni[(size_t)i]->load() > 0.5f;
    }
    mp.bpm = bpm;

    modMatrix.handleMidi(midiMessages);
    modMatrix.processBlock(numSm, mp);

    if (ingestState.load(std::memory_order_acquire) != IngestState::Ready
        || activeSample == nullptr
        || activeSample->validSamples <= 0)
    {
        synth.allNotesOff(); // サンプル未ロード/削除時は発音を止める
        return;
    }

    const double sr = currentSampleRate.load(std::memory_order_relaxed);
    GranularParams p;
    p.attack        = cp.attack->load();
    p.decay         = cp.decay->load();
    p.sustain       = cp.sustain->load();
    p.release       = cp.release->load();
    p.grainSize     = cp.grainSize->load();
    p.density       = cp.density->load();
    p.position      = cp.position->load();
    p.positionSpray = cp.positionSpray->load();
    p.pitchJitter   = cp.pitchJitter->load();
    p.panSpread     = cp.panSpread->load();
    p.speed         = cp.speed->load();
    p.octave        = (int)cp.octave->load();
    p.pitchSt       = (int)cp.pitchSt->load();
    p.fineTune      = cp.fineTune->load();
    p.outGainDb     = cp.outGain->load();
    p.reverseGrain  = cp.reverseGrain->load() > 0.5f;
    p.windowShape   = (int)cp.windowShape->load();
    p.key           = keyIdx;
    p.scale         = scaleIdx;
    p.scaleOn       = scaleOn;
    p.pitchSync     = cp.pitchSync->load() > 0.5f;
    p.spraySpeed    = cp.spraySpeed->load();
    p.tone          = cp.tone->load();
    p.masterHPF     = cp.masterHPF->load();
    p.masterLPF     = cp.masterLPF->load();
    p.filterSlope   = (int)cp.filterSlope->load();
    p.ceiling       = cp.ceiling->load();
    p.limRelease    = cp.limRelease->load();
    p.poly          = (int)cp.poly->load();
    p.playMode      = (int)cp.playMode->load();
    p.speedSync     = cp.speedSync->load() > 0.5f;
    p.speedSyncDiv  = (int)cp.speedSyncDiv->load();
    p.bpm           = bpm;
    p.detectedRoot  = activeSample->detectedRoot;
    p.rootNote      = activeSample->rootMidiNote;
    p.srComp        = (float)(activeSample->sampleRate / (sr > 1.0 ? sr : 44100.0)); // SR補償

    // --- MOD変調の適用 (Pitch系は量子化前に加算されるためスケールに乗る) ---
    {
        using D = ModMatrix;
        auto mod = [this](int dst) { return modMatrix.get(dst); };

        p.position      = juce::jlimit(0.0f, 1.0f,      p.position      + mod(D::DstPosition));
        p.grainSize     = juce::jlimit(0.02f, 0.5f,     p.grainSize     + mod(D::DstSize) * 0.48f);
        p.density       = juce::jlimit(0.0f, 1.0f,      p.density       + mod(D::DstDensity));
        p.positionSpray = juce::jlimit(0.0f, 1.0f,      p.positionSpray + mod(D::DstSpray));
        p.spraySpeed    = juce::jlimit(0.01f, 1.0f,     p.spraySpeed    + mod(D::DstSpraySpeed) * 0.99f);
        p.pitchJitter   = juce::jlimit(0.0f, 2400.0f,   p.pitchJitter   + mod(D::DstJitter) * 2400.0f);
        p.panSpread     = juce::jlimit(0.0f, 1.0f,      p.panSpread     + mod(D::DstPanSpread));
        p.speed         = juce::jlimit(0.0f, 2.0f,      p.speed         + mod(D::DstSpeed) * 2.0f);
        p.pitchMod      = juce::jlimit(-48.0f, 48.0f,   mod(D::DstOctave) * 24.0f + mod(D::DstSemi) * 12.0f);
        p.fineTune      = juce::jlimit(-100.0f, 100.0f, p.fineTune      + mod(D::DstFine) * 100.0f);
        p.tone          = juce::jlimit(0.0f, 1.0f,      p.tone          + mod(D::DstTone));

        // フィルターは指数(オクターブ)スケールで音楽的に
        p.masterHPF = juce::jlimit(20.0f, 2000.0f,   p.masterHPF * std::pow(2.0f, mod(D::DstMasterHpf) * 5.0f));
        p.masterLPF = juce::jlimit(200.0f, 20000.0f, p.masterLPF * std::pow(2.0f, mod(D::DstMasterLpf) * 6.0f));

        // マスター
        p.outGainDb  = juce::jlimit(-36.0f, 12.0f, p.outGainDb  + mod(D::DstOutGain) * 24.0f);
        p.ceiling    = juce::jlimit(-6.0f, 0.0f,   p.ceiling    + mod(D::DstLimCeiling) * 6.0f);
        p.limRelease = juce::jlimit(1.0f, 500.0f,  p.limRelease + mod(D::DstLimRelease) * 250.0f);
    }

    // ユーザーStart/End: 自動トリム領域内の相対指定 → 実効トリムを算出
    const int totalSamples = activeSample->validSamples;
    const int autoS = juce::jlimit(0, juce::jmax(0, totalSamples - 1), activeSample->trimStart);
    const int autoE = juce::jlimit(autoS + 1, totalSamples, juce::jmax(autoS + 1, activeSample->trimEnd));
    const int span  = juce::jmax(0, autoE - autoS);
    const float s01 = juce::jlimit(0.0f, 1.0f, cp.sampleStart->load() + modMatrix.get(ModMatrix::DstSampleStart));
    const float e01 = juce::jlimit(0.0f, 1.0f, cp.sampleEnd->load()   + modMatrix.get(ModMatrix::DstSampleEnd));

    int effS = autoS + (int)(s01 * (float)span);
    int effE = autoS + (int)(e01 * (float)span);
    const int minSpan = juce::jmin(1000, span);
    if (effE - effS < minSpan)
    {
        effE = juce::jmin(autoE, effS + minSpan);
        effS = juce::jmax(autoS, effE - minSpan);
    }

    synth.processBlock(buffer, midiMessages, &activeSample->buffer, effS, effE, p);

    // --- FX前の安全ソフトクリップ ---
    {
        auto softClipHi = [](float x) noexcept
        {
            const float t = 0.9f;
            const float a = std::abs(x);
            if (a <= t) return x;
            const float s = x < 0.0f ? -1.0f : 1.0f;
            return s * (t + (1.0f - t) * std::tanh((a - t) / (1.0f - t)));
        };
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer(ch);
            for (int i = 0; i < numSm; ++i)
                d[i] = softClipHi(d[i]);
        }
    }

    // --- FXチェーン (シンセ後段) ---
    FxChain::Params fxP;
    for (int i = 0; i < FxChain::kNumSlots; ++i)
    {
        fxP.type[(size_t)i]   = (int)cp.fxType[(size_t)i]->load();
        fxP.amount[(size_t)i] = juce::jlimit(0.0f, 1.0f,
            cp.fxAmount[(size_t)i]->load() / 100.0f
            + modMatrix.get(ModMatrix::DstFx1Amount + i));
    }
    fxP.bpm = bpm;
    fxP.satAlgo     = (int)cp.satAlgo->load();
    fxP.satDrive    = juce::jlimit(1.0f, 12.0f,     cp.satDrive->load() + modMatrix.get(ModMatrix::DstSatDrive) * 5.5f);
    fxP.satPreHz    = juce::jlimit(20.0f, 2000.0f,  cp.satPreHz->load() * std::pow(2.0f, modMatrix.get(ModMatrix::DstSatPreHpf) * 5.0f));
    fxP.satTrimDb   = juce::jlimit(-12.0f, 12.0f,   cp.satTrim->load()  + modMatrix.get(ModMatrix::DstSatTrim) * 12.0f);

    fxP.choRate     = juce::jlimit(0.05f, 4.0f,     cp.choRate->load()  + modMatrix.get(ModMatrix::DstChoRate) * 2.0f);
    fxP.choDepth    = juce::jlimit(0.0f, 1.0f,      cp.choDepth->load() + modMatrix.get(ModMatrix::DstChoDepth));
    fxP.choWidth    = juce::jlimit(0.0f, 1.0f,      cp.choWidth->load() + modMatrix.get(ModMatrix::DstChoWidth));

    fxP.dlyTime     = (int)cp.dlyTime->load();
    fxP.dlyFeedback = juce::jlimit(0.0f, 0.95f,     cp.dlyFeedback->load() + modMatrix.get(ModMatrix::DstDlyFeedback));
    fxP.dlyDuck     = juce::jlimit(0.0f, 1.0f,      cp.dlyDuck->load()     + modMatrix.get(ModMatrix::DstDlyDuck));
    fxP.dlyDamp     = juce::jlimit(0.0f, 1.0f,      cp.dlyDamp->load()     + modMatrix.get(ModMatrix::DstDlyDamp));

    fxP.frzSize     = juce::jlimit(20.0f, 1000.0f,  cp.frzSize->load()     + modMatrix.get(ModMatrix::DstFrzSize) * 500.0f);
    fxP.frzFeedback = juce::jlimit(0.0f, 0.99f,     cp.frzFeedback->load() + modMatrix.get(ModMatrix::DstFrzFeedback));
    fxP.frzDamp     = juce::jlimit(0.0f, 1.0f,      cp.frzDamp->load()     + modMatrix.get(ModMatrix::DstFrzDamp));

    fxP.revDecay    = juce::jlimit(0.0f, 1.0f,      cp.revDecay->load()   + modMatrix.get(ModMatrix::DstRevDecay));
    fxP.revShimmer  = juce::jlimit(0.0f, 1.0f,      cp.revShimmer->load() + modMatrix.get(ModMatrix::DstRevShimmer));
    fxP.revDamp     = juce::jlimit(0.0f, 1.0f,      cp.revDamp->load()    + modMatrix.get(ModMatrix::DstRevDamp));
    fxP.revMod      = juce::jlimit(0.0f, 1.0f,      cp.revMod->load()     + modMatrix.get(ModMatrix::DstRevMod));

    fxP.ppqPosition = ppqPos;
    fxP.isPlaying   = isPlaying;
    fxP.gateRate    = (int)cp.gateRate->load();
    fxP.gatePattern = (int)cp.gatePattern->load();
    fxP.gateDepth   = juce::jlimit(0.0f, 100.0f,    cp.gateDepth->load() + modMatrix.get(ModMatrix::DstGateDepth) * 100.0f);
    fxP.gateDecay   = juce::jlimit(10.0f, 100.0f,   cp.gateDecay->load() + modMatrix.get(ModMatrix::DstGateDecay) * 100.0f);
    fxP.gateVowel   = juce::jlimit(0.0f, 100.0f,    cp.gateVowel->load() + modMatrix.get(ModMatrix::DstGateVowel) * 100.0f);

    // --- FX Dry/Wet (MOD対象): プリFXを退避 → FX後にブレンド ---
    const float fxMix = juce::jlimit(0.0f, 1.0f, cp.fxMix->load() + modMatrix.get(ModMatrix::DstFxMix));

    if (fxMix < 0.999f)
    {
        if (fxDryBuffer.getNumChannels() < numCh || fxDryBuffer.getNumSamples() < numSm)
            fxDryBuffer.setSize(juce::jmax(2, numCh), numSm, false, false, true);
        for (int ch = 0; ch < numCh; ++ch)
            fxDryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSm);
    }

    fxChain.process(buffer, fxP);

    if (fxMix < 0.999f)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            buffer.applyGain(ch, 0, numSm, fxMix);
            buffer.addFrom(ch, 0, fxDryBuffer, ch, 0, numSm, 1.0f - fxMix);
        }
    }

    // --- Dryサンプル・プレビュー (原音をそのまま出力に加算) ---
    if (dryPreviewActive.load(std::memory_order_acquire) && totalSamples > 0)
    {
        const int srcCh = activeSample->buffer.getNumChannels();
        int startS = autoS;
        int endS   = autoE;
        if (endS <= startS || endS > totalSamples) { startS = 0; endS = totalSamples; }
        if (dryPreviewResetPos.exchange(false, std::memory_order_acquire) || dryPreviewPos < 0.0)
            dryPreviewPos = (double)startS;

        for (int i = 0; i < numSm; ++i)
        {
            const int sp = (int)dryPreviewPos;
            if (sp >= endS)
            {
                dryPreviewActive.store(false, std::memory_order_release);
                dryPreviewPos = -1.0;
                break;
            }
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float s = activeSample->buffer.getSample(juce::jmin(ch, srcCh - 1), sp);
                buffer.addSample(ch, i, s);
            }
            dryPreviewPos += 1.0;
        }
    }

    // --- マスターブリックウォールリミッター + NaN/Inf/バーストセーフティガード ---
    {
        masterLimiter.setReleaseMs(p.limRelease);
        const float ceilingLin = juce::Decibels::decibelsToGain(
            juce::jlimit(-6.0f, 0.0f, p.ceiling));
        float* dL = buffer.getWritePointer(0);
        if (numCh > 1)
        {
            float* dR = buffer.getWritePointer(1);
            for (int i = 0; i < numSm; ++i)
                masterLimiter.process(dL[i], dR[i], ceilingLin);
        }
        else
        {
            for (int i = 0; i < numSm; ++i)
                masterLimiter.processMono(dL[i], ceilingLin);
        }

        bool hasAnomaly = false;
        for (int ch = 0; ch < numCh && !hasAnomaly; ++ch)
        {
            const float* d = buffer.getReadPointer(ch);
            for (int i = 0; i < numSm; ++i)
            {
                const float v = d[i];
                if (!isFiniteFloat(v) || std::abs(v) > 2.0f)
                {
                    hasAnomaly = true;
                    break;
                }
            }
        }

        if (hasAnomaly)
        {
            buffer.clear();
            fxChain.reset();
            synth.reset();
            masterLimiter.reset();
        }
    }
}

bool GranularAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* GranularAudioProcessor::createEditor() { return new GranularAudioProcessorEditor(*this); }

// ------------------------------------------
// 状態保存 / 復元
// ------------------------------------------
void GranularAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    juce::ScopedLock sl(stateLock);
    xml->setAttribute("AudioFilePath", fileToIngest.getFullPathName());
    xml->setAttribute("editorWidth", savedEditorWidth);
    xml->setAttribute("editorHeight", savedEditorHeight);
    copyXmlToBinary(*xml, destData);
}

void GranularAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        savedEditorWidth  = xmlState->getIntAttribute("editorWidth", 1080);
        savedEditorHeight = xmlState->getIntAttribute("editorHeight", 640);

        juce::String savedFilePath;
        {
            juce::ScopedLock sl(stateLock);
            savedFilePath = xmlState->getStringAttribute("AudioFilePath", "");
        }
        if (savedFilePath.isNotEmpty())
        {
            juce::File savedFile(savedFilePath);
            if (savedFile.existsAsFile()) loadFileForIngest(savedFile);
        }
    }
}

// ------------------------------------------
// サンプル読み込み（背景スレッド）
// ------------------------------------------
void GranularAudioProcessor::loadFileForIngest(const juce::File& file)
{
    if (isThreadRunning()) stopThread(2000);
    // 再読込/再解析時、音声スレッドがサンプルを読み続ける競合をイミュータブル退避で防止
    ingestState.store(IngestState::Loading);
    waveformReady.store(false);
    dryPreviewActive.store(false);
    retireCurrentSample();
    {
        juce::ScopedLock sl(stateLock);
        fileToIngest = file;
        hudLineFile = "File: " + file.getFileName();
        hudLineOrig = "Original: ---";
        hudLineCorr = "Corrected: ---";
    }
    startThread();
}

void GranularAudioProcessor::reingestCurrentFile()
{
    const juce::File file = getFileToIngestCopy();
    if (file.existsAsFile())
        loadFileForIngest(file);
}

void GranularAudioProcessor::clearSample()
{
    if (isThreadRunning()) stopThread(2000);
    ingestState.store(IngestState::Empty);   // processBlock が即座に発音を止める
    waveformReady.store(false);
    dryPreviewActive.store(false);
    panicRequested.store(true, std::memory_order_release);
    retireCurrentSample();
    validSampleCount.store(0);
    trimStartSample.store(0);
    trimEndSample.store(0);
    rootMidiNote.store(60);

    {
        juce::ScopedLock sl(stateLock);
        fileToIngest = juce::File();
        waveformPeaks.fill(0.0f);
        waveformBright.fill(0.0f);
        hudLineFile = "File: ---";
        hudLineTask = "Task: Drop a sample";
        hudLineOrig = "Original: ---";
        hudLineCorr = "Corrected: ---";
    }
    sendChangeMessage();
}

void GranularAudioProcessor::run()
{
    ingestState.store(IngestState::Loading);
    ingestProgress.store(0.02f);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Loading..."; }

    // このインジェストが対象とするSRとファイルをスレッド開始時にスナップショット確定
    const double targetSR = currentSampleRate.load(std::memory_order_relaxed);
    const juce::File targetFile = getFileToIngestCopy();
    const int materialMode = (int)cp.materialMode->load();

    // DAWのファイルロック解除待ちを含むリトライ読み込み
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> tempRaw;
    double fileSampleRate = 44100.0;
    bool loadedViaFallback = false;

    for (int i = 0; i < 200 && !threadShouldExit(); ++i)
    {
        if (targetFile.existsAsFile() && targetFile.getSize() > 0)
        {
            // 1. Ableton fl32 AIFF-C フォールバック読み込み
            if (readAbletonAif(targetFile, tempRaw, fileSampleRate))
            {
                loadedViaFallback = true;
                break;
            }

            // 2. 通常のJUCEデコーダー
            auto stream = targetFile.createInputStream();
            if (stream != nullptr && !stream->isExhausted())
            {
                reader.reset(formatManager.createReaderFor(std::move(stream)));
            }

            // 3. ファイルロック回避フォールバック (std::ifstream で共有オープンし、メモリバッファ経由でデコード)
            if (reader == nullptr)
            {
                std::ifstream fLockFree(targetFile.getFullPathName().toWideCharPointer(), std::ios::binary | std::ios::ate);
                if (fLockFree.is_open())
                {
                    std::streamsize size = fLockFree.tellg();
                    if (size > 0)
                    {
                        fLockFree.seekg(0, std::ios::beg);
                        juce::MemoryBlock memBlock((size_t)size);
                        if (fLockFree.read((char*)memBlock.getData(), size))
                        {
                            auto memStream = std::make_unique<juce::MemoryInputStream>(memBlock, false);
                            reader.reset(formatManager.createReaderFor(std::move(memStream)));
                        }
                    }
                }
            }

            if (reader != nullptr && reader->lengthInSamples > 0)
                break;
        }
        juce::Thread::sleep(100);
    }

    if (threadShouldExit()) return;

    if (!loadedViaFallback && (!reader || reader->lengthInSamples == 0))
    {
        ingestState.store(IngestState::Failed);
        { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Failed (Timeout or Empty)"; }
        return;
    }

    ingestProgress.store(0.10f);

    // 読み込み（最大60秒に制限）
    if (!loadedViaFallback)
    {
        const double fileSR = reader->sampleRate;
        const int maxSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, (juce::int64)(fileSR * 60.0));
        tempRaw.setSize((int)reader->numChannels, maxSamples);
        reader->read(&tempRaw, 0, maxSamples, 0, true, true);
        fileSampleRate = fileSR;
    }
    else
    {
        const int maxSamples = (int)juce::jmin<juce::int64>(tempRaw.getNumSamples(), (juce::int64)(fileSampleRate * 60.0));
        if (tempRaw.getNumSamples() > maxSamples)
        {
            juce::AudioBuffer<float> truncated(tempRaw.getNumChannels(), maxSamples);
            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch)
                truncated.copyFrom(ch, 0, tempRaw, ch, 0, maxSamples);
            tempRaw = truncated;
        }
    }

    if (threadShouldExit() || tempRaw.getNumSamples() <= 0 || tempRaw.getNumChannels() <= 0) return;

    // ホストSRへリサンプル
    if (fileSampleRate != targetSR && fileSampleRate > 0.0)
    {
        const double ratio = fileSampleRate / targetSR;
        const int newLength = juce::jmax(1, (int)((double)tempRaw.getNumSamples() / ratio));
        juce::AudioBuffer<float> resampled(tempRaw.getNumChannels(), newLength);
        for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch)
        {
            if (threadShouldExit()) return;
            juce::LagrangeInterpolator interpolator;
            interpolator.process(ratio, tempRaw.getReadPointer(ch), resampled.getWritePointer(ch), newLength);
        }
        tempRaw = resampled;
    }
    ingestProgress.store(0.25f);
    if (threadShouldExit()) return;

    // --- ピッチ解析 (Omni-Analyzer: 全チャンネル位相整合モノミックス) ---
    ingestState.store(IngestState::Analyzing);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Analyzing Pitch..."; }

    {
        const int numRawCh = tempRaw.getNumChannels();
        const int numRawSamples = tempRaw.getNumSamples();
        if (numRawCh <= 1)
        {
            currentPitch = PitchAnalyzer::analyzeOmni(tempRaw.getReadPointer(0), numRawSamples,
                                                      targetSR, targetFile.getFileName(), materialMode);
        }
        else
        {
            std::vector<float> monoMix((size_t)numRawSamples, 0.0f);
            const float* ch0 = tempRaw.getReadPointer(0);
            std::vector<float> chSigns((size_t)numRawCh, 1.0f);

            // Lchと逆相のチャンネルがあれば位相反転して加算キャンセルを防止
            double e0 = 0.0;
            for (int i = 0; i < numRawSamples; ++i)
                e0 += (double)ch0[i] * (double)ch0[i];

            for (int ch = 1; ch < numRawCh; ++ch)
            {
                const float* ck = tempRaw.getReadPointer(ch);
                double dot = 0.0, ek = 0.0;
                for (int i = 0; i < numRawSamples; ++i)
                {
                    dot += (double)ch0[i] * (double)ck[i];
                    ek  += (double)ck[i]  * (double)ck[i];
                }
                const double denom = std::sqrt(e0 * ek);
                if (denom > 1.0e-12 && dot < -0.5 * denom)
                    chSigns[(size_t)ch] = -1.0f;
            }

            const float invCh = 1.0f / (float)numRawCh;
            for (int ch = 0; ch < numRawCh; ++ch)
            {
                const float* ck = tempRaw.getReadPointer(ch);
                const float s = invCh * chSigns[(size_t)ch];
                for (int i = 0; i < numRawSamples; ++i)
                    monoMix[(size_t)i] += ck[i] * s;
            }

            currentPitch = PitchAnalyzer::analyzeOmni(monoMix.data(), numRawSamples,
                                                      targetSR, targetFile.getFileName(), materialMode);
        }
    }
    currentDetectedRoot.store(currentPitch.detectedRoot);
    currentIsMajor.store(currentPitch.isMajor);
    ingestProgress.store(0.45f);
    if (threadShouldExit()) return;

    {
        juce::ScopedLock sl(stateLock);
        if (currentPitch.isDetected)
        {
            const juce::String sign = currentPitch.centsOffset >= 0 ? "+" : "";
            hudLineOrig = "Original: " + currentPitch.noteName + " | "
                        + juce::String(currentPitch.frequency, 2) + " Hz | "
                        + sign + juce::String(juce::roundToInt(currentPitch.centsOffset)) + " Cents";
        }
        else
        {
            hudLineOrig = "Original: Unpitched / Pure Noise";
        }
    }

    // --- Root Note Tracking: セント偏差のみ補正（音質ほぼ無劣化） ---
    //     検出ノートをルートとして記憶。鍵盤でそのノートを弾くと原音の高さで再生される。
    ingestState.store(IngestState::Correcting);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Cents Correction (Signalsmith)..."; }

    juce::AudioBuffer<float> corrected;

    int foldedRoot = 60;
    if (currentPitch.isDetected && currentPitch.frequency > 5.0)
    {
        // Register Fold: ルートの音名(ピッチクラス)は保ったまま、オクターブを
        // 鍵盤中央(C3=60)に最も近い位置へ再配置する。
        //  例: C7素材 → ルートをC3にマッピング。C3で原音、C4で原音の1oct上。
        //      C1素材 → 同じくC3付近に畳み込まれる。
        // サンプル自体は無変換なので音色は原音(C7/C1)のまま。
        const int pc = ((currentPitch.midiNote % 12) + 12) % 12;
        foldedRoot = pc + 12 * juce::roundToInt((60.0f - (float)pc) / 12.0f);
        foldedRoot = juce::jlimit(0, 127, foldedRoot);
        rootMidiNote.store(foldedRoot);

        const float cents = currentPitch.centsOffset;
        if (std::abs(cents) > 1.0f)
        {
            // ジャストピッチとのズレ(セント)だけを補正 → シフト量が極小で高音質
            const float pitchScale = std::pow(2.0f, -cents / 1200.0f);
            const int numCh = tempRaw.getNumChannels();
            const int inLen = tempRaw.getNumSamples();

            signalsmith::stretch::SignalsmithStretch<float> stretch;
            stretch.presetDefault(numCh, (float)targetSR);
            stretch.setTransposeFactor(pitchScale, 8000.0f / (float)targetSR);

            // 総レイテンシ分を末尾にパディングして1:1処理→先頭を捨てて位置合わせ
            const int totalLat = stretch.inputLatency() + stretch.outputLatency();
            const int padLen = inLen + totalLat;

            juce::AudioBuffer<float> padded(numCh, padLen);
            padded.clear();
            for (int ch = 0; ch < numCh; ++ch)
                padded.copyFrom(ch, 0, tempRaw, ch, 0, inLen);

            juce::AudioBuffer<float> stretched(numCh, padLen);
            stretched.clear();
            stretch.process(padded.getArrayOfReadPointers(), padLen,
                            stretched.getArrayOfWritePointers(), padLen);

            corrected.setSize(numCh, inLen);
            for (int ch = 0; ch < numCh; ++ch)
                corrected.copyFrom(ch, 0, stretched, ch, totalLat, inLen);
        }
        else
        {
            corrected.makeCopyOf(tempRaw); // ほぼジャスト: 補正不要
        }

        juce::ScopedLock sl(stateLock);
        const juce::String sign = cents >= 0 ? "-" : "+";
        hudLineCorr = "Corrected: Root " + currentPitch.noteName
                    + " -> Key " + PitchAnalyzer::midiNoteName(foldedRoot) + " | "
                    + sign + juce::String(std::abs(juce::roundToInt(cents))) + "c | "
                    + currentPitch.scaleName;
    }
    else
    {
        rootMidiNote.store(60); // 未検出: C3をルートに
        corrected.makeCopyOf(tempRaw);
        juce::ScopedLock sl(stateLock);
        hudLineCorr = "Corrected: N/A (Root C3)";
    }
    if (threadShouldExit()) return;
    ingestProgress.store(0.75f);

    // --- ステレオ化 → 正規化 → トリム → 波形ピーク (イミュータブル構築後に一括公開) ---
    ingestState.store(IngestState::Normalizing);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Normalizing..."; }

    const int len = corrected.getNumSamples();
    if (len <= 0)
    {
        ingestState.store(IngestState::Failed);
        { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Failed (Empty Audio)"; }
        return;
    }

    auto nextSample = std::make_shared<IngestedSample>();
    nextSample->buffer.setSize(2, len, false, true, true);
    nextSample->buffer.copyFrom(0, 0, corrected, 0, 0, len);
    nextSample->buffer.copyFrom(1, 0, corrected, corrected.getNumChannels() > 1 ? 1 : 0, 0, len);

    const float rawPeak = nextSample->buffer.getMagnitude(0, len);
    int s = 0, e = len - 1;
    if (rawPeak > 1.0e-7f)
    {
        const float thresh = juce::jmax(1.0e-5f, rawPeak * 0.002f); // ピーク相対 -54dB フロア
        const float* d0 = nextSample->buffer.getReadPointer(0);
        const float* d1 = nextSample->buffer.getReadPointer(1);
        for (int i = 0; i < len; ++i)
        {
            if (std::max(std::abs(d0[i]), std::abs(d1[i])) > thresh) { s = i; break; }
        }
        for (int i = len - 1; i >= s; --i)
        {
            if (std::max(std::abs(d0[i]), std::abs(d1[i])) > thresh) { e = i; break; }
        }
    }

    const float mag = nextSample->buffer.getMagnitude(s, e - s + 1);
    if (mag > 1.0e-7f) nextSample->buffer.applyGain(0, len, 0.5f / mag); // -6dB 正規化 (全体連続ゲイン)

    nextSample->validSamples = len;
    nextSample->trimStart    = s;
    nextSample->trimEnd      = e;
    nextSample->sampleRate   = targetSR;
    nextSample->detectedRoot = currentPitch.detectedRoot;
    nextSample->isMajor      = currentPitch.isMajor;
    nextSample->rootMidiNote = foldedRoot;

    std::array<float, 1000> peaksLocal {};
    std::array<float, 1000> brightLocal {};
    const float* wp0 = nextSample->buffer.getReadPointer(0);
    const float* wp1 = nextSample->buffer.getReadPointer(1);
    for (int i = 0; i < 1000; ++i)
    {
        if (threadShouldExit()) return;
        int startIdx = (int)((double)i * (double)len / 1000.0);
        int endIdx   = (int)((double)(i + 1) * (double)len / 1000.0);
        startIdx = juce::jlimit(0, len - 1, startIdx);
        endIdx   = juce::jlimit(startIdx + 1, len, endIdx);
        const int seg = endIdx - startIdx;

        peaksLocal[(size_t)i] = nextSample->buffer.getMagnitude(startIdx, seg);

        // ステレオエネルギー和ベースの一次差分スペクトル明度 (マイクロ素材でも全1000ビン補間)
        double totalE = 1.0e-9, highE = 0.0;
        for (int n = startIdx; n < endIdx; ++n)
        {
            const float x0 = wp0[n];
            const float x1 = wp1[n];
            const float p0 = (n > 0) ? wp0[n - 1] : x0;
            const float p1 = (n > 0) ? wp1[n - 1] : x1;
            const float dx0 = x0 - p0;
            const float dx1 = x1 - p1;
            totalE += 0.5 * ((double)x0 * (double)x0 + (double)x1 * (double)x1);
            highE  += 0.5 * ((double)dx0 * (double)dx0 + (double)dx1 * (double)dx1);
        }
        const float ratio = (float)std::sqrt(highE / totalE); // 0..~2
        brightLocal[(size_t)i] = juce::jlimit(0.0f, 1.0f, ratio / 1.6f);
    }

    if (threadShouldExit()) return;

    {
        juce::ScopedLock sl(stateLock);
        waveformPeaks = peaksLocal;
        waveformBright = brightLocal;
        hudLineTask = "Task: Ready";
    }
    validSampleCount.store(len);
    trimStartSample.store(s);
    trimEndSample.store(e);
    bufferSampleRate.store(targetSR); // このサンプルのSRを記録 (srComp補正の基準)
    waveformReady.store(true);

    // 完全初期化済みサンプルをオーディオスレッドへ公開（リリースフェンス付き）
    publishIngestedSample(std::move(nextSample));

    ingestProgress.store(1.0f);
    ingestState.store(IngestState::Ready);

    sendChangeMessage();
}

// ==========================================
// プリセット
// ==========================================
void GranularAudioProcessor::panic()
{
    dryPreviewActive.store(false, std::memory_order_release);
    panicRequested.store(true, std::memory_order_release);
}

void GranularAudioProcessor::resetAllParametersToDefault()
{
    // colorTheme / poly はUI・システム設定なのでプリセット/INITで変えない
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            const auto id = rp->getParameterID();
            if (id == "colorTheme" || id == "poly") continue;
            rp->setValueNotifyingHost(rp->getDefaultValue());
        }
}

void GranularAudioProcessor::applyFactoryPreset(int index)
{
    panic(); // プリセット切替時に古い発音や残響を確実に消音
    const auto& list = GranularPresets::all();
    if (index < 0 || index >= (int)list.size()) return;

    resetAllParametersToDefault();     // 未指定パラメータはデフォルトへ（決定論的）

    const auto& preset = list[(size_t)index];
    for (const auto& pp : preset.params)
    {
        if (auto* rp = apvts.getParameter(pp.id))
        {
            const float norm = juce::jlimit(0.0f, 1.0f, rp->convertTo0to1(pp.v));
            rp->setValueNotifyingHost(norm);
        }
    }
    currentPresetName = preset.name;
}

void GranularAudioProcessor::randomizePreset()
{
    resetAllParametersToDefault();
    juce::Random r;
    auto setV = [&](juce::String id, float v)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, p->convertTo0to1(v)));
    };
    auto rf = [&](float a, float b) { return a + (b - a) * r.nextFloat(); };
    auto ri = [&](int a, int b) { return a + r.nextInt(b - a + 1); };

    // --- 音作りの基本 ---
    setV("grainSize", rf(0.03f, 0.45f));
    setV("density", rf(0.3f, 0.95f));
    setV("positionSpray", rf(0.0f, 0.7f));
    setV("pitchJitter", rf(0.0f, 150.0f));
    setV("panSpread", rf(0.3f, 1.0f));
    setV("speed", rf(0.1f, 1.5f));
    setV("playMode", (float)ri(0, 3));
    setV("windowShape", (float)ri(0, 3));
    setV("octave", (float)ri(-2, 1));
    setV("tone", rf(0.0f, 0.6f));
    setV("pitchSync", r.nextBool() ? 1.0f : 0.0f);
    setV("masterHPF", rf(20.0f, 400.0f));
    setV("masterLPF", rf(3000.0f, 20000.0f));
    setV("attack", rf(0.002f, 2.0f));
    setV("decay", rf(0.05f, 1.5f));
    setV("sustain", rf(0.3f, 1.0f));
    setV("release", rf(0.1f, 2.5f));
    setV("fxMix", rf(0.6f, 1.0f));
    setV("outGain", 0.0f);                    // 安全のため0dB固定
    const bool scOn = r.nextFloat() < 0.6f;
    setV("scaleOn", scOn ? 1.0f : 0.0f);
    if (scOn)
    {
        setV("key", (float)ri(0, 12));
        setV("scale", (float)ri(0, ScaleQuantizer::numScales() - 1));
    }

    // --- LFO/ENV に動きを仕込む ---
    for (int i = 1; i <= 4; ++i)
    {
        const juce::String x(i);
        setV("lfo" + x + "Rate", rf(0.1f, 8.0f));
        setV("lfo" + x + "Wave", (float)ri(0, 5));
        setV("lfo" + x + "Sync", r.nextBool() ? 1.0f : 0.0f);
        setV("lfo" + x + "SyncRate", (float)ri(0, 15));
    }
    for (int i = 1; i <= 3; ++i)
    {
        const juce::String x(i);
        setV("env" + x + "Attack", rf(0.01f, 3.0f));
        setV("env" + x + "Decay", rf(0.05f, 3.0f));
        setV("env" + x + "Sustain", rf(0.0f, 1.0f));
        setV("env" + x + "Release", rf(0.05f, 2.0f));
        setV("env" + x + "Loop", r.nextBool() ? 1.0f : 0.0f);
    }

    // --- MODマトリクス: ランダムな本数(3〜10)を有効化 ---
    for (int i = 1; i <= 16; ++i)
    {
        const juce::String x(i);
        setV("mod" + x + "Src", 0.0f); setV("mod" + x + "Dst", 0.0f);
        setV("mod" + x + "Amt", 0.0f); setV("mod" + x + "Uni", 0.0f);
    }
    const int k = ri(3, 10);
    for (int j = 0; j < k; ++j)
    {
        const juce::String x(1 + r.nextInt(16));
        setV("mod" + x + "Src", (float)ri(1, ModMatrix::NumSrcs - 1));
        setV("mod" + x + "Dst", (float)ri(1, ModMatrix::NumDsts - 1));
        setV("mod" + x + "Amt", rf(-1.0f, 1.0f));
        setV("mod" + x + "Uni", r.nextBool() ? 1.0f : 0.0f);
    }

    // --- FX: 1〜4スロットを重複なしでランダムに有効化 (Gate含む) ---
    for (int i = 1; i <= 5; ++i)
    {
        setV("fx" + juce::String(i) + "Type", 0.0f);
        setV("fx" + juce::String(i) + "Amount", 0.0f);
    }

    // 利用可能なFXタイプ [1..6] をシャッフル (重複なし)
    std::vector<int> availableTypes = { 1, 2, 3, 4, 5, 6 };
    for (int i = (int)availableTypes.size() - 1; i > 0; --i)
    {
        int swapIdx = r.nextInt(i + 1);
        std::swap(availableTypes[(size_t)i], availableTypes[(size_t)swapIdx]);
    }

    // スロット [1..5] をシャッフル (重複なし)
    std::vector<int> availableSlots = { 1, 2, 3, 4, 5 };
    for (int i = (int)availableSlots.size() - 1; i > 0; --i)
    {
        int swapIdx = r.nextInt(i + 1);
        std::swap(availableSlots[(size_t)i], availableSlots[(size_t)swapIdx]);
    }

    const int nfx = ri(1, 4);
    for (int j = 0; j < nfx; ++j)
    {
        const int slotIdx = availableSlots[(size_t)j];
        const int fxt     = availableTypes[(size_t)j];
        const juce::String x(slotIdx);

        setV("fx" + x + "Type", (float)fxt);
        setV("fx" + x + "Amount", rf(25.0f, 65.0f));

        if (fxt == 6)
        {
            setV("gateRate", (float)ri(1, 6));
            setV("gatePattern", (float)ri(0, gfx::FormantGate::kTotalPatterns - 1));
            setV("gateDepth", rf(35.0f, 100.0f));
            setV("gateDecay", rf(15.0f, 90.0f));
            setV("gateVowel", rf(0.0f, 100.0f));
        }
    }

    currentPresetName = "Random";
}

juce::File GranularAudioProcessor::getUserPresetDir() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("OTODESK").getChildFile("Granular").getChildFile("Presets");
    if (!dir.exists()) dir.createDirectory();
    return dir;
}

bool GranularAudioProcessor::saveUserPreset(const juce::String& name, const juce::String& category, const juce::String& sub)
{
    auto safe = juce::File::createLegalFileName(name.trim());
    if (safe.isEmpty()) return false;

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr) return false;

    // Sampleは埋め込まず「パス参照」で保存（ファイルが残っていれば読込時に再ロード）
    {
        juce::ScopedLock sl(stateLock);
        xml->setAttribute("AudioFilePath", fileToIngest.getFullPathName());
        xml->setAttribute("PresetName", name);
        xml->setAttribute("Category", category.isNotEmpty() ? category : "User");
        xml->setAttribute("Subcategory", sub);
    }
    auto file = getUserPresetDir().getChildFile(safe + ".xml");
    const bool ok = xml->writeTo(file, {});
    if (ok) currentPresetName = name;
    return ok;
}

void GranularAudioProcessor::loadUserPresetFile(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    panic(); // プリセット切替時に古い発音や残響を確実に消音
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml == nullptr) return;

    if (xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

    currentPresetName = xml->getStringAttribute("PresetName", file.getFileNameWithoutExtension());

    const juce::String path = xml->getStringAttribute("AudioFilePath", "");
    if (path.isNotEmpty())
    {
        juce::File f(path);
        if (f.existsAsFile()) loadFileForIngest(f);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularAudioProcessor(); }
