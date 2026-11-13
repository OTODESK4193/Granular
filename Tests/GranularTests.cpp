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

    float monoVal = 1.0f;
    float aliasedVal = 1.0f;
    limA.processMono(monoVal, 0.5f);
    limB.process(aliasedVal, aliasedVal, 0.5f);
    check(std::abs(monoVal - 0.5f) < 1.0e-4f, "Mono limiter should clamp 1.0 to 0.5");
    check(std::abs(aliasedVal - monoVal) < 1.0e-5f, "Aliased L==R process() should match processMono() with no double attenuation");

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
    std::cerr << "\nALL GRANULAR PHASE 1 TESTS PASSED!\n";
    return 0;
}
