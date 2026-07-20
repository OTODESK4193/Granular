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
static bool readAbletonAif(const juce::File& file, juce::AudioBuffer<float>& destBuffer, double& sampleRate)
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
        std::streampos nextChunk = fs.tellg() + (std::streamoff)chunkSize;
        if (chunkSize % 2 != 0) nextChunk += 1; // 奇数サイズ時のアライメント

        if (std::memcmp(chunkId, "COMM", 4) == 0)
        {
            commOffset = (uint32_t)fs.tellg();
            commSize = chunkSize;
        }
        else if (std::memcmp(chunkId, "SSND", 4) == 0)
        {
            ssndOffset = (uint32_t)fs.tellg();
            ssndSize = chunkSize;
        }

        fs.seekg(nextChunk);
    }

    if (commOffset == 0 || ssndOffset == 0) return false;

    // COMM チャンク解析
    fs.seekg(commOffset);
    uint8_t commData[18];
    if (!fs.read((char*)commData, 18)) return false;

    uint16_t numChannels = readBigEndian16(commData);
    uint32_t numSampleFrames = readBigEndian32(commData + 2);
    uint16_t sampleSize = readBigEndian16(commData + 6);
    double sr = readExtendedFloat(commData + 8);

    // 圧縮タイプの判定
    char compType[4] = {0};
    fs.read(compType, 4);

    // デコード対応判定: 'able', 'fl32', 'FL32', 'NONE', 'none'
    bool isAble = (std::memcmp(compType, "able", 4) == 0);
    bool isFloat32 = (std::memcmp(compType, "fl32", 4) == 0 || std::memcmp(compType, "FL32", 4) == 0);
    bool isPcmStandard = (std::memcmp(compType, "NONE", 4) == 0 || std::memcmp(compType, "none", 4) == 0);

    // いずれの対応タイプでもない場合は、JUCEの標準デコーダーにフォールバックさせるため処理を中断
    if (!isAble && !isFloat32 && !isPcmStandard)
        return false;

    // SSND チャンク解析
    fs.seekg(ssndOffset);
    uint8_t ssndHeader[8];
    if (!fs.read((char*)ssndHeader, 8)) return false;

    uint32_t offset = readBigEndian32(ssndHeader);
    fs.seekg(ssndOffset + 8 + offset);

    // バッファにデータを展開
    destBuffer.setSize((int)numChannels, (int)numSampleFrames);
    sampleRate = sr;

    const int bytesPerSample = sampleSize / 8;
    if (bytesPerSample <= 0 || bytesPerSample > 4) return false; // 安全ガード

    const int bytesPerFrame = numChannels * bytesPerSample;

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
    params.push_back(std::make_unique<juce::AudioParameterChoice>("key", "Key",
        juce::StringArray { "Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));

    juce::StringArray scaleChoices;
    for (const auto& s : ScaleQuantizer::getScales())
        scaleChoices.add(s.name);
    params.push_back(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", scaleChoices, 0));

    // --- 音程感強調 ---
    params.push_back(std::make_unique<juce::AudioParameterBool>("pitchSync", "Pitch Sync", false));
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
GranularAudioProcessor::GranularAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread("SampleIngestThread"),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    formatManager.registerBasicFormats();
    waveformPeaks.fill(0.0f);

    // 保存済みカラーテーマをUI構築前に適用（ノブ等のアクセント色を正しくキャッシュ）
    GranularColors::setTheme((int)apvts.getRawParameterValue("colorTheme")->load());
}

GranularAudioProcessor::~GranularAudioProcessor() { stopThread(4000); }

const juce::String GranularAudioProcessor::getName() const { return juce::String(JucePlugin_Name); }
bool GranularAudioProcessor::acceptsMidi() const { return true; }
bool GranularAudioProcessor::producesMidi() const { return false; }
bool GranularAudioProcessor::isMidiEffect() const { return false; }
double GranularAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int GranularAudioProcessor::getNumPrograms() { return 1; }
int GranularAudioProcessor::getCurrentProgram() { return 0; }
void GranularAudioProcessor::setCurrentProgram(int) {}
const juce::String GranularAudioProcessor::getProgramName(int) { return {}; }
void GranularAudioProcessor::changeProgramName(int, const juce::String&) {}

void GranularAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    synth.prepareToPlay(sampleRate, samplesPerBlock);
    arp.prepare(sampleRate);
    fxChain.prepare(sampleRate);
    masterLimiter.prepare(sampleRate);
    modMatrix.prepare(sampleRate);
    fxDryBuffer.setSize(2, samplesPerBlock, false, false, true);

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
    for (int i = 0; i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (ingestState.load() != IngestState::Ready)
    {
        synth.allNotesOff(); // サンプル未ロード/削除時は発音を止める
        return;
    }

    // --- ホストBPM取得 ---
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getBpm()) bpm = *pos->getBpm();

    // --- アルペジエイター (MIDIを置き換え) ---
    ArpParams arpP;
    arpP.enable       = apvts.getRawParameterValue("arpEnable")->load() > 0.5f;
    arpP.pattern      = (int)apvts.getRawParameterValue("arpPattern")->load();
    arpP.octaves      = (int)apvts.getRawParameterValue("arpOctaves")->load();
    arpP.sync         = apvts.getRawParameterValue("arpSync")->load() > 0.5f;
    arpP.rateSync     = (int)apvts.getRawParameterValue("arpRateSync")->load();
    arpP.rateFree     = apvts.getRawParameterValue("arpRateFree")->load();
    arpP.durMode      = (int)apvts.getRawParameterValue("arpDurMode")->load();
    arpP.gate         = apvts.getRawParameterValue("arpGate")->load();
    arpP.latch        = apvts.getRawParameterValue("arpLatch")->load() > 0.5f;
    arpP.key          = (int)apvts.getRawParameterValue("key")->load();
    arpP.scale        = (int)apvts.getRawParameterValue("scale")->load();
    arpP.detectedRoot = currentDetectedRoot.load();
    arpP.bpm          = bpm;

    // Arp系への変調（modMatrixは1ブロック前の値。アルペジオはmodより前段のため）
    {
        arpP.octaves = juce::jlimit(1, 4,
            arpP.octaves + (int)std::round(modMatrix.get(ModMatrix::DstArpOct) * 3.0f));
        arpP.gate = juce::jlimit(0.05f, 1.0f,
            arpP.gate + modMatrix.get(ModMatrix::DstArpGate));
        const float rateMod = modMatrix.get(ModMatrix::DstArpRate);
        if (arpP.sync)
            arpP.rateSync = juce::jlimit(0, 12, arpP.rateSync + (int)std::round(rateMod * 6.0f));
        else
            arpP.rateFree = juce::jlimit(0.1f, 40.0f, arpP.rateFree * std::pow(2.0f, rateMod * 2.0f));
    }

    arp.process(midiMessages, buffer.getNumSamples(), arpP);

    // --- MODマトリクス (アルペジオ後のMIDIでトリガー) ---
    ModMatrix::Params mp;
    for (int i = 0; i < ModMatrix::kNumLfos; ++i)
    {
        const juce::String idx(i + 1);
        mp.lfo[(size_t)i].rateHz   = apvts.getRawParameterValue("lfo" + idx + "Rate")->load();
        mp.lfo[(size_t)i].sync     = apvts.getRawParameterValue("lfo" + idx + "Sync")->load() > 0.5f;
        mp.lfo[(size_t)i].rateSync = (int)apvts.getRawParameterValue("lfo" + idx + "SyncRate")->load();
        mp.lfo[(size_t)i].wave     = (int)apvts.getRawParameterValue("lfo" + idx + "Wave")->load();
    }
    for (int i = 0; i < ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i + 1);
        mp.env[(size_t)i].attack  = apvts.getRawParameterValue("env" + idx + "Attack")->load();
        mp.env[(size_t)i].decay   = apvts.getRawParameterValue("env" + idx + "Decay")->load();
        mp.env[(size_t)i].sustain = apvts.getRawParameterValue("env" + idx + "Sustain")->load();
        mp.env[(size_t)i].release = apvts.getRawParameterValue("env" + idx + "Release")->load();
        mp.env[(size_t)i].loop    = apvts.getRawParameterValue("env" + idx + "Loop")->load() > 0.5f;
    }
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);
        mp.slot[(size_t)i].src = (int)apvts.getRawParameterValue("mod" + idx + "Src")->load();
        mp.slot[(size_t)i].dst = (int)apvts.getRawParameterValue("mod" + idx + "Dst")->load();
        mp.slot[(size_t)i].amt = apvts.getRawParameterValue("mod" + idx + "Amt")->load();
        mp.slot[(size_t)i].uni = apvts.getRawParameterValue("mod" + idx + "Uni")->load() > 0.5f;
    }
    mp.bpm = bpm;

    modMatrix.handleMidi(midiMessages);
    modMatrix.processBlock(buffer.getNumSamples(), mp);

    GranularParams p;
    p.attack        = apvts.getRawParameterValue("attack")->load();
    p.decay         = apvts.getRawParameterValue("decay")->load();
    p.sustain       = apvts.getRawParameterValue("sustain")->load();
    p.release       = apvts.getRawParameterValue("release")->load();
    p.grainSize     = apvts.getRawParameterValue("grainSize")->load();
    p.density       = apvts.getRawParameterValue("density")->load();
    p.position      = apvts.getRawParameterValue("position")->load();
    p.positionSpray = apvts.getRawParameterValue("positionSpray")->load();
    p.pitchJitter   = apvts.getRawParameterValue("pitchJitter")->load();
    p.panSpread     = apvts.getRawParameterValue("panSpread")->load();
    p.speed         = apvts.getRawParameterValue("speed")->load();
    p.octave        = (int)apvts.getRawParameterValue("octave")->load();
    p.pitchSt       = (int)apvts.getRawParameterValue("pitchSt")->load();
    p.fineTune      = apvts.getRawParameterValue("fineTune")->load();
    p.outGainDb     = apvts.getRawParameterValue("outGain")->load();
    p.reverseGrain  = apvts.getRawParameterValue("reverseGrain")->load() > 0.5f;
    p.windowShape   = (int)apvts.getRawParameterValue("windowShape")->load();
    p.key           = (int)apvts.getRawParameterValue("key")->load();
    p.scale         = (int)apvts.getRawParameterValue("scale")->load();
    p.pitchSync     = apvts.getRawParameterValue("pitchSync")->load() > 0.5f;
    p.tone          = apvts.getRawParameterValue("tone")->load();
    p.masterHPF     = apvts.getRawParameterValue("masterHPF")->load();
    p.masterLPF     = apvts.getRawParameterValue("masterLPF")->load();
    p.filterSlope   = (int)apvts.getRawParameterValue("filterSlope")->load();
    p.ceiling       = apvts.getRawParameterValue("ceiling")->load();
    p.limRelease    = apvts.getRawParameterValue("limRelease")->load();
    p.poly          = (int)apvts.getRawParameterValue("poly")->load();
    p.playMode      = (int)apvts.getRawParameterValue("playMode")->load();
    p.speedSync     = apvts.getRawParameterValue("speedSync")->load() > 0.5f;
    p.speedSyncDiv  = (int)apvts.getRawParameterValue("speedSyncDiv")->load();
    p.bpm           = bpm;
    p.detectedRoot  = currentDetectedRoot.load();
    p.rootNote      = rootMidiNote.load();
    p.srComp        = (float)(bufferSampleRate.load() / currentSampleRate); // SR補償

    // --- MOD変調の適用 (Pitch系は量子化前に加算されるためスケールに乗る) ---
    {
        using D = ModMatrix;
        auto mod = [this](int dst) { return modMatrix.get(dst); };

        p.position      = juce::jlimit(0.0f, 1.0f,      p.position      + mod(D::DstPosition));
        p.grainSize     = juce::jlimit(0.02f, 0.5f,     p.grainSize     + mod(D::DstSize) * 0.48f);
        p.density       = juce::jlimit(0.0f, 1.0f,      p.density       + mod(D::DstDensity));
        p.positionSpray = juce::jlimit(0.0f, 1.0f,      p.positionSpray + mod(D::DstSpray));
        p.pitchJitter   = juce::jlimit(0.0f, 2400.0f,   p.pitchJitter   + mod(D::DstJitter) * 2400.0f);
        p.panSpread     = juce::jlimit(0.0f, 1.0f,      p.panSpread     + mod(D::DstPanSpread));
        p.speed         = juce::jlimit(0.0f, 2.0f,      p.speed         + mod(D::DstSpeed) * 2.0f);
        p.pitchMod      = juce::jlimit(-24.0f, 24.0f,   mod(D::DstPitch) * 24.0f);
        p.fineTune      = juce::jlimit(-100.0f, 100.0f, p.fineTune      + mod(D::DstFine) * 100.0f);
        p.tone          = juce::jlimit(0.0f, 1.0f,      p.tone          + mod(D::DstTone));

        // フィルターは指数(オクターブ)スケールで音楽的に
        p.masterHPF = juce::jlimit(20.0f, 2000.0f,   p.masterHPF * std::pow(2.0f, mod(D::DstHpf) * 5.0f));
        p.masterLPF = juce::jlimit(200.0f, 20000.0f, p.masterLPF * std::pow(2.0f, mod(D::DstLpf) * 6.0f));
    }

    // ユーザーStart/End: 自動トリム領域内の相対指定 → 実効トリムを算出
    const int autoS = trimStartSample.load();
    const int autoE = trimEndSample.load();
    const int span = juce::jmax(0, autoE - autoS);
    const float s01 = juce::jlimit(0.0f, 1.0f,
        apvts.getRawParameterValue("sampleStart")->load() + modMatrix.get(ModMatrix::DstSampleStart));
    const float e01 = juce::jlimit(0.0f, 1.0f,
        apvts.getRawParameterValue("sampleEnd")->load() + modMatrix.get(ModMatrix::DstSampleEnd));

    int effS = autoS + (int)(s01 * (float)span);
    int effE = autoS + (int)(e01 * (float)span);
    if (effE - effS < 1000) // 最低1000サンプルは確保
    {
        effE = juce::jmin(autoE, effS + 1000);
        effS = juce::jmax(autoS, effE - 1000);
    }

    synth.processBlock(buffer, midiMessages, &sharedAudioBuffer, effS, effE, p);

    // --- FX前の安全ソフトクリップ ---
    // グレインの位相整列(pitchSync×高密度)やTONEコム共鳴などで稀に生じる突発ピークが、
    // 高フィードバックのReverb/Delayを過励起して「バシャーン」(大音量の減衰ノイズ)になるのを防ぐ。
    // 通常レベル(|x|<0.9)はそのまま素通しし、0.9超のみ~1.0付近へ滑らかに制限する。
    {
        auto softClipHi = [](float x) noexcept
        {
            const float t = 0.9f;
            const float a = std::abs(x);
            if (a <= t) return x;
            const float s = x < 0.0f ? -1.0f : 1.0f;
            return s * (t + (1.0f - t) * std::tanh((a - t) / (1.0f - t)));
        };
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* d = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                d[i] = softClipHi(d[i]);
        }
    }

    // --- FXチェーン (シンセ後段) ---
    FxChain::Params fxP;
    for (int i = 0; i < FxChain::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);
        fxP.type[(size_t)i]   = (int)apvts.getRawParameterValue("fx" + idx + "Type")->load();
        fxP.amount[(size_t)i] = juce::jlimit(0.0f, 1.0f,
            apvts.getRawParameterValue("fx" + idx + "Amount")->load() / 100.0f
            + modMatrix.get(ModMatrix::DstFx1 + i)); // MOD変調
    }
    fxP.bpm = bpm;
    fxP.satAlgo     = (int)apvts.getRawParameterValue("satAlgo")->load();
    fxP.satDrive    = apvts.getRawParameterValue("satDrive")->load();
    fxP.satPreHz    = apvts.getRawParameterValue("satPreHz")->load();
    fxP.satTrimDb   = apvts.getRawParameterValue("satTrim")->load();
    fxP.choRate     = apvts.getRawParameterValue("choRate")->load();
    fxP.choDepth    = apvts.getRawParameterValue("choDepth")->load();
    fxP.choWidth    = apvts.getRawParameterValue("choWidth")->load();
    fxP.dlyTime     = (int)apvts.getRawParameterValue("dlyTime")->load();
    fxP.dlyFeedback = apvts.getRawParameterValue("dlyFeedback")->load();
    fxP.dlyDuck     = apvts.getRawParameterValue("dlyDuck")->load();
    fxP.dlyDamp     = apvts.getRawParameterValue("dlyDamp")->load();
    fxP.frzSize     = apvts.getRawParameterValue("frzSize")->load();
    fxP.frzFeedback = apvts.getRawParameterValue("frzFeedback")->load();
    fxP.frzDamp     = apvts.getRawParameterValue("frzDamp")->load();
    fxP.revDecay    = apvts.getRawParameterValue("revDecay")->load();
    fxP.revShimmer  = apvts.getRawParameterValue("revShimmer")->load();
    fxP.revDamp     = apvts.getRawParameterValue("revDamp")->load();
    fxP.revMod      = apvts.getRawParameterValue("revMod")->load();

    // --- FX Dry/Wet (MOD対象): プリFXを退避 → FX後にブレンド ---
    const int numCh = buffer.getNumChannels();
    const int numSm = buffer.getNumSamples();
    const float fxMix = juce::jlimit(0.0f, 1.0f,
        apvts.getRawParameterValue("fxMix")->load() + modMatrix.get(ModMatrix::DstDryWet));

    if (fxMix < 0.999f)
    {
        fxDryBuffer.setSize(numCh, numSm, false, false, true);
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
    if (dryPreviewActive.load() && sharedAudioBuffer.getNumSamples() > 0)
    {
        const int total = sharedAudioBuffer.getNumSamples();
        const int srcCh = sharedAudioBuffer.getNumChannels();
        int startS = trimStartSample.load();
        int endS   = trimEndSample.load();
        if (endS <= startS || endS > total) { startS = 0; endS = total; }
        if (dryPreviewPos < 0.0) dryPreviewPos = (double)startS;

        for (int i = 0; i < numSm; ++i)
        {
            const int sp = (int)dryPreviewPos;
            if (sp >= endS) { dryPreviewActive.store(false); dryPreviewPos = -1.0; break; }
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float s = sharedAudioBuffer.getSample(juce::jmin(ch, srcCh - 1), sp);
                buffer.addSample(ch, i, s);
            }
            dryPreviewPos += 1.0;
        }
    }

    // --- マスターブリックウォールリミッター + NaN/Inf/バーストセーフティガード ---
    {
        const int channels = buffer.getNumChannels();
        masterLimiter.setReleaseMs(apvts.getRawParameterValue("limRelease")->load());
        const float ceilingLin = juce::Decibels::decibelsToGain(
            juce::jlimit(-6.0f, 0.0f, apvts.getRawParameterValue("ceiling")->load()));
        float* dL = buffer.getWritePointer(0);
        float* dR = channels > 1 ? buffer.getWritePointer(1) : dL;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            masterLimiter.process(dL[i], dR[i], ceilingLin);

        bool hasAnomaly = false;
        const int numSmp = buffer.getNumSamples();
        for (int i = 0; i < numSmp; ++i)
        {
            const float cL = dL[i];
            const float cR = channels > 1 ? dR[i] : cL;
            if (std::isnan(cL) || std::isinf(cL) || std::isnan(cR) || std::isinf(cR) || std::abs(cL) > 2.0f || std::abs(cR) > 2.0f)
            {
                hasAnomaly = true;
                break;
            }
        }

        if (hasAnomaly)
        {
            buffer.clear();
            fxChain.prepare(currentSampleRate); // FX内部状態をリセット
            synth.releaseResources(); // 音源リセット
            synth.prepareToPlay(currentSampleRate, buffer.getNumSamples());
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
    copyXmlToBinary(*xml, destData);
}

void GranularAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

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
    // 再読込/再解析時、音声スレッドが sharedAudioBuffer を読みながら
    // 背景スレッドが再確保する競合を防ぐため、先に非Ready状態にする
    ingestState.store(IngestState::Loading);
    waveformReady.store(false);
    fileToIngest = file;
    {
        juce::ScopedLock sl(stateLock);
        hudLineFile = "File: " + file.getFileName();
        hudLineOrig = "Original: ---";
        hudLineCorr = "Corrected: ---";
    }
    startThread();
}

void GranularAudioProcessor::reingestCurrentFile()
{
    if (fileToIngest.existsAsFile())
        loadFileForIngest(fileToIngest);
}

void GranularAudioProcessor::clearSample()
{
    if (isThreadRunning()) stopThread(2000);
    ingestState.store(IngestState::Empty);   // processBlock が即座に発音を止める
    waveformReady.store(false);
    validSampleCount.store(0);
    trimStartSample.store(0);
    trimEndSample.store(0);
    rootMidiNote.store(60);
    fileToIngest = juce::File();
    waveformPeaks.fill(0.0f);

    juce::ScopedLock sl(stateLock);
    hudLineFile = "File: ---";
    hudLineTask = "Task: Drop a sample";
    hudLineOrig = "Original: ---";
    hudLineCorr = "Corrected: ---";
}

void GranularAudioProcessor::run()
{
    ingestState.store(IngestState::Loading);
    ingestProgress.store(0.02f);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Loading..."; }

    // このインジェストが対象とするSRをスレッド開始時に確定
    // (処理中にprepareToPlayでSRが変わっても混在しない)
    const double targetSR = currentSampleRate;

    const int materialMode = (int)apvts.getRawParameterValue("materialMode")->load();

    // DAWのファイルロック解除待ちを含むリトライ読み込み
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> tempRaw;
    double fileSampleRate = 44100.0;
    bool loadedViaFallback = false;

    for (int i = 0; i < 200 && !threadShouldExit(); ++i)
    {
        if (fileToIngest.existsAsFile() && fileToIngest.getSize() > 0)
        {
            // 1. Ableton fl32 AIFF-C フォールバック読み込み
            if (readAbletonAif(fileToIngest, tempRaw, fileSampleRate))
            {
                loadedViaFallback = true;
                break;
            }

            // 2. 通常のJUCEデコーダー
            auto stream = fileToIngest.createInputStream();
            if (stream != nullptr && !stream->isExhausted())
            {
                reader.reset(formatManager.createReaderFor(std::move(stream)));
            }

            // 3. ファイルロック回避フォールバック (std::ifstream で共有オープンし、メモリバッファ経由でデコード)
            if (reader == nullptr)
            {
                std::ifstream fLockFree(fileToIngest.getFullPathName().toWideCharPointer(), std::ios::binary | std::ios::ate);
                if (fLockFree.is_open())
                {
                    std::streamsize size = fLockFree.tellg();
                    fLockFree.seekg(0, std::ios::beg);

                    juce::MemoryBlock memBlock((size_t)size);
                    if (fLockFree.read((char*)memBlock.getData(), size))
                    {
                        auto memStream = std::make_unique<juce::MemoryInputStream>(memBlock, false);
                        reader.reset(formatManager.createReaderFor(std::move(memStream)));
                    }
                }
            }

            if (reader != nullptr && reader->lengthInSamples > 0)
                break;
        }
        juce::Thread::sleep(100);
    }

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
        // フォールバックロード時は、すでに tempRaw にデータが入り、fileSampleRate がセットされています。
        // 最大60秒の制限を適用
        const int maxSamples = (int)juce::jmin<juce::int64>(tempRaw.getNumSamples(), (juce::int64)(fileSampleRate * 60.0));
        if (tempRaw.getNumSamples() > maxSamples)
        {
            juce::AudioBuffer<float> truncated(tempRaw.getNumChannels(), maxSamples);
            for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch)
                truncated.copyFrom(ch, 0, tempRaw, ch, 0, maxSamples);
            tempRaw = truncated;
        }
    }

    // ホストSRへリサンプル
    if (fileSampleRate != targetSR && fileSampleRate > 0.0)
    {
        const double ratio = fileSampleRate / targetSR;
        const int newLength = (int)((double)tempRaw.getNumSamples() / ratio);
        juce::AudioBuffer<float> resampled(tempRaw.getNumChannels(), newLength);
        for (int ch = 0; ch < tempRaw.getNumChannels(); ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.process(ratio, tempRaw.getReadPointer(ch), resampled.getWritePointer(ch), newLength);
        }
        tempRaw = resampled;
    }
    ingestProgress.store(0.25f);

    // --- ピッチ解析 (Omni-Analyzer) ---
    ingestState.store(IngestState::Analyzing);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Analyzing Pitch..."; }

    currentPitch = PitchAnalyzer::analyzeOmni(tempRaw.getReadPointer(0), tempRaw.getNumSamples(),
                                              targetSR, fileToIngest.getFileName(), materialMode);
    currentDetectedRoot.store(currentPitch.detectedRoot);
    currentIsMajor.store(currentPitch.isMajor);
    ingestProgress.store(0.45f);

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
    ingestProgress.store(0.75f);

    // --- ステレオ化 → 正規化 → トリム → 波形ピーク ---
    ingestState.store(IngestState::Normalizing);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Normalizing..."; }

    const int len = corrected.getNumSamples();
    sharedAudioBuffer.setSize(2, len, false, true, true);
    sharedAudioBuffer.copyFrom(0, 0, corrected, 0, 0, len);
    sharedAudioBuffer.copyFrom(1, 0, corrected, corrected.getNumChannels() > 1 ? 1 : 0, 0, len);
    validSampleCount.store(len);

    int s = 0, e = len - 1;
    const float thresh = 0.005f;
    const float* d = sharedAudioBuffer.getReadPointer(0);
    for (int i = 0; i < len; ++i)          { if (std::abs(d[i]) > thresh) { s = i; break; } }
    for (int i = len - 1; i >= s; --i)     { if (std::abs(d[i]) > thresh) { e = i; break; } }
    trimStartSample.store(s);
    trimEndSample.store(e);

    const float mag = sharedAudioBuffer.getMagnitude(s, e - s + 1);
    if (mag > 0.0f) sharedAudioBuffer.applyGain(s, e - s + 1, 0.5f / mag); // -6dB 正規化

    waveformPeaks.fill(0.0f);
    waveformBright.fill(0.0f);
    const int stepSize = juce::jmax(1, len / 1000);
    const float* wp = sharedAudioBuffer.getReadPointer(0);
    for (int i = 0; i < 1000; ++i)
    {
        const int startIdx = i * stepSize;
        const int seg = juce::jmin(stepSize, len - startIdx);
        if (seg <= 0) continue;
        waveformPeaks[(size_t)i] = sharedAudioBuffer.getMagnitude(startIdx, seg);

        // スペクトル明度: 一次差分(=簡易ハイパス)の全体エネルギー比。
        // 低域主体→比が小(暖色)、高域主体→比が大(寒色)。振幅に依存しない。
        double totalE = 1.0e-9, highE = 0.0;
        for (int n = 1; n < seg; ++n)
        {
            const float x = wp[startIdx + n];
            const float dx = x - wp[startIdx + n - 1];
            totalE += (double)x * x;
            highE  += (double)dx * dx;
        }
        const float ratio = (float)std::sqrt(highE / totalE); // 0..~2
        waveformBright[(size_t)i] = juce::jlimit(0.0f, 1.0f, ratio / 1.6f);
    }
    waveformReady.store(true);

    bufferSampleRate.store(targetSR); // このバッファのSRを記録 (srComp補正の基準)
    ingestProgress.store(1.0f);
    ingestState.store(IngestState::Ready);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Ready"; }



    sendChangeMessage();
}

// ==========================================
// プリセット
// ==========================================
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
    if (r.nextFloat() < 0.5f) setV("scale", (float)ri(1, 4));

    // --- LFO/ENV に動きを仕込む ---
    for (int i = 1; i <= 4; ++i)
    {
        const juce::String x(i);
        setV("lfo" + x + "Rate", rf(0.1f, 8.0f));
        setV("lfo" + x + "Wave", (float)ri(0, 5));
        setV("lfo" + x + "Sync", r.nextBool() ? 1.0f : 0.0f);
        setV("lfo" + x + "SyncRate", (float)ri(0, 12));
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
        setV("mod" + x + "Src", (float)ri(1, 15));
        setV("mod" + x + "Dst", (float)ri(1, 17));
        setV("mod" + x + "Amt", rf(-1.0f, 1.0f));
        setV("mod" + x + "Uni", r.nextBool() ? 1.0f : 0.0f);
    }

    // --- FX: 1〜3スロットをランダムに有効化 ---
    for (int i = 1; i <= 5; ++i) setV("fx" + juce::String(i) + "Type", 0.0f);
    const int nfx = ri(1, 3);
    for (int j = 0; j < nfx; ++j)
    {
        const juce::String x(1 + r.nextInt(5));
        setV("fx" + x + "Type", (float)ri(1, 5));
        setV("fx" + x + "Amount", rf(20.0f, 60.0f));
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

bool GranularAudioProcessor::saveUserPreset(const juce::String& name)
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
    }
    auto file = getUserPresetDir().getChildFile(safe + ".xml");
    const bool ok = xml->writeTo(file, {});
    if (ok) currentPresetName = name;
    return ok;
}

void GranularAudioProcessor::loadUserPresetFile(const juce::File& file)
{
    if (!file.existsAsFile()) return;
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
