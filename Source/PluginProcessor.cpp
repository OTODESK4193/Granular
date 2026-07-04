// ==========================================
// File: PluginProcessor.cpp
// ==========================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    // --- リミッター天井 (Configタブで調整予定) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ceiling", "Ceiling",
        juce::NormalisableRange<float>(-6.0f, 0.0f, 0.01f), -0.1f));

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

    // --- MODマトリクス: ENV×2 (ループ可能AD) ---
    for (int i = 1; i <= ModMatrix::kNumEnvs; ++i)
    {
        const juce::String idx(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Attack", "ENV " + idx + " Attack",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.05f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("env" + idx + "Decay", "ENV " + idx + " Decay",
            juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.25f), 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterBool>("env" + idx + "Loop", "ENV " + idx + " Loop", false));
    }

    // --- MODマトリクス: Macro×4 ---
    for (int i = 1; i <= ModMatrix::kNumMacros; ++i)
        params.push_back(std::make_unique<juce::AudioParameterFloat>("macro" + juce::String(i),
            "Macro " + juce::String(i), 0.0f, 1.0f, 0.5f));

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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outGain",   "Out Gain", -36.0f, 12.0f, -12.0f));

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
        mp.env[(size_t)i].attack = apvts.getRawParameterValue("env" + idx + "Attack")->load();
        mp.env[(size_t)i].decay  = apvts.getRawParameterValue("env" + idx + "Decay")->load();
        mp.env[(size_t)i].loop   = apvts.getRawParameterValue("env" + idx + "Loop")->load() > 0.5f;
    }
    for (int i = 0; i < ModMatrix::kNumMacros; ++i)
        mp.macro[(size_t)i] = apvts.getRawParameterValue("macro" + juce::String(i + 1))->load();
    for (int i = 0; i < ModMatrix::kNumSlots; ++i)
    {
        const juce::String idx(i + 1);
        mp.slot[(size_t)i].src = (int)apvts.getRawParameterValue("mod" + idx + "Src")->load();
        mp.slot[(size_t)i].dst = (int)apvts.getRawParameterValue("mod" + idx + "Dst")->load();
        mp.slot[(size_t)i].amt = apvts.getRawParameterValue("mod" + idx + "Amt")->load();
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
    p.ceiling       = apvts.getRawParameterValue("ceiling")->load();
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
    const float s01 = apvts.getRawParameterValue("sampleStart")->load();
    const float e01 = apvts.getRawParameterValue("sampleEnd")->load();

    int effS = autoS + (int)(s01 * (float)span);
    int effE = autoS + (int)(e01 * (float)span);
    if (effE - effS < 1000) // 最低1000サンプルは確保
    {
        effE = juce::jmin(autoE, effS + 1000);
        effS = juce::jmax(autoS, effE - 1000);
    }

    synth.processBlock(buffer, midiMessages, &sharedAudioBuffer, effS, effE, p);

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
    fxChain.process(buffer, fxP);

    // --- マスターブリックウォールリミッター + NaN/Infガード ---
    {
        const int channels = buffer.getNumChannels();
        const float ceilingLin = juce::Decibels::decibelsToGain(
            juce::jlimit(-6.0f, 0.0f, apvts.getRawParameterValue("ceiling")->load()));
        float* dL = buffer.getWritePointer(0);
        float* dR = channels > 1 ? buffer.getWritePointer(1) : dL;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            masterLimiter.process(dL[i], dR[i], ceilingLin);

        const int lastIdx = buffer.getNumSamples() - 1;
        if (lastIdx >= 0)
        {
            const float cL = dL[lastIdx], cR = dR[lastIdx];
            if (std::isnan(cL) || std::isinf(cL) || std::isnan(cR) || std::isinf(cR))
            {
                buffer.clear();
                fxChain.prepare(currentSampleRate); // FX内部状態をリセット
            }
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
    for (int i = 0; i < 200 && !threadShouldExit(); ++i)
    {
        if (fileToIngest.existsAsFile() && fileToIngest.getSize() > 0)
        {
            auto stream = fileToIngest.createInputStream();
            if (stream != nullptr && !stream->isExhausted())
            {
                reader.reset(formatManager.createReaderFor(std::move(stream)));
                if (reader != nullptr && reader->lengthInSamples > 0) break;
            }
        }
        juce::Thread::sleep(100);
    }

    if (!reader || reader->lengthInSamples == 0)
    {
        ingestState.store(IngestState::Failed);
        { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Failed (Timeout or Empty)"; }
        return;
    }

    ingestProgress.store(0.10f);

    // 読み込み（最大60秒に制限）
    const double fileSampleRate = reader->sampleRate;
    const int maxSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, (juce::int64)(fileSampleRate * 60.0));
    juce::AudioBuffer<float> tempRaw((int)reader->numChannels, maxSamples);
    reader->read(&tempRaw, 0, maxSamples, 0, true, true);

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
    const int stepSize = juce::jmax(1, len / 1000);
    for (int i = 0; i < 1000; ++i)
    {
        const int startIdx = i * stepSize;
        const int seg = juce::jmin(stepSize, len - startIdx);
        if (seg > 0) waveformPeaks[(size_t)i] = sharedAudioBuffer.getMagnitude(startIdx, seg);
    }
    waveformReady.store(true);

    bufferSampleRate.store(targetSR); // このバッファのSRを記録 (srComp補正の基準)
    ingestProgress.store(1.0f);
    ingestState.store(IngestState::Ready);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Ready"; }
    sendChangeMessage();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularAudioProcessor(); }
