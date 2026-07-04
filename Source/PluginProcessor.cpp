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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("speed",         "Speed",      -2.0f, 2.0f, 1.0f));
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
    p.detectedRoot  = currentDetectedRoot.load();
    p.rootNote      = rootMidiNote.load();

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
    if (fileSampleRate != currentSampleRate && fileSampleRate > 0.0)
    {
        const double ratio = fileSampleRate / currentSampleRate;
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
                                              currentSampleRate, fileToIngest.getFileName(), materialMode);
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
            stretch.presetDefault(numCh, (float)currentSampleRate);
            stretch.setTransposeFactor(pitchScale, 8000.0f / (float)currentSampleRate);

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

    ingestProgress.store(1.0f);
    ingestState.store(IngestState::Ready);
    { juce::ScopedLock sl(stateLock); hudLineTask = "Task: Ready"; }
    sendChangeMessage();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GranularAudioProcessor(); }
