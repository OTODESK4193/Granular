#include <JuceHeader.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "PluginProcessor.h"
#include "Presets/PresetData.h"
#include "DSP/ScaleQuantizer.h"

namespace
{
int gFailures = 0;

void check(bool cond, const std::string& msg)
{
    if (!cond)
    {
        ++gFailures;
        std::cerr << "[FAIL] " << msg << "\n";
    }
}

void setParamReal(GranularAudioProcessor& proc, const juce::String& id, float realVal)
{
    if (auto* p = proc.apvts.getParameter(id))
        p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, p->convertTo0to1(realVal)));
}

float getParamReal(GranularAudioProcessor& proc, const juce::String& id)
{
    if (auto* v = proc.apvts.getRawParameterValue(id))
        return v->load();
    return 0.0f;
}

float computeRms(const juce::AudioBuffer<float>& buf)
{
    double sumSq = 0.0;
    const int nCh = buf.getNumChannels();
    const int nSm = buf.getNumSamples();
    if (nCh <= 0 || nSm <= 0) return 0.0f;
    for (int ch = 0; ch < nCh; ++ch)
    {
        const float* d = buf.getReadPointer(ch);
        for (int i = 0; i < nSm; ++i)
            sumSq += (double)d[i] * (double)d[i];
    }
    return (float)std::sqrt(sumSq / (double)(nCh * nSm));
}

bool createTempSineWav(const juce::File& file, double sr = 48000.0, float durSec = 1.0f, float freqHz = 261.6256f)
{
    const int totalSamples = (int)(sr * durSec);
    juce::AudioBuffer<float> buf(2, totalSamples);
    for (int i = 0; i < totalSamples; ++i)
    {
        const float v = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * freqHz * (float)i / (float)sr);
        buf.setSample(0, i, v);
        buf.setSample(1, i, v);
    }
    juce::WavAudioFormat wav;
    file.deleteFile();
    if (std::unique_ptr<juce::OutputStream> stream{ file.createOutputStream() })
    {
        const auto opts = juce::AudioFormatWriterOptions{}
            .withSampleRate(sr)
            .withNumChannels(2)
            .withBitsPerSample(16);
        if (auto writer = wav.createWriterFor(stream, opts))
            return writer->writeFromAudioSampleBuffer(buf, 0, totalSamples);
    }
    return false;
}

bool waitForSampleReady(GranularAudioProcessor& proc, int timeoutMs = 4000)
{
    const int stepMs = 10;
    for (int t = 0; t < timeoutMs; t += stepMs)
    {
        if (proc.ingestState.load(std::memory_order_acquire) == IngestState::Ready)
            return true;
        juce::Thread::sleep(stepMs);
    }
    return proc.ingestState.load(std::memory_order_acquire) == IngestState::Ready;
}

void testAllFactoryPresetsValid()
{
    std::cerr << "Running testAllFactoryPresetsValid (sizeof=" << sizeof(GranularAudioProcessor) << ")...\n";
    auto procPtr = std::make_unique<GranularAudioProcessor>();
    auto& proc = *procPtr;
    proc.prepareToPlay(48000.0, 256);

    const auto& list = GranularPresets::all();
    check(list.size() == 120, "Factory preset count should be 120");

    juce::AudioBuffer<float> buf(2, 256);
    juce::MidiBuffer midi;

    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& preset = list[i];
        int fxSeen[8] = {};
        bool hasGateFx = false;

        for (const auto& pp : preset.params)
        {
            auto* rp = proc.apvts.getParameter(pp.id);
            check(rp != nullptr, ("Preset '" + preset.name + "' references missing param: " + pp.id).toStdString());
            if (rp != nullptr)
            {
                const auto& r = rp->getNormalisableRange();
                check(pp.v >= r.start - 1.0e-3f && pp.v <= r.end + 1.0e-3f,
                    ("Preset '" + preset.name + "' param '" + pp.id + "' out of range").toStdString());
            }

            if (std::strcmp(pp.id, "fx1Type") == 0 || std::strcmp(pp.id, "fx2Type") == 0 ||
                std::strcmp(pp.id, "fx3Type") == 0 || std::strcmp(pp.id, "fx4Type") == 0 ||
                std::strcmp(pp.id, "fx5Type") == 0)
            {
                const int ft = (int)std::round(pp.v);
                if (ft > 0 && ft < 8)
                {
                    check(fxSeen[ft] == 0, ("Duplicate FX type in preset " + preset.name).toStdString());
                    fxSeen[ft] = 1;
                    if (ft == FxChain::Gate) hasGateFx = true;
                }
            }
            if (preset.name == "CD Skip" && std::strcmp(pp.id, "mod1Dst") == 0)
            {
                check((int)std::round(pp.v) == ModMatrix::DstOutGain, "CD Skip mod1Dst should be DstOutGain (20)");
            }
        }

        if (preset.name == "Gate Rider" || preset.name == "Vowel Lead" ||
            preset.name == "Elastic Formant" || preset.name == "Granular Techno Gate" ||
            preset.name == "Glitch Pulse Walker" || preset.name == "Vowel Slicer" ||
            preset.name == "Stutter Trance Gate" || preset.name == "Sync Rhythmic Gate")
        {
            check(hasGateFx, ("Gate preset '" + preset.name + "' should have FormantGate on active slot fx1..fx5").toStdString());
        }

        proc.applyFactoryPreset((int)i);
        buf.clear();
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        proc.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < 256; ++s)
                check(isFiniteFloat(buf.getSample(ch, s)), "Non-finite output after preset load");
    }
    std::cerr << "  120 presets finished\n";
}

void testScaleQuantizerUniqueMenuIds()
{
    std::cout << "Running testScaleQuantizerUniqueMenuIds...\n";
    auto menu = ScaleQuantizer::buildScaleMenu(0);
    std::vector<int> ids;
    juce::PopupMenu::MenuItemIterator it(menu, true);
    while (it.next())
    {
        const int id = it.getItem().itemID;
        if (id > 0)
            ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    for (size_t i = 1; i < ids.size(); ++i)
        check(ids[i] != ids[i - 1], "Duplicate PopupMenu scale ID detected: " + std::to_string(ids[i]));
    check((int)ids.size() == ScaleQuantizer::numScales(), "PopupMenu count should match numScales()");
}

void testGetTailLengthSeconds()
{
    std::cerr << "Running testGetTailLengthSeconds...\n";
    auto procPtr = std::make_unique<GranularAudioProcessor>();
    auto& proc = *procPtr;
    proc.prepareToPlay(48000.0, 256);
    proc.resetAllParametersToDefault();

    setParamReal(proc, "release", 0.4f);
    setParamReal(proc, "fx1Type", 0.0f);
    setParamReal(proc, "fx5Type", 0.0f);
    check(std::abs(proc.getTailLengthSeconds() - 0.4) < 0.05, "Tail length with no FX should reflect release");

    setParamReal(proc, "fxMix", 1.0f);
    setParamReal(proc, "fx1Type", (float)FxChain::Reverb);
    setParamReal(proc, "fx1Amount", 50.0f);
    setParamReal(proc, "revDecay", 0.9f);
    check(proc.getTailLengthSeconds() > 6.0, "Tail length with Reverb active should expand dynamically");

    setParamReal(proc, "fxMix", 0.0f);
    check(proc.getTailLengthSeconds() < 1.0, "Tail length with fxMix=0 should exclude FX tail");
}

void testRandomizePresetGateUnits()
{
    std::cerr << "Running testRandomizePresetGateUnits...\n";
    auto procPtr = std::make_unique<GranularAudioProcessor>();
    auto& proc = *procPtr;
    proc.prepareToPlay(48000.0, 256);

    for (int iter = 0; iter < 50; ++iter)
    {
        proc.randomizePreset();
        const float gd = getParamReal(proc, "gateDepth");
        const float gk = getParamReal(proc, "gateDecay");
        const float gv = getParamReal(proc, "gateVowel");
        const int gr   = (int)std::round(getParamReal(proc, "gateRate"));
        const int gp   = (int)std::round(getParamReal(proc, "gatePattern"));

        check(gd >= 30.0f && gd <= 100.0f, "gateDepth should be in real percent units [30..100]");
        check(gk >= 10.0f && gk <= 100.0f, "gateDecay should be in real percent units [10..100]");
        check(gv >= 0.0f  && gv <= 100.0f, "gateVowel should be in real percent units [0..100]");
        check(gr >= 0     && gr <= 7,      "gateRate index should be in [0..7]");
        check(gp >= 0     && gp <= 49,     "gatePattern index should be in [0..49]");
    }
}

void testMonoBrickLimiterAndModulatedCeiling()
{
    std::cout << "Running testMonoBrickLimiterAndModulatedCeiling...\n";
    BrickLimiter limA, limB;
    limA.prepare(48000.0);
    limB.prepare(48000.0);
    limA.setReleaseMs(50.0f);
    limB.setReleaseMs(50.0f);

    const int limLatency = limA.getLatencySamples();
    float monoOut = 0.0f;
    float aliasedOut = 0.0f;
    for (int k = 0; k <= limLatency; ++k)
    {
        float m = 1.0f;
        float a = 1.0f;
        limA.processMono(m, 0.5f);
        limB.process(a, a, 0.5f);
        monoOut = m;
        aliasedOut = a;
    }
    check(std::abs(monoOut - 0.5f) < 1.0e-4f, "Mono limiter should clamp 1.0 to 0.5 after look-ahead latency");
    check(std::abs(aliasedOut - monoOut) < 1.0e-5f, "Aliased L==R process() should match processMono() with no double attenuation");

    // Verify modulated ceiling on actual processor
    juce::File tmpWav = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_lim.wav");
    check(createTempSineWav(tmpWav), "Failed to create temp wav for limiter test");

    auto procPtr = std::make_unique<GranularAudioProcessor>();
    auto& proc = *procPtr;
    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());
    check(proc.setBusesLayout(monoLayout), "Processor should accept mono output bus layout");
    proc.prepareToPlay(48000.0, 256);
    proc.loadFileForIngest(tmpWav);
    check(waitForSampleReady(proc), "Sample ingest timed out for limiter test");

    proc.resetAllParametersToDefault();
    setParamReal(proc, "attack", 0.001f);
    setParamReal(proc, "outGain", 12.0f);
    setParamReal(proc, "fxMix", 0.0f);
    setParamReal(proc, "ceiling", 0.0f);
    // Modulate LimCeiling (-6dB) via PitchBend or Env (use Env1 sustain=1, attack=0.001, DstLimCeiling, amt=-1.0)
    setParamReal(proc, "env1Attack", 0.001f);
    setParamReal(proc, "env1Sustain", 1.0f);
    setParamReal(proc, "mod1Src", (float)ModMatrix::SrcEnv1);
    setParamReal(proc, "mod1Dst", (float)ModMatrix::DstLimCeiling);
    setParamReal(proc, "mod1Amt", -1.0f);
    setParamReal(proc, "mod1Uni", 1.0f);

    juce::AudioBuffer<float> monoBuf(1, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    proc.processBlock(monoBuf, midi);

    const float expectedCeilLin = juce::Decibels::decibelsToGain(-6.0f);
    float maxPeak = 0.0f;
    for (int blk = 0; blk < 12; ++blk)
    {
        midi.clear();
        proc.processBlock(monoBuf, midi);
        for (int i = 0; i < 256; ++i)
            maxPeak = juce::jmax(maxPeak, std::abs(monoBuf.getSample(0, i)));
    }
    check(maxPeak > 0.05f, "Mono synth should produce non-trivial audio");
    check(maxPeak <= expectedCeilLin + 0.01f, "Modulated -6dB ceiling should limit peak below ~0.501");
    tmpWav.deleteFile();
}

void testAsyncPanicSafety()
{
    std::cerr << "Running testAsyncPanicSafety...\n";
    juce::File tmpWav = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_panic.wav");
    check(createTempSineWav(tmpWav), "Failed to create temp wav for panic test");

    auto procPtr = std::make_unique<GranularAudioProcessor>();
    auto& proc = *procPtr;
    proc.prepareToPlay(48000.0, 256);
    proc.loadFileForIngest(tmpWav);
    check(waitForSampleReady(proc), "Sample ingest timed out for panic test");

    proc.resetAllParametersToDefault();
    setParamReal(proc, "attack", 0.001f);
    setParamReal(proc, "release", 5.0f);
    setParamReal(proc, "fx1Type", (float)FxChain::Reverb);
    setParamReal(proc, "fx1Amount", 50.0f);
    setParamReal(proc, "revDecay", 0.95f);

    juce::AudioBuffer<float> buf(2, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    for (int b = 0; b < 16; ++b)
    {
        proc.processBlock(buf, midi);
        midi.clear();
    }
    check(computeRms(buf) > 0.01f, "Audio should be sounding before panic()");

    proc.panic();
    midi.clear();
    proc.processBlock(buf, midi);
    check(computeRms(buf) == 0.0f, "First processBlock after panic() must output complete silence");

    proc.processBlock(buf, midi);
    check(computeRms(buf) == 0.0f, "Second processBlock after panic() must remain silent (no reverb/voice leak)");
    tmpWav.deleteFile();
}

void testArpOffAndScaleToggleCleanup()
{
    std::cout << "Running testArpOffAndScaleToggleCleanup...\n";

    // Subtest A: Arpeggiator OFF NoteOff flush
    {
        Arpeggiator arp;
        arp.prepare(48000.0);
        ArpParams p;
        p.enable = true;
        p.sync = false;
        p.rateFree = 20.0f;
        p.gate = 0.8f;

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);
        arp.process(midi, 256, p);

        bool sawNoteOn = false;
        for (const auto m : midi)
            if (m.getMessage().isNoteOn()) sawNoteOn = true;
        check(sawNoteOn, "Arp should fire initial noteOn");

        // Disable Arp while step is still active
        p.enable = false;
        midi.clear();
        arp.process(midi, 256, p);
        bool sawNoteOff = false;
        for (const auto m : midi)
            if (m.getMessage().isNoteOff() && m.getMessage().getNoteNumber() == 60) sawNoteOff = true;
        check(sawNoteOff, "Arp OFF transition must emit NoteOff for active arp notes");
    }

    // Subtest B: scaleOn toggled mid-note must still release snapped voice
    {
        juce::File tmpWav = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_scale.wav");
        check(createTempSineWav(tmpWav), "Failed to create temp wav for scale test");

        auto procPtr = std::make_unique<GranularAudioProcessor>();
        auto& proc = *procPtr;
        proc.prepareToPlay(48000.0, 256);
        proc.loadFileForIngest(tmpWav);
        check(waitForSampleReady(proc), "Sample ingest timed out for scale test");

        proc.resetAllParametersToDefault();
        setParamReal(proc, "attack", 0.001f);
        setParamReal(proc, "release", 0.01f);
        setParamReal(proc, "fxMix", 0.0f);
        setParamReal(proc, "arpEnable", 0.0f);
        setParamReal(proc, "scaleOn", 1.0f);
        setParamReal(proc, "key", 1.0f);   // C
        setParamReal(proc, "scale", 0.0f); // Major

        juce::AudioBuffer<float> buf(2, 256);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 61, 0.9f), 0); // C#4 snaps to C4(60) or D4(62)
        for (int b = 0; b < 6; ++b)
        {
            proc.processBlock(buf, midi);
            midi.clear();
        }
        check(computeRms(buf) > 0.01f, "Note should sound before scaleOn toggled off");

        // Toggle scaleOn OFF BEFORE releasing key 61!
        setParamReal(proc, "scaleOn", 0.0f);
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOff(1, 61, 0.0f), 0);
        for (int b = 0; b < 30; ++b)
        {
            proc.processBlock(buf, midi);
            midi.clear();
        }
        check(computeRms(buf) < 1.0e-4f, "Toggling scaleOn OFF before NoteOff must not stick snapped voice");
        tmpWav.deleteFile();
    }
}

void testModMatrixSyncRatesAndGuiSnapshot()
{
    std::cout << "Running testModMatrixSyncRatesAndGuiSnapshot...\n";
    ModMatrix mm;
    mm.prepare(48000.0);

    auto measureAtSyncIdx = [&](int idx) -> float
    {
        mm.reset();
        ModMatrix::Params p;
        p.bpm = 120.0;
        p.lfo[0].sync = true;
        p.lfo[0].rateSync = idx;
        p.lfo[0].wave = 0; // Sine
        p.slot[0].src = ModMatrix::SrcLfo1;
        p.slot[0].dst = ModMatrix::DstPosition;
        p.slot[0].amt = 1.0f;
        p.slot[0].uni = false;
        mm.processBlock(750, p); // Block 1 evaluates phase=0 and advances phase
        mm.processBlock(750, p); // Block 2 evaluates at 750/48000s (0.125 vs 0.25 cycles)
        check(std::abs(mm.getGui(ModMatrix::DstPosition) - mm.get(ModMatrix::DstPosition)) < 1.0e-6f,
            "ModMatrix GUI snapshot should match audio accumulator");
        check(mm.getRangeMin(ModMatrix::DstPosition) <= mm.getGui(ModMatrix::DstPosition) + 1.0e-6f &&
              mm.getGui(ModMatrix::DstPosition) <= mm.getRangeMax(ModMatrix::DstPosition) + 1.0e-6f,
            "ModMatrix GUI range bounds invalid");
        return mm.get(ModMatrix::DstPosition);
    };

    const float v12 = measureAtSyncIdx(12); // 1/16 -> 8 Hz -> sin(pi/4) = 0.707
    const float v15 = measureAtSyncIdx(15); // 1/32 -> 16 Hz -> sin(pi/2) = 1.000
    check(std::abs(v12 - v15) > 0.2f, "LFO SyncRate index 15 should differ from index 12 (not clamped to 12)");
}

void testGrainVisualizerSeqLock()
{
    std::cerr << "Running testGrainVisualizerSeqLock...\n";
    auto vizPtr = std::make_unique<GrainVisualizerData>();
    auto& viz = *vizPtr;
    std::atomic<bool> done { false };
    std::atomic<int> mismatch { 0 };

    std::thread writer([&]()
    {
        for (int i = 1; i <= 100000; ++i)
        {
            const float v = (float)i;
            viz.push(v, v, v, v);
        }
        done.store(true, std::memory_order_release);
    });

    std::thread reader([&]()
    {
        GrainVisualizerData::EventData d;
        while (!done.load(std::memory_order_acquire))
        {
            const uint32_t c = viz.counter.load(std::memory_order_acquire);
            if (c > 0 && viz.tryRead(c - 1, d))
            {
                if (d.normPos != d.pan || d.normPos != d.semitones || d.normPos != d.durationSec)
                    mismatch.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    writer.join();
    reader.join();
    check(mismatch.load() == 0, "SeqLock read produced torn GrainVisualizerData fields");
}

void writeBigEndianU32(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back((uint8_t)((v >> 24) & 0xFF));
    out.push_back((uint8_t)((v >> 16) & 0xFF));
    out.push_back((uint8_t)((v >> 8) & 0xFF));
    out.push_back((uint8_t)(v & 0xFF));
}

void writeBigEndianU16(std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back((uint8_t)((v >> 8) & 0xFF));
    out.push_back((uint8_t)(v & 0xFF));
}

void testReadAbletonAifBounds()
{
    std::cout << "Running testReadAbletonAifBounds...\n";
    juce::File badAif = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_bad_test.aif");

    // 1) Malformed FORM header with absurd huge chunk size
    {
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), { 'F', 'O', 'R', 'M' });
        writeBigEndianU32(raw, 0x7FFFFFF0u);
        raw.insert(raw.end(), { 'A', 'I', 'F', 'F' });
        raw.insert(raw.end(), { 'C', 'O', 'M', 'M' });
        writeBigEndianU32(raw, 18);
        writeBigEndianU16(raw, 0); // 0 channels (invalid)
        writeBigEndianU32(raw, 100000);
        writeBigEndianU16(raw, 16);
        raw.insert(raw.end(), 10, 0);
        badAif.replaceWithData(raw.data(), raw.size());

        juce::AudioBuffer<float> buf;
        double sr = 0.0;
        check(!GranularAudioProcessor::readAbletonAif(badAif, buf, sr),
              "Malformed 0-ch AIFF should safely return false");
    }

    // 2) Valid minimal 16-bit mono AIFC at 44100 Hz
    {
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), { 'F', 'O', 'R', 'M' });
        writeBigEndianU32(raw, 4 + 30 + 24);
        raw.insert(raw.end(), { 'A', 'I', 'F', 'C' });
        // COMM chunk (size = 22: 18 + 4-byte compression type)
        raw.insert(raw.end(), { 'C', 'O', 'M', 'M' });
        writeBigEndianU32(raw, 22);
        writeBigEndianU16(raw, 1);     // 1 ch
        writeBigEndianU32(raw, 4);     // 4 frames
        writeBigEndianU16(raw, 16);    // 16-bit
        // 44100.0 Hz in 80-bit IEEE extended: exp = 0x400E, mant = 0xAC44000000000000
        raw.insert(raw.end(), { 0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        raw.insert(raw.end(), { 'N', 'O', 'N', 'E' });
        // SSND chunk (size = 16: 8 header + 4 * 2 bytes)
        raw.insert(raw.end(), { 'S', 'S', 'N', 'D' });
        writeBigEndianU32(raw, 16);
        writeBigEndianU32(raw, 0); // offset
        writeBigEndianU32(raw, 0); // blockSize
        writeBigEndianU16(raw, 16384); // +0.5
        writeBigEndianU16(raw, 0);
        writeBigEndianU16(raw, (uint16_t)-16384); // -0.5
        writeBigEndianU16(raw, 0);
        badAif.replaceWithData(raw.data(), raw.size());

        juce::AudioBuffer<float> buf;
        double sr = 0.0;
        check(GranularAudioProcessor::readAbletonAif(badAif, buf, sr), "Valid minimal AIFC failed to load");
        check(std::abs(sr - 44100.0) < 1.0, "Valid AIFC sample rate decoded incorrectly");
        check(buf.getNumSamples() == 4, "Valid AIFC frame count mismatch");
        if (buf.getNumSamples() == 4)
            check(std::abs(buf.getSample(0, 0) - 0.5f) < 1.0e-3f, "Valid AIFC PCM sample mismatch");
    }

    // 3) Oversized COMM numSampleFrames (0xFFFFFFFF) must safely cap to available SSND chunk bytes without OOM
    {
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), { 'F', 'O', 'R', 'M' });
        writeBigEndianU32(raw, 4 + 30 + 20);
        raw.insert(raw.end(), { 'A', 'I', 'F', 'C' });
        raw.insert(raw.end(), { 'C', 'O', 'M', 'M' });
        writeBigEndianU32(raw, 22);
        writeBigEndianU16(raw, 1);
        writeBigEndianU32(raw, 0xFFFFFFFFu); // Absurd inflated frame count
        writeBigEndianU16(raw, 16);
        raw.insert(raw.end(), { 0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        raw.insert(raw.end(), { 'N', 'O', 'N', 'E' });
        raw.insert(raw.end(), { 'S', 'S', 'N', 'D' });
        writeBigEndianU32(raw, 12);
        writeBigEndianU32(raw, 0);
        writeBigEndianU32(raw, 0);
        writeBigEndianU16(raw, 8192);
        writeBigEndianU16(raw, 8192);
        badAif.replaceWithData(raw.data(), raw.size());

        juce::AudioBuffer<float> buf;
        double sr = 0.0;
        check(GranularAudioProcessor::readAbletonAif(badAif, buf, sr), "Bounded AIFC should cap oversized frame count");
        check(buf.getNumSamples() == 2, "Bounded AIFC must limit frames to SSND bytes (2)");
    }

    badAif.deleteFile();
}

void testPitchAnalyzerYinMpmAnd24Key()
{
    std::cout << "Running testPitchAnalyzerYinMpmAnd24Key...\n";
    constexpr double sr = 44100.0;

    // 1. Multi-harmonic sawtooth wave across 55, 110, 440, 1760 Hz: zero octave jumps
    const double targetFreqs[4] = { 55.0, 110.0, 440.0, 1760.0 };
    for (double f0 : targetFreqs)
    {
        const int len = 16384;
        std::vector<float> buf((size_t)len, 0.0f);
        for (int i = 0; i < len; ++i)
        {
            double s = 0.0;
            for (int h = 1; h <= 8; ++h)
            {
                if ((double)h * f0 < sr * 0.45)
                    s += std::sin(juce::MathConstants<double>::twoPi * f0 * (double)h * (double)i / sr) / (double)h;
            }
            buf[(size_t)i] = (float)(s * 0.4);
        }

        const PitchInfo pi = PitchAnalyzer::analyzeOmni(buf.data(), len, sr, "saw_test.wav", 0);
        check(pi.isDetected, ("Pitch should be detected for " + std::to_string(f0) + " Hz saw").c_str());
        check(std::abs(pi.frequency - f0) < f0 * 0.015,
              ("Octave-accurate YIN/MPM failed at " + std::to_string(f0) + " Hz, got " + std::to_string(pi.frequency)).c_str());
        check(pi.clarity > 0.7f, "Harmonic saw clarity should exceed 0.7");
    }

    // 2. Micro sample (< 1024 samples): single-cycle / short 256-sample wavetable
    {
        const int len = 256;
        const double f0 = sr / 128.0; // 2 cycles = 344.53125 Hz
        std::vector<float> micro((size_t)len);
        for (int i = 0; i < len; ++i)
            micro[(size_t)i] = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 2.0f * (float)i / (float)len);

        const PitchInfo pi = PitchAnalyzer::analyzeOmni(micro.data(), len, sr, "micro.wav", 0);
        check(pi.isDetected, "Micro sample (<1024) must still detect pitch");
        check(std::abs(pi.frequency - f0) < 5.0,
              ("Micro sample pitch mismatch: expected " + std::to_string(f0) + ", got " + std::to_string(pi.frequency)).c_str());
    }

    // 3. HatCym route full-rate FFT (7000 Hz > 5512.5 Hz old alias threshold!)
    {
        const int len = 8192;
        std::vector<float> hat((size_t)len);
        for (int i = 0; i < len; ++i)
            hat[(size_t)i] = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 7000.0f * (float)i / (float)sr);

        const PitchInfo pi = PitchAnalyzer::analyzeOmni(hat.data(), len, sr, "closed_hat_hi.wav", 0);
        check(pi.isDetected, "HatCym FFT should detect 7000 Hz");
        check(std::abs(pi.frequency - 7000.0) < 25.0,
              ("HatCym 7000 Hz should not alias down, got " + std::to_string(pi.frequency)).c_str());
    }

    // 4. Temperley 24-Key Detection (A Minor triad with loud fifth E4 vs C Major triad)
    {
        const int len = 16384;
        std::vector<float> am((size_t)len);
        std::vector<float> cm((size_t)len);
        for (int i = 0; i < len; ++i)
        {
            const double t = (double)i / sr;
            // A3 (220), C4 (261.63), loud E4 (329.63)
            am[(size_t)i] = (float)(0.25 * std::sin(juce::MathConstants<double>::twoPi * 220.0 * t)
                                  + 0.25 * std::sin(juce::MathConstants<double>::twoPi * 261.6256 * t)
                                  + 0.35 * std::sin(juce::MathConstants<double>::twoPi * 329.6276 * t));
            // C3 (130.81), E4 (329.63), G4 (392.00)
            cm[(size_t)i] = (float)(0.30 * std::sin(juce::MathConstants<double>::twoPi * 130.8128 * t)
                                  + 0.25 * std::sin(juce::MathConstants<double>::twoPi * 329.6276 * t)
                                  + 0.30 * std::sin(juce::MathConstants<double>::twoPi * 391.9954 * t));
        }

        const PitchInfo piAm = PitchAnalyzer::analyzeOmni(am.data(), len, sr, "chord_pad.wav", 0);
        check(piAm.detectedRoot == 9 && !piAm.isMajor,
              ("Expected A Minor (root=9, min), got root=" + std::to_string(piAm.detectedRoot) + (piAm.isMajor ? " Maj" : " Min")).c_str());

        const PitchInfo piCm = PitchAnalyzer::analyzeOmni(cm.data(), len, sr, "chord_maj.wav", 0);
        check(piCm.detectedRoot == 0 && piCm.isMajor,
              ("Expected C Major (root=0, maj), got root=" + std::to_string(piCm.detectedRoot) + (piCm.isMajor ? " Maj" : " Min")).c_str());
    }
}

void testStereoIngestRightOnlyAndMicroRender()
{
    std::cout << "Running testStereoIngestRightOnlyAndMicroRender...\n";
    juce::File tmpR = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_right_only.wav");
    juce::File tmpMicro = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_micro256.wav");

    // 1. Stereo file: Left silent, Right = 440 Hz @ -30dB in samples [4000..24000]
    {
        const int total = 48000;
        juce::AudioBuffer<float> buf(2, total);
        buf.clear();
        for (int i = 4000; i < 24000; ++i)
            buf.setSample(1, i, 0.0316f * std::sin(juce::MathConstants<float>::twoPi * 440.0f * (float)i / 48000.0f));

        tmpR.deleteFile();
        juce::WavAudioFormat wav;
        if (std::unique_ptr<juce::OutputStream> st{ tmpR.createOutputStream() })
            if (auto w = wav.createWriterFor(st, juce::AudioFormatWriterOptions{}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(24)))
                w->writeFromAudioSampleBuffer(buf, 0, total);

        auto procPtr = std::make_unique<GranularAudioProcessor>();
        auto& proc = *procPtr;
        proc.prepareToPlay(48000.0, 256);
        proc.loadFileForIngest(tmpR);
        check(waitForSampleReady(proc), "Right-only stereo ingest timed out");

        const int s = proc.trimStartSample.load();
        const int e = proc.trimEndSample.load();
        check(s > 3500 && e < 24500, "Right-channel quiet sample should trim leading/trailing silence accurately");
        check(proc.currentDetectedRoot.load() == 9 && proc.rootMidiNote.load() == 57,
              "L+R stereo mix should detect 440 Hz (A -> folded root 57) present only on Right channel");
        {
            const juce::ScopedLock sl(proc.stateLock);
            check(proc.hudLineOrig.contains("440."), "HUD should report detected 440 Hz from Right channel");
        }
    }

    // 2. Micro sample (256 samples): waveform bins 0..999 all populated AND audio renders
    {
        const int total = 256;
        juce::AudioBuffer<float> buf(2, total);
        for (int i = 0; i < total; ++i)
        {
            const float v = 0.6f * std::sin(juce::MathConstants<float>::twoPi * 4.0f * (float)i / (float)total);
            buf.setSample(0, i, v);
            buf.setSample(1, i, v);
        }

        tmpMicro.deleteFile();
        juce::WavAudioFormat wav;
        if (std::unique_ptr<juce::OutputStream> st{ tmpMicro.createOutputStream() })
            if (auto w = wav.createWriterFor(st, juce::AudioFormatWriterOptions{}.withSampleRate(48000.0).withNumChannels(2).withBitsPerSample(16)))
                w->writeFromAudioSampleBuffer(buf, 0, total);

        auto procPtr = std::make_unique<GranularAudioProcessor>();
        auto& proc = *procPtr;
        proc.prepareToPlay(48000.0, 256);
        proc.loadFileForIngest(tmpMicro);
        check(waitForSampleReady(proc), "Micro 256-sample ingest timed out");
        check(proc.waveformPeaks[995] > 0.05f, "Micro sample should populate tail bins up to 999");

        proc.resetAllParametersToDefault();
        setParamReal(proc, "attack", 0.001f);
        setParamReal(proc, "fxMix", 0.0f);
        juce::AudioBuffer<float> out(2, 256);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        for (int b = 0; b < 4; ++b)
        {
            proc.processBlock(out, midi);
            midi.clear();
        }
        check(computeRms(out) > 0.01f, "Micro sample (<512 samples) must render audible granular output");
    }

    tmpR.deleteFile();
    tmpMicro.deleteFile();
}

void testScaleQuantizerBoundaryClamp()
{
    std::cout << "Running testScaleQuantizerBoundaryClamp...\n";
    const int nScales = ScaleQuantizer::numScales();
    const float testInputs[5] = { -12.0f, 0.0f, 126.9f, 127.0f, 132.0f };
    for (int root = 0; root < 12; ++root)
    {
        for (int sc = 0; sc < nScales; ++sc)
        {
            for (float inVal : testInputs)
            {
                const float q = ScaleQuantizer::quantize(inVal, root, sc);
                check(q >= 0.0f && q <= 127.0f, "ScaleQuantizer::quantize exceeded [0, 127]");
            }
        }
    }

    // C# Major (root=1, scale=1=Major): MIDI 127 (G) is out of scale, 128 (G#) > 127 -> must snap to 126 (F#)
    const float qTop = ScaleQuantizer::quantize(127.0f, 1, 1);
    check(std::abs(qTop - 126.0f) < 1.0e-4f,
          ("Expected C# Major quantize(127) -> 126 (F#), got " + std::to_string(qTop)).c_str());
}

void testSubBlockMidiTimingAndWindowRms()
{
    std::cout << "Running testSubBlockMidiTimingAndWindowRms...\n";
    constexpr double sr = 48000.0;
    juce::AudioBuffer<float> src(2, 48000);
    for (int i = 0; i < 48000; ++i)
    {
        const float v = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 261.6256f * (float)i / (float)sr);
        src.setSample(0, i, v);
        src.setSample(1, i, v);
    }

    GranularSynth synth;
    synth.prepareToPlay(sr, 512);

    GranularParams p;
    p.attack = 0.001f;
    p.density = 0.5f;
    p.grainSize = 0.06f;
    p.position = 0.5f;
    p.windowShape = 2; // Square (1% fast edge)
    p.masterHPF = 20.0f;
    p.masterLPF = 20000.0f;
    p.speed = 1.0f;

    juce::AudioBuffer<float> out(2, 512);
    out.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 250); // Sample-accurate NoteOn at 250!

    synth.processBlock(out, midi, &src, 0, 48000, p);

    float prePeak = 0.0f, postPeak = 0.0f;
    for (int i = 0; i < 250; ++i)
        prePeak = juce::jmax(prePeak, std::abs(out.getSample(0, i)));
    for (int i = 250; i < 512; ++i)
        postPeak = juce::jmax(postPeak, std::abs(out.getSample(0, i)));

    check(prePeak == 0.0f, "Samples [0..249] before NoteOn at sample 250 must be strictly zero");
    check(postPeak > 0.02f, "Samples [250..511] after NoteOn must sound immediately");

    // Window RMS parity check across shapes 0..3
    float rmsPerShape[4] = {};
    for (int w = 0; w < 4; ++w)
    {
        synth.reset();
        p.windowShape = w;
        p.pitchJitter = 0.0f;
        p.positionSpray = 0.0f;
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);

        double sumSq = 0.0;
        int count = 0;
        for (int blk = 0; blk < 20; ++blk)
        {
            out.clear();
            synth.processBlock(out, midi, &src, 0, 48000, p);
            midi.clear();
            if (blk >= 4)
            {
                const float* d = out.getReadPointer(0);
                for (int i = 0; i < 512; ++i) { sumSq += (double)d[i] * d[i]; ++count; }
            }
        }
        rmsPerShape[w] = (float)std::sqrt(sumSq / (double)juce::jmax(1, count));
        check(rmsPerShape[w] > 0.05f, "Window shape RMS should be substantial");
    }
    const float minW = *std::min_element(rmsPerShape, rmsPerShape + 4);
    const float maxW = *std::max_element(rmsPerShape, rmsPerShape + 4);
    check(maxW / juce::jmax(1.0e-5f, minW) < 1.35f,
          ("RMS-normalized windows should match within 1.35x, ratio=" + std::to_string(maxW / minW)).c_str());
}

void testButterworth24DbCascadeAndCombThiran()
{
    std::cout << "Running testButterworth24DbCascadeAndCombThiran...\n";
    constexpr double sr = 48000.0;

    // 1. 4th-order Butterworth cascade: -3.01dB at fc (1000 Hz) and -48dB at 4000 Hz
    auto measureSvfGain = [&](double testFreqHz) -> float
    {
        TptSvf s0, s1;
        s0.prepare(sr);
        s1.prepare(sr);
        s0.setTarget(1000.0f, 0.54119610f);
        s1.setTarget(1000.0f, 1.30656296f);

        double sq = 0.0;
        int cnt = 0;
        for (int i = 0; i < 12000; ++i)
        {
            const float x = std::sin(juce::MathConstants<float>::twoPi * (float)testFreqHz * (float)i / (float)sr);
            const float y = s1.processLP(s0.processLP(x));
            if (i >= 8000) { sq += (double)y * y; ++cnt; }
        }
        return (float)std::sqrt((sq / (double)cnt) / 0.5);
    };

    const float gainAtFc = measureSvfGain(1000.0);
    const float gainAt4Fc = measureSvfGain(4000.0);
    check(std::abs(gainAtFc - 0.70710678f) < 0.04f,
          ("4th-order Butterworth @ fc must be ~0.7071 (-3dB), got " + std::to_string(gainAtFc)).c_str());
    check(gainAt4Fc < 0.006f,
          ("4th-order Butterworth @ 4*fc must be < 0.006 (-48dB), got " + std::to_string(gainAt4Fc)).c_str());

    // 2. Comb TONE Thiran AP + DC blocker prevents runaway DC offset
    juce::AudioBuffer<float> biasedSrc(2, 48000);
    for (int i = 0; i < 48000; ++i)
    {
        const float v = 0.25f + 0.35f * std::sin(juce::MathConstants<float>::twoPi * 440.0f * (float)i / (float)sr);
        biasedSrc.setSample(0, i, v);
        biasedSrc.setSample(1, i, v);
    }

    GranularSynth synth;
    synth.prepareToPlay(sr, 512);
    GranularParams p;
    p.attack = 0.001f;
    p.tone = 0.9f;
    p.density = 0.6f;
    p.grainSize = 0.05f;
    p.masterHPF = 20.0f;
    p.masterLPF = 20000.0f;

    juce::AudioBuffer<float> out(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, 1.0f), 0);

    for (int blk = 0; blk < 60; ++blk)
    {
        out.clear();
        synth.processBlock(out, midi, &biasedSrc, 0, 48000, p);
        midi.clear();
    }

    double sum = 0.0;
    int totalSamples = 0;
    for (int blk = 0; blk < 16; ++blk)
    {
        out.clear();
        synth.processBlock(out, midi, &biasedSrc, 0, 48000, p);
        for (int i = 0; i < 512; ++i)
        {
            const float s = out.getSample(0, i);
            check(isFiniteFloat(s), "TONE comb resonator output must be finite");
            sum += (double)s;
            ++totalSamples;
        }
    }
    const double mean = sum / (double)totalSamples;
    check(std::abs(mean) < 0.005,
          ("TONE comb DC blocker failed to settle DC offset, mean=" + std::to_string(mean)).c_str());
}

void testProcessorFootprintAndFxChain192k()
{
    std::cout << "Running testProcessorFootprintAndFxChain192k...\n";
    check(sizeof(GranularAudioProcessor) < 350000,
          ("GranularAudioProcessor object size too large: " + std::to_string(sizeof(GranularAudioProcessor))).c_str());

    // 1. 192kHz全FX安定性 & Velvet左右非相関確認
    FxChain fx;
    fx.prepare(192000.0);
    FxChain::Params p;
    p.bpm = 128.0;
    p.type = { FxChain::Saturation, FxChain::Chorus, FxChain::Delay, FxChain::Freeze, FxChain::Reverb };
    p.amount = { 0.8f, 0.5f, 0.5f, 0.7f, 0.8f };
    p.satPreHz = 20.0f; // HPFバイパス境界
    p.satDrive = 3.0f;
    p.revDecay = 0.8f;
    p.revShimmer = 0.5f;

    juce::AudioBuffer<float> buf(2, 512);
    double diffSum = 0.0;
    for (int blk = 0; blk < 20; ++blk)
    {
        for (int i = 0; i < 512; ++i)
        {
            const int n = blk * 512 + i;
            const float x = (n < 1000) ? 0.4f * std::sin(juce::MathConstants<float>::twoPi * 440.0f * (float)n / 192000.0f) : 0.0f;
            buf.setSample(0, i, x);
            buf.setSample(1, i, x); // 完全モノラル入力
        }
        fx.process(buf, p);
        for (int i = 0; i < 512; ++i)
        {
            const float l = buf.getSample(0, i);
            const float r = buf.getSample(1, i);
            check(isFiniteFloat(l) && isFiniteFloat(r), "192kHz FxChain must remain finite");
            check(std::abs(l) < 4.0f && std::abs(r) < 4.0f, "192kHz FxChain amplitude bounded");
            diffSum += std::abs((double)l - (double)r);
        }
    }
    check(diffSum > 0.5, ("Velvet L/R & Stereo FX must decorrelate mono input, diffSum=" + std::to_string(diffSum)).c_str());

    // 2. FormantGate 正規化BP ゲイン & ADAA 6Hz DCブロッカー確認
    gfx::FormantGate gate;
    gate.prepare(48000.0);
    gate.setParams(4, 3, 1.0f, 0.99f, 1.0f); // Legato all-on, full vowel
    float maxGatePeak = 0.0f;
    for (int i = 0; i < 4096; ++i)
    {
        float l = 0.5f * std::sin(juce::MathConstants<float>::twoPi * 730.0f * (float)i / 48000.0f);
        float r = l;
        gate.process(l, r, 120.0, 0.0, false);
        check(isFiniteFloat(l) && isFiniteFloat(r), "FormantGate output finite");
        maxGatePeak = juce::jmax(maxGatePeak, std::abs(l));
    }
    check(maxGatePeak < 1.2f, ("Normalized FormantGate must not clip explosively at F1 resonance, peak=" + std::to_string(maxGatePeak)).c_str());

    // 3. Pre-HPFバイパス時は20Hz重低音が減衰せずSatを通過する
    FxChain fxSat;
    fxSat.prepare(48000.0);
    FxChain::Params sp;
    sp.type = { FxChain::Saturation, 0, 0, 0, 0 };
    sp.amount = { 1.0f, 0, 0, 0, 0 };
    sp.satAlgo = 0;
    sp.satDrive = 2.0f;
    sp.satPreHz = 20.0f; // バイパス!
    juce::AudioBuffer<float> subBuf(2, 4800);
    for (int i = 0; i < 4800; ++i)
    {
        const float s = 0.4f * std::sin(juce::MathConstants<float>::twoPi * 25.0f * (float)i / 48000.0f);
        subBuf.setSample(0, i, s);
        subBuf.setSample(1, i, s);
    }
    fxSat.process(subBuf, sp);
    double rms25Hz = 0.0;
    for (int i = 2400; i < 4800; ++i) rms25Hz += (double)subBuf.getSample(0, i) * subBuf.getSample(0, i);
    rms25Hz = std::sqrt(rms25Hz / 2400.0);
    check(rms25Hz > 0.25, ("6Hz DC blocker + Pre-HPF bypass should preserve 25Hz sub RMS > 0.25, got " + std::to_string(rms25Hz)).c_str());
}

void testModMatrixLorenzChaosLfo()
{
    std::cout << "Running testModMatrixLorenzChaosLfo...\n";
    ModMatrix mod;
    mod.prepare(48000.0);
    ModMatrix::Params mp;
    mp.lfo[0].wave = 5; // Lorenz Chaos
    mp.lfo[0].sync = false;
    mp.lfo[0].rateHz = 4.0f;
    mp.lfo[1].wave = 5;
    mp.lfo[1].sync = false;
    mp.lfo[1].rateHz = 4.0f;

    mp.slot[0].src = ModMatrix::SrcLfo1;
    mp.slot[0].dst = ModMatrix::DstPosition;
    mp.slot[0].amt = 1.0f;
    mp.slot[0].uni = false;

    mp.slot[1].src = ModMatrix::SrcLfo2;
    mp.slot[1].dst = ModMatrix::DstSize;
    mp.slot[1].amt = 1.0f;
    mp.slot[1].uni = false;

    std::vector<float> traj1, traj2;
    for (int b = 0; b < 240; ++b)
    {
        mod.processBlock(256, mp);
        const float v1 = mod.get(ModMatrix::DstPosition);
        const float v2 = mod.get(ModMatrix::DstSize);
        check(isFiniteFloat(v1) && v1 >= -1.0f && v1 <= 1.0f, "Lorenz Chaos LFO1 must stay within [-1, 1]");
        check(isFiniteFloat(v2) && v2 >= -1.0f && v2 <= 1.0f, "Lorenz Chaos LFO2 must stay within [-1, 1]");
        traj1.push_back(v1);
        traj2.push_back(v2);
    }

    double sum1 = 0.0, sq1 = 0.0, diff12 = 0.0;
    for (size_t i = 0; i < traj1.size(); ++i)
    {
        sum1 += traj1[i];
        sq1 += (double)traj1[i] * traj1[i];
        diff12 += std::abs((double)traj1[i] - (double)traj2[i]);
    }
    const double mean1 = sum1 / (double)traj1.size();
    const double std1 = std::sqrt(juce::jmax(0.0, sq1 / (double)traj1.size() - mean1 * mean1));
    check(std1 > 0.10, ("Lorenz LFO must exhibit real chaotic swing, std=" + std::to_string(std1)).c_str());
    check(diff12 / (double)traj1.size() > 0.05, "Different LFO indices must follow diverging Lorenz orbits");
}

void testArpeggiatorLatchOffStrumCrossBlockAndEuclidean()
{
    std::cout << "Running testArpeggiatorLatchOffStrumCrossBlockAndEuclidean...\n";

    // 1. Latch OFF transition clears non-physical held notes and emits NoteOff
    {
        Arpeggiator arp;
        arp.prepare(44100.0);
        ArpParams p;
        p.enable = true;
        p.latch = true;
        p.pattern = Arpeggiator::Up;

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 64, 1.0f), 0);
        arp.process(midi, 256, p);

        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 64, 0.0f), 0);
        arp.process(midi, 256, p); // Still latched

        p.latch = false;
        bool gotNoteOff60 = false, gotNoteOff64 = false, gotLaterNoteOn = false;
        for (int b = 0; b < 4; ++b)
        {
            midi.clear();
            arp.process(midi, 256, p);
            for (const auto& m : midi)
            {
                const auto msg = m.getMessage();
                if (msg.isNoteOff() && msg.getNoteNumber() == 60) gotNoteOff60 = true;
                if (msg.isNoteOff() && msg.getNoteNumber() == 64) gotNoteOff64 = true;
                if (b > 0 && msg.isNoteOn()) gotLaterNoteOn = true;
            }
        }
        check(gotNoteOff60 || gotNoteOff64, "Latch OFF must stop held latched note");
        check(!gotLaterNoteOn, "After Latch OFF, no further latched NoteOn should fire");
    }

    // 2. Strum cross-block delivery with small 64-sample buffer
    {
        Arpeggiator arp;
        arp.prepare(44100.0);
        ArpParams p;
        p.enable = true;
        p.pattern = Arpeggiator::Strum;
        p.latch = true;
        p.bpm = 60.0;
        p.rateSync = 4; // 1/2 note -> slow step

        std::array<bool, 128> firedNotes {};
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 64, 1.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 67, 1.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 71, 1.0f), 0);

        for (int b = 0; b < 45; ++b)
        {
            if (b > 0) midi.clear();
            arp.process(midi, 64, p);
            for (const auto& m : midi)
            {
                check(m.samplePosition >= 0 && m.samplePosition < 64, "Strum MIDI offset must be inside block [0, 64)");
                if (m.getMessage().isNoteOn())
                    firedNotes[(size_t)m.getMessage().getNoteNumber()] = true;
            }
        }
        check(firedNotes[60] && firedNotes[64] && firedNotes[67] && firedNotes[71],
              "Cross-block Strum must emit all 4 chord notes reliably across 64-sample blocks");
    }

    // 3. Euclidean Rhythm E(3, 8, 0) & step count
    {
        const bool expectedTresillo[8] = { true, false, false, true, false, false, true, false };
        for (int s = 0; s < 8; ++s)
            check(Arpeggiator::isEuclideanHit(s, 3, 8, 0) == expectedTresillo[s], "E(3,8,0) must match Tresillo");

        Arpeggiator arp;
        arp.prepare(48000.0);
        ArpParams p;
        p.enable = true;
        p.sync = false;
        p.rateFree = 10.0f; // 1 step per 4800 samples
        p.euclidEnable = true;
        p.euclidSteps = 8;
        p.euclidFills = 3;
        p.euclidRot = 0;

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        int noteOnCount = 0;
        // 8 steps * 4800 = 38400 samples -> 75 blocks of 512 = 38400 samples (0..7 steps = 8 steps)
        for (int b = 0; b < 75; ++b)
        {
            if (b > 0) midi.clear();
            arp.process(midi, 512, p);
            for (const auto& m : midi)
                if (m.getMessage().isNoteOn()) ++noteOnCount;
        }
        check(noteOnCount == 3, ("Euclidean E(3,8) over 8 steps must trigger 3 notes, got " + std::to_string(noteOnCount)).c_str());
    }
}

void testMipMapAntiAliasingPitchUp()
{
    std::cout << "Running testMipMapAntiAliasingPitchUp...\n";
    constexpr double sr = 48000.0;
    juce::AudioBuffer<float> hiSrc(2, 48000);
    for (int i = 0; i < 48000; ++i)
    {
        // 10kHz 高調波素材 (+2oct再生=40kHz相当でエイリアシング試験)
        const float v = 0.7f * std::sin(juce::MathConstants<float>::twoPi * 10000.0f * (float)i / (float)sr);
        hiSrc.setSample(0, i, v);
        hiSrc.setSample(1, i, v);
    }

    SampleMipLevels mips;
    mips.buildFrom(hiSrc);
    check(mips.numExtraLevels == SampleMipLevels::kExtraLevels, "MipMap must generate 4 extra downsampled levels");

    auto measurePitchUpRms = [&](const SampleMipLevels* mipPtr) -> float
    {
        GranularSynth synth;
        synth.prepareToPlay(sr, 512);
        GranularParams p;
        p.attack = 0.001f;
        p.octave = 2; // +2 octaves -> step = 4.0!
        p.rootNote = 60;
        p.outGainDb = 0.0f;
        p.density = 0.7f;
        p.grainSize = 0.06f;
        p.pitchJitter = 0.0f;
        p.masterHPF = 20.0f;
        p.masterLPF = 20000.0f;

        juce::AudioBuffer<float> out(2, 512);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);

        double sq = 0.0;
        int cnt = 0;
        for (int blk = 0; blk < 16; ++blk)
        {
            out.clear();
            synth.processBlock(out, midi, &hiSrc, 0, 48000, p, mipPtr);
            midi.clear();
            if (blk >= 4)
            {
                const float* d = out.getReadPointer(0);
                for (int i = 0; i < 512; ++i) { sq += (double)d[i] * d[i]; ++cnt; }
            }
        }
        return (float)std::sqrt(sq / (double)juce::jmax(1, cnt));
    };

    const float unantialiasedRms = measurePitchUpRms(nullptr);
    const float mipMappedRms     = measurePitchUpRms(&mips);
    check(unantialiasedRms > 0.15f, "Without MipMap, 10kHz at +2 octaves aliases strongly back into audible band");
    check(mipMappedRms < unantialiasedRms * 0.08f,
          ("MipMap must suppress high-pitch foldback by >22dB, raw=" + std::to_string(unantialiasedRms)
           + " mip=" + std::to_string(mipMappedRms)).c_str());
}

void testLookAheadLimiterZeroOvershootAndLatency()
{
    std::cout << "Running testLookAheadLimiterZeroOvershootAndLatency...\n";
    BrickLimiter lim;
    lim.prepare(48000.0);
    const int latency = lim.getLatencySamples();
    check(latency == 72, ("Look-Ahead Limiter at 48kHz must have 72-sample (1.5ms) latency, got " + std::to_string(latency)).c_str());

    const float ceil = 0.25f;
    std::vector<float> outL(512, 0.0f), outR(512, 0.0f);
    for (int i = 0; i < 512; ++i)
    {
        float l = (i == 200) ? 5.0f : ((i >= 180 && i < 200) ? 0.20f : 0.0f);
        float r = l;
        lim.process(l, r, ceil);
        outL[(size_t)i] = l;
        outR[(size_t)i] = r;
        check(std::abs(l) <= ceil + 1.0e-6f && std::abs(r) <= ceil + 1.0e-6f,
              ("Look-Ahead Limiter must never overshoot ceiling, sample=" + std::to_string(i)
               + " val=" + std::to_string(l)).c_str());
    }
    check(std::abs(outL[(size_t)(200 + latency)]) > 0.1f, "Impulse peak must emerge aligned at exact lookahead offset");
    check(std::abs(outL[(size_t)(200 + latency - 10)]) < 0.20f, "Look-Ahead gain ramp should attenuate quietly before peak arrives");

    auto proc = std::make_unique<GranularAudioProcessor>();
    proc->prepareToPlay(48000.0, 256);
    check(proc->getLatencySamples() == latency, "Processor must publish exact Look-Ahead Limiter latency to DAW");
}

void testClicklessDryPreviewRamp()
{
    std::cout << "Running testClicklessDryPreviewRamp...\n";
    juce::File tmpWav = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("granular_test_dry_ramp.wav");
    {
        juce::WavAudioFormat wav;
        juce::AudioBuffer<float> buf(1, 48000);
        for (int i = 0; i < 48000; ++i) buf.setSample(0, i, 0.5f); // DCステップでクリック検出しやすくする
        tmpWav.deleteFile();
        if (auto s = std::unique_ptr<juce::FileOutputStream>(tmpWav.createOutputStream()))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(s.get(), 48000.0, 1, 24, {}, 0)))
            { s.release(); w->writeFromAudioSampleBuffer(buf, 0, 48000); }
    }

    auto proc = std::make_unique<GranularAudioProcessor>();
    proc->prepareToPlay(48000.0, 256);
    proc->loadFileForIngest(tmpWav);
    check(waitForSampleReady(*proc), "Sample ingest timed out for dry preview ramp test");

    proc->resetAllParametersToDefault();
    juce::AudioBuffer<float> buf(2, 256);
    juce::MidiBuffer midi;

    proc->startDryPreview();
    proc->processBlock(buf, midi);
    check(std::abs(buf.getSample(0, 0)) < 0.02f, "Dry preview start must fade in smoothly from ~0");

    // 立ち上がり完了後、途中停止要求でクリックなくゼロへフェードアウト
    proc->processBlock(buf, midi);
    const float midAmp = std::abs(buf.getSample(0, 255));
    check(midAmp > 0.25f, "Dry preview should reach full level after ~5ms fade-in");

    proc->stopDryPreview();
    float maxJump = 0.0f;
    float prev = midAmp;
    for (int b = 0; b < 2; ++b)
    {
        proc->processBlock(buf, midi);
        for (int i = 0; i < 256; ++i)
        {
            const float cur = std::abs(buf.getSample(0, i));
            maxJump = juce::jmax(maxJump, std::abs(cur - prev));
            prev = cur;
        }
    }
    check(maxJump < 0.05f, ("Dry preview stop must fade out without sudden step click, maxJump=" + std::to_string(maxJump)).c_str());
    check(std::abs(buf.getSample(0, 255)) < 0.01f, "Dry preview should complete 5ms fade-out cleanly");

    tmpWav.deleteFile();
}
} // namespace

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    std::cerr << "[TRACE] main enter\n";

    try
    {
        juce::ScopedJuceInitialiser_GUI juceInit;
        std::cerr << "[TRACE] juceInit ok\n";

        testAllFactoryPresetsValid();
        std::cerr << "[TRACE] testAllFactoryPresetsValid ok\n";
        testScaleQuantizerUniqueMenuIds();
        testGetTailLengthSeconds();
        testRandomizePresetGateUnits();
        testMonoBrickLimiterAndModulatedCeiling();
        testAsyncPanicSafety();
        testArpOffAndScaleToggleCleanup();
        testModMatrixSyncRatesAndGuiSnapshot();
        testGrainVisualizerSeqLock();
        testReadAbletonAifBounds();
        testPitchAnalyzerYinMpmAnd24Key();
        testStereoIngestRightOnlyAndMicroRender();
        testScaleQuantizerBoundaryClamp();
        testSubBlockMidiTimingAndWindowRms();
        testButterworth24DbCascadeAndCombThiran();
        testProcessorFootprintAndFxChain192k();
        testModMatrixLorenzChaosLfo();
        testArpeggiatorLatchOffStrumCrossBlockAndEuclidean();
        testMipMapAntiAliasingPitchUp();
        testLookAheadLimiterZeroOvershootAndLatency();
        testClicklessDryPreviewRamp();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 2;
    }
    catch (...)
    {
        std::cerr << "[EXCEPTION] unknown\n";
        return 3;
    }

    if (gFailures > 0)
    {
        std::cerr << "\nTOTAL FAILURES: " << gFailures << "\n";
        return 1;
    }
    std::cerr << "\nALL GRANULAR PHASE 1, 2 & 3 TESTS PASSED!\n";
    return 0;
}
