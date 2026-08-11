// ==========================================
// File: PresetData.h
// ファクトリープリセット 80種（パラメータ上書き方式・ハードコード）
//   各プリセットは「基準デフォルトからの上書き」を持つ。
//   Sampleは含まない（プリセットはノブ/MOD/FX設定のみ。音は各自ロード）。
//   カテゴリ8種 × 各10 = 80。オーソドックス〜奇抜まで音楽的に。
// ==========================================
#pragma once

#include <JuceHeader.h>
#include <vector>

namespace GranularPresets
{
    struct PParam { const char* id; float v; };
    struct Preset
    {
        juce::String name, category, sub;
        std::vector<PParam> params;
    };

    // 値はパラメータの「実単位」（適用時に convertTo0to1 + クランプ）。
    // choice/int は該当インデックス。bool は 0/1。
    inline const std::vector<Preset>& all()
    {
        static const std::vector<Preset> P = {
        // ============================================================
        // A. PADS & AMBIENT
        // ============================================================
        { "Warm Cathedral", "Pads & Ambient", "Warm", {
            {"grainSize",0.34f},{"density",0.85f},{"positionSpray",0.18f},{"panSpread",0.9f},
            {"attack",1.8f},{"decay",2.0f},{"sustain",0.9f},{"release",3.0f},
            {"tone",0.25f},{"masterLPF",8000.f},{"fx5Type",5},{"fx5Amount",45.f},{"revDecay",0.85f},{"fxMix",1.f} }},
        { "Glass Choir", "Pads & Ambient", "Glass", {
            {"grainSize",0.28f},{"density",0.8f},{"positionSpray",0.12f},{"pitchSync",1},{"tone",0.45f},
            {"attack",1.2f},{"release",2.4f},{"octave",1},{"masterHPF",120.f},
            {"fx2Type",2},{"fx2Amount",40.f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Slow Bloom", "Pads & Ambient", "Evolving", {
            {"grainSize",0.4f},{"density",0.7f},{"positionSpray",0.35f},{"speed",0.25f},
            {"attack",3.0f},{"release",4.0f},{"sustain",1.f},
            {"lfo1Rate",0.15f},{"lfo1Wave",0},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.4f},
            {"fx5Type",5},{"fx5Amount",50.f},{"revDecay",0.9f} }},
        { "Dark Vespers", "Pads & Ambient", "Dark", {
            {"grainSize",0.45f},{"density",0.9f},{"octave",-1},{"tone",0.15f},
            {"masterLPF",3000.f},{"attack",2.2f},{"release",3.5f},
            {"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",55.f},{"revDecay",0.92f} }},
        { "Aurora Wash", "Pads & Ambient", "Evolving", {
            {"grainSize",0.3f},{"density",0.82f},{"positionSpray",0.4f},{"panSpread",1.f},
            {"lfo1Rate",0.2f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.3f},{"mod1Uni",1},
            {"attack",2.5f},{"release",3.2f},{"fx5Type",5},{"fx5Amount",48.f},{"revShimmer",0.5f} }},
        { "Velvet Drift", "Pads & Ambient", "Warm", {
            {"grainSize",0.36f},{"density",0.75f},{"positionSpray",0.22f},{"speed",0.4f},
            {"tone",0.3f},{"masterLPF",6000.f},{"attack",1.6f},{"release",2.8f},
            {"fx2Type",2},{"fx2Amount",30.f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Frozen Lake", "Pads & Ambient", "Glass", {
            {"grainSize",0.26f},{"density",0.7f},{"pitchJitter",30.f},{"octave",1},
            {"attack",2.0f},{"release",3.0f},{"masterHPF",200.f},
            {"fx4Type",4},{"fx4Amount",30.f},{"fx5Type",5},{"fx5Amount",45.f},{"revShimmer",0.6f} }},
        { "Distant Bells Pad", "Pads & Ambient", "Glass", {
            {"grainSize",0.22f},{"density",0.65f},{"pitchSync",1},{"tone",0.55f},{"scale",1},
            {"attack",1.0f},{"release",2.6f},{"fx5Type",5},{"fx5Amount",42.f},{"revDecay",0.8f} }},
        { "Undertow", "Pads & Ambient", "Dark", {
            {"grainSize",0.42f},{"density",0.88f},{"octave",-2},{"speed",0.3f},{"tone",0.1f},
            {"masterLPF",2200.f},{"attack",2.8f},{"release",3.8f},
            {"lfo1Rate",0.1f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.5f},{"fx5Type",5},{"fx5Amount",50.f} }},
        { "Breathing Room", "Pads & Ambient", "Evolving", {
            {"grainSize",0.32f},{"density",0.78f},{"positionSpray",0.3f},
            {"env1Attack",2.0f},{"env1Decay",2.0f},{"env1Loop",1},{"mod1Src",5},{"mod1Dst",2},{"mod1Amt",0.4f},
            {"attack",1.8f},{"release",3.0f},{"fx5Type",5},{"fx5Amount",44.f} }},

        // ============================================================
        // B. TEXTURES
        // ============================================================
        { "Grain Cloud", "Textures", "Cloud", {
            {"grainSize",0.12f},{"density",0.9f},{"positionSpray",0.6f},{"pitchJitter",80.f},{"panSpread",1.f},
            {"attack",0.8f},{"release",1.5f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Dust Motes", "Textures", "Grainy", {
            {"grainSize",0.06f},{"density",0.5f},{"positionSpray",0.8f},{"pitchJitter",120.f},{"panSpread",1.f},
            {"speed",0.2f},{"attack",0.4f},{"release",1.0f},{"fx2Type",2},{"fx2Amount",25.f} }},
        { "Static Field", "Textures", "Noise", {
            {"grainSize",0.04f},{"density",0.95f},{"positionSpray",0.9f},{"pitchJitter",400.f},
            {"masterHPF",300.f},{"fx1Type",1},{"fx1Amount",20.f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Shimmer Veil", "Textures", "Shimmer", {
            {"grainSize",0.18f},{"density",0.85f},{"positionSpray",0.4f},{"octave",1},{"pitchJitter",25.f},
            {"attack",1.0f},{"release",2.0f},{"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.8f} }},
        { "Particle Stream", "Textures", "Cloud", {
            {"grainSize",0.1f},{"density",0.8f},{"positionSpray",0.5f},{"speed",0.6f},{"panSpread",0.9f},
            {"lfo1Rate",0.3f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.6f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Frostbite", "Textures", "Grainy", {
            {"grainSize",0.08f},{"density",0.7f},{"pitchJitter",200.f},{"octave",2},
            {"masterHPF",500.f},{"attack",0.3f},{"release",1.2f},{"fx5Type",5},{"fx5Amount",40.f},{"revShimmer",0.5f} }},
        { "Whisper Grains", "Textures", "Noise", {
            {"grainSize",0.05f},{"density",0.4f},{"positionSpray",0.7f},{"pitchJitter",60.f},{"tone",0.2f},
            {"masterLPF",5000.f},{"attack",0.6f},{"release",1.4f},{"fx2Type",2},{"fx2Amount",30.f} }},
        { "Crystalline", "Textures", "Shimmer", {
            {"grainSize",0.15f},{"density",0.75f},{"pitchSync",1},{"octave",2},{"tone",0.5f},{"scale",1},
            {"attack",0.5f},{"release",1.8f},{"fx5Type",5},{"fx5Amount",46.f},{"revShimmer",0.9f} }},
        { "Tape Erosion", "Textures", "Grainy", {
            {"grainSize",0.09f},{"density",0.65f},{"positionSpray",0.45f},{"reverseGrain",0},
            {"masterLPF",4000.f},{"fx1Type",1},{"fx1Amount",35.f},{"fx3Type",3},{"fx3Amount",28.f},{"dlyFeedback",0.5f} }},
        { "Spectral Haze", "Textures", "Cloud", {
            {"grainSize",0.14f},{"density",0.88f},{"positionSpray",0.55f},{"pitchJitter",40.f},{"panSpread",1.f},
            {"lfo1Rate",0.25f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.3f},{"fx5Type",5},{"fx5Amount",42.f} }},

        // ============================================================
        // C. RHYTHMIC
        // ============================================================
        { "Pulse Engine", "Rhythmic", "Pulse", {
            {"grainSize",0.08f},{"density",0.6f},{"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.7f},{"mod1Uni",1},{"attack",0.01f},{"release",0.3f} }},
        { "Arp Runner", "Rhythmic", "Arp", {
            {"grainSize",0.1f},{"density",0.5f},{"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.6f},{"scale",1},{"attack",0.005f},{"decay",0.15f},{"sustain",0.3f},{"release",0.2f},
            {"fx3Type",3},{"fx3Amount",30.f} }},
        { "Stutter Chop", "Rhythmic", "Stutter", {
            {"grainSize",0.05f},{"density",0.7f},{"lfo1Rate",8.f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.8f},{"attack",0.002f},{"release",0.15f} }},
        { "Sequenced Motion", "Rhythmic", "Sequence", {
            {"grainSize",0.12f},{"density",0.55f},{"speedSync",1},{"speedSyncDiv",2},{"playMode",0},
            {"arpEnable",1},{"arpPattern",2},{"arpRateSync",9},{"arpOctaves",1},{"scale",2},
            {"attack",0.01f},{"release",0.4f} }},
        { "Gate Rider", "Rhythmic", "Pulse", {
            {"grainSize",0.15f},{"density",0.8f},{"lfo1Rate",2.f},{"lfo1Sync",1},{"lfo1SyncRate",6},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",19},{"mod1Amt",-0.9f},{"mod1Uni",1},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Trance Pluck Arp", "Rhythmic", "Arp", {
            {"grainSize",0.07f},{"density",0.5f},{"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.4f},{"scale",1},{"attack",0.002f},{"decay",0.12f},{"sustain",0.2f},{"release",0.15f},
            {"masterHPF",150.f},{"fx3Type",3},{"fx3Amount",35.f},{"dlyFeedback",0.55f} }},
        { "Broken Clock", "Rhythmic", "Stutter", {
            {"grainSize",0.06f},{"density",0.6f},{"playMode",3},{"positionSpray",0.3f},
            {"lfo1Rate",6.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.6f},{"attack",0.003f},{"release",0.2f} }},
        { "Motorik", "Rhythmic", "Pulse", {
            {"grainSize",0.1f},{"density",0.7f},{"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",1},
            {"mod1Src",1},{"mod1Dst",7},{"mod1Amt",0.3f},{"attack",0.01f},{"release",0.25f},
            {"fx1Type",1},{"fx1Amount",20.f} }},
        { "Ratchet Keys", "Rhythmic", "Sequence", {
            {"grainSize",0.09f},{"density",0.5f},{"arpEnable",1},{"arpPattern",4},{"arpRateSync",12},{"arpOctaves",3},
            {"arpGate",0.5f},{"scale",1},{"attack",0.004f},{"decay",0.2f},{"sustain",0.25f},{"release",0.25f} }},
        { "Polyrhythm Grains", "Rhythmic", "Sequence", {
            {"grainSize",0.11f},{"density",0.65f},
            {"lfo1Rate",3.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.5f},
            {"lfo2Rate",2.f},{"lfo2Sync",1},{"lfo2SyncRate",7},{"mod2Src",2},{"mod2Dst",6},{"mod2Amt",0.6f} }},

        // ============================================================
        // D. BASS
        // ============================================================
        { "Sub Foundation", "Bass", "Sub", {
            {"grainSize",0.2f},{"density",0.9f},{"octave",-2},{"pitchSync",1},{"tone",0.2f},
            {"masterLPF",1200.f},{"attack",0.005f},{"decay",0.3f},{"sustain",0.8f},{"release",0.3f} }},
        { "Movement Bass", "Bass", "Movement", {
            {"grainSize",0.15f},{"density",0.85f},{"octave",-1},{"tone",0.3f},
            {"lfo1Rate",1.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.4f},{"mod1Uni",1},
            {"masterLPF",2000.f},{"attack",0.005f},{"decay",0.25f},{"sustain",0.7f},{"release",0.2f} }},
        { "Growl Machine", "Bass", "Gritty", {
            {"grainSize",0.1f},{"density",0.9f},{"octave",-1},{"pitchJitter",30.f},
            {"fx1Type",1},{"fx1Amount",55.f},{"satDrive",6.f},{"masterLPF",3000.f},
            {"lfo1Rate",5.f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.5f},
            {"attack",0.003f},{"decay",0.2f},{"sustain",0.75f},{"release",0.15f} }},
        { "Reese Grains", "Bass", "Gritty", {
            {"grainSize",0.12f},{"density",0.95f},{"octave",-1},{"pitchJitter",15.f},{"panSpread",0.6f},
            {"fx1Type",1},{"fx1Amount",35.f},{"fx2Type",2},{"fx2Amount",20.f},{"masterLPF",2500.f},
            {"attack",0.004f},{"sustain",0.8f},{"release",0.2f} }},
        { "Deep Pulse Bass", "Bass", "Movement", {
            {"grainSize",0.16f},{"density",0.88f},{"octave",-2},
            {"lfo1Rate",2.f},{"lfo1Sync",1},{"lfo1SyncRate",6},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",19},{"mod1Amt",-0.5f},{"mod1Uni",1},
            {"masterLPF",1500.f},{"attack",0.005f},{"decay",0.2f},{"sustain",0.85f},{"release",0.2f} }},
        { "Wobble Grain", "Bass", "Movement", {
            {"grainSize",0.13f},{"density",0.9f},{"octave",-1},
            {"lfo1Rate",1.5f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",0},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.6f},{"mod1Uni",1},
            {"masterLPF",2200.f},{"attack",0.004f},{"sustain",0.8f},{"release",0.18f} }},
        { "Round Sub", "Bass", "Sub", {
            {"grainSize",0.22f},{"density",0.85f},{"octave",-2},{"pitchSync",1},{"tone",0.15f},
            {"masterLPF",900.f},{"masterHPF",30.f},{"attack",0.008f},{"sustain",0.9f},{"release",0.25f} }},
        { "Distorted Depths", "Bass", "Gritty", {
            {"grainSize",0.09f},{"density",0.92f},{"octave",-2},{"pitchJitter",50.f},
            {"fx1Type",1},{"fx1Amount",70.f},{"satDrive",9.f},{"masterLPF",1800.f},
            {"attack",0.003f},{"sustain",0.8f},{"release",0.15f} }},
        { "Formant Bass", "Bass", "Movement", {
            {"grainSize",0.14f},{"density",0.88f},{"octave",-1},{"tone",0.4f},{"pitchSync",1},
            {"lfo1Rate",0.5f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.5f},{"mod1Uni",1},
            {"attack",0.005f},{"sustain",0.8f},{"release",0.2f} }},
        { "Granular 808", "Bass", "Sub", {
            {"grainSize",0.25f},{"density",0.8f},{"octave",-2},{"pitchSync",1},
            {"fx1Type",1},{"fx1Amount",25.f},{"masterLPF",1400.f},
            {"attack",0.002f},{"decay",0.8f},{"sustain",0.4f},{"release",0.6f} }},

        // ============================================================
        // E. LEADS & KEYS
        // ============================================================
        { "Glass Pluck", "Leads & Keys", "Pluck", {
            {"grainSize",0.08f},{"density",0.6f},{"pitchSync",1},{"tone",0.4f},{"scale",1},
            {"attack",0.002f},{"decay",0.3f},{"sustain",0.2f},{"release",0.4f},
            {"fx3Type",3},{"fx3Amount",25.f} }},
        { "Music Box", "Leads & Keys", "Bell", {
            {"grainSize",0.06f},{"density",0.5f},{"pitchSync",1},{"octave",1},{"tone",0.6f},{"scale",1},
            {"attack",0.001f},{"decay",0.6f},{"sustain",0.0f},{"release",0.8f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Marimba Grains", "Leads & Keys", "Mallet", {
            {"grainSize",0.05f},{"density",0.55f},{"pitchSync",1},{"tone",0.5f},{"scale",1},
            {"attack",0.001f},{"decay",0.4f},{"sustain",0.0f},{"release",0.3f} }},
        { "Vox Lead", "Leads & Keys", "Vox", {
            {"grainSize",0.16f},{"density",0.8f},{"pitchSync",1},{"tone",0.55f},{"scale",1},
            {"attack",0.05f},{"decay",0.2f},{"sustain",0.8f},{"release",0.4f},
            {"fx2Type",2},{"fx2Amount",30.f},{"fx5Type",5},{"fx5Amount",25.f} }},
        { "Bell Choir", "Leads & Keys", "Bell", {
            {"grainSize",0.1f},{"density",0.7f},{"pitchSync",1},{"octave",1},{"tone",0.65f},{"scale",1},
            {"attack",0.003f},{"decay",0.9f},{"sustain",0.1f},{"release",1.2f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Soft Keys", "Leads & Keys", "Vox", {
            {"grainSize",0.18f},{"density",0.75f},{"pitchSync",1},{"tone",0.35f},
            {"attack",0.02f},{"decay",0.4f},{"sustain",0.7f},{"release",0.5f},{"masterLPF",7000.f} }},
        { "Detuned Lead", "Leads & Keys", "Pluck", {
            {"grainSize",0.09f},{"density",0.7f},{"pitchJitter",20.f},{"panSpread",0.7f},{"scale",1},
            {"attack",0.004f},{"decay",0.25f},{"sustain",0.5f},{"release",0.3f},
            {"fx2Type",2},{"fx2Amount",35.f},{"fx3Type",3},{"fx3Amount",25.f} }},
        { "Kalimba Dream", "Leads & Keys", "Mallet", {
            {"grainSize",0.055f},{"density",0.5f},{"pitchSync",1},{"tone",0.55f},{"octave",1},{"scale",4},
            {"attack",0.001f},{"decay",0.5f},{"sustain",0.0f},{"release",0.5f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Ghost Choir", "Leads & Keys", "Vox", {
            {"grainSize",0.2f},{"density",0.82f},{"pitchSync",1},{"tone",0.5f},{"positionSpray",0.2f},
            {"attack",0.4f},{"release",1.5f},{"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.6f} }},
        { "Harp Cascade", "Leads & Keys", "Pluck", {
            {"grainSize",0.07f},{"density",0.6f},{"pitchSync",1},{"scale",4},{"octave",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",3},{"arpGate",0.7f},
            {"attack",0.002f},{"decay",0.4f},{"sustain",0.1f},{"release",0.6f},{"fx5Type",5},{"fx5Amount",30.f} }},

        // ============================================================
        // F. CINEMATIC
        // ============================================================
        { "Rising Tension", "Cinematic", "Riser", {
            {"grainSize",0.14f},{"density",0.85f},{"positionSpray",0.4f},
            {"env1Attack",6.f},{"env1Decay",0.5f},{"mod1Src",5},{"mod1Dst",9},{"mod1Amt",0.9f},{"mod1Uni",1},
            {"mod2Src",5},{"mod2Dst",8},{"mod2Amt",0.6f},{"attack",0.5f},{"release",1.f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Impact Bloom", "Cinematic", "Impact", {
            {"grainSize",0.3f},{"density",0.95f},{"octave",-1},{"pitchJitter",100.f},
            {"attack",0.001f},{"decay",1.5f},{"sustain",0.2f},{"release",2.5f},
            {"fx1Type",1},{"fx1Amount",40.f},{"fx5Type",5},{"fx5Amount",60.f},{"revDecay",0.95f} }},
        { "Deep Drone", "Cinematic", "Drone", {
            {"grainSize",0.48f},{"density",0.92f},{"octave",-2},{"tone",0.1f},{"speed",0.15f},
            {"masterLPF",2000.f},{"attack",3.f},{"release",4.f},
            {"lfo1Rate",0.08f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.4f},{"fx5Type",5},{"fx5Amount",55.f} }},
        { "Underscore Pad", "Cinematic", "Underscore", {
            {"grainSize",0.35f},{"density",0.8f},{"positionSpray",0.25f},{"tone",0.2f},
            {"attack",2.f},{"release",3.f},{"masterLPF",5000.f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Doom Riser", "Cinematic", "Riser", {
            {"grainSize",0.1f},{"density",0.9f},{"octave",-1},{"pitchJitter",150.f},
            {"env1Attack",8.f},{"mod1Src",5},{"mod1Dst",5},{"mod1Amt",0.8f},{"mod1Uni",1},
            {"mod2Src",5},{"mod2Dst",3},{"mod2Amt",0.7f},{"fx1Type",1},{"fx1Amount",30.f},{"fx5Type",5},{"fx5Amount",50.f} }},
        { "Sub Impact", "Cinematic", "Impact", {
            {"grainSize",0.28f},{"density",0.9f},{"octave",-2},{"pitchSync",1},
            {"attack",0.001f},{"decay",1.f},{"sustain",0.1f},{"release",1.8f},
            {"fx1Type",1},{"fx1Amount",30.f},{"masterLPF",1600.f} }},
        { "Endless Fall", "Cinematic", "Drone", {
            {"grainSize",0.24f},{"density",0.85f},{"reverseGrain",1},{"speed",0.3f},{"positionSpray",0.5f},
            {"lfo1Rate",0.12f},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",-0.5f},
            {"attack",1.5f},{"release",3.f},{"fx5Type",5},{"fx5Amount",55.f} }},
        { "War Horns", "Cinematic", "Underscore", {
            {"grainSize",0.2f},{"density",0.9f},{"octave",-1},{"tone",0.4f},{"pitchSync",1},
            {"fx1Type",1},{"fx1Amount",30.f},{"attack",0.3f},{"release",1.2f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Cryo Chamber", "Cinematic", "Drone", {
            {"grainSize",0.4f},{"density",0.88f},{"pitchJitter",40.f},{"positionSpray",0.3f},
            {"masterHPF",150.f},{"attack",2.5f},{"release",3.5f},
            {"fx4Type",4},{"fx4Amount",35.f},{"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.5f} }},
        { "Signal Lost", "Cinematic", "Riser", {
            {"grainSize",0.06f},{"density",0.7f},{"positionSpray",0.6f},{"pitchJitter",300.f},
            {"env1Attack",5.f},{"env1Loop",0},{"mod1Src",5},{"mod1Dst",13},{"mod1Amt",0.9f},{"mod1Uni",1},
            {"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",45.f} }},

        // ============================================================
        // G. GLITCH & IDM
        // ============================================================
        { "Data Corrupt", "Glitch & IDM", "Broken", {
            {"grainSize",0.04f},{"density",0.6f},{"playMode",3},{"positionSpray",0.7f},{"pitchJitter",200.f},
            {"lfo1Rate",7.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.7f},
            {"attack",0.002f},{"release",0.2f} }},
        { "Micro Chop", "Glitch & IDM", "Chop", {
            {"grainSize",0.03f},{"density",0.7f},{"lfo1Rate",12.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.9f},{"attack",0.001f},{"release",0.1f} }},
        { "Random Access", "Glitch & IDM", "Random", {
            {"grainSize",0.05f},{"density",0.65f},{"playMode",3},{"pitchJitter",100.f},
            {"mod1Src",11},{"mod1Dst",1},{"mod1Amt",0.9f},{"mod2Src",11},{"mod2Dst",8},{"mod2Amt",0.5f},
            {"attack",0.002f},{"release",0.25f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Buffer Error", "Glitch & IDM", "Broken", {
            {"grainSize",0.035f},{"density",0.55f},{"reverseGrain",1},{"positionSpray",0.5f},
            {"lfo1Rate",9.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",7},{"mod1Amt",0.8f},
            {"fx4Type",4},{"fx4Amount",40.f} }},
        { "Bit Rattle", "Glitch & IDM", "Chop", {
            {"grainSize",0.045f},{"density",0.75f},{"pitchJitter",50.f},
            {"lfo1Rate",8.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.8f},
            {"fx1Type",1},{"fx1Amount",30.f},{"attack",0.001f},{"release",0.12f} }},
        { "Neural Noise", "Glitch & IDM", "Random", {
            {"grainSize",0.04f},{"density",0.6f},{"playMode",3},{"positionSpray",0.8f},{"pitchJitter",400.f},
            {"mod1Src",11},{"mod1Dst",9},{"mod1Amt",0.7f},{"mod2Src",11},{"mod2Dst",6},{"mod2Amt",1.f},
            {"masterHPF",200.f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Skip Groove", "Glitch & IDM", "Chop", {
            {"grainSize",0.06f},{"density",0.7f},{"arpEnable",1},{"arpPattern",10},{"arpRateSync",14},{"arpOctaves",2},
            {"lfo1Rate",6.f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.5f},
            {"attack",0.002f},{"release",0.15f} }},
        { "Fractal Grains", "Glitch & IDM", "Random", {
            {"grainSize",0.05f},{"density",0.7f},{"pitchJitter",120.f},
            {"lfo1Rate",5.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.6f},
            {"lfo2Rate",3.f},{"lfo2Wave",5},{"mod2Src",2},{"mod2Dst",8},{"mod2Amt",0.5f},{"fx3Type",3},{"fx3Amount",28.f} }},
        { "Clock Divider", "Glitch & IDM", "Broken", {
            {"grainSize",0.07f},{"density",0.65f},
            {"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",19},{"mod1Amt",-0.9f},{"mod1Uni",1},
            {"fx4Type",4},{"fx4Amount",35.f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Granular Stutter", "Glitch & IDM", "Chop", {
            {"grainSize",0.03f},{"density",0.8f},{"lfo1Rate",10.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",1.f},{"attack",0.001f},{"release",0.08f},{"fx5Type",5},{"fx5Amount",25.f} }},

        // ============================================================
        // H. EXPERIMENTAL
        // ============================================================
        { "Chaos Engine", "Experimental", "Chaos", {
            {"grainSize",0.08f},{"density",0.7f},{"pitchJitter",300.f},{"positionSpray",0.6f},{"panSpread",1.f},
            {"lfo1Rate",3.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.8f},
            {"lfo2Rate",7.f},{"lfo2Wave",5},{"mod2Src",2},{"mod2Dst",8},{"mod2Amt",0.7f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Alien Broadcast", "Experimental", "FX", {
            {"grainSize",0.05f},{"density",0.6f},{"octave",1},{"pitchJitter",500.f},
            {"lfo1Rate",2.f},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.9f},{"mod1Uni",1},
            {"fx1Type",1},{"fx1Amount",30.f},{"fx3Type",3},{"fx3Amount",40.f},{"dlyFeedback",0.7f} }},
        { "Mobius Loop", "Experimental", "Wild", {
            {"grainSize",0.12f},{"density",0.8f},{"reverseGrain",1},{"speed",0.5f},
            {"env1Attack",2.f},{"env1Decay",2.f},{"env1Loop",1},{"mod1Src",5},{"mod1Dst",1},{"mod1Amt",0.7f},
            {"env2Attack",1.f},{"env2Decay",3.f},{"env2Loop",1},{"mod2Src",6},{"mod2Dst",8},{"mod2Amt",0.5f},
            {"fx5Type",5},{"fx5Amount",45.f} }},
        { "Quantum Foam", "Experimental", "Chaos", {
            {"grainSize",0.04f},{"density",0.85f},{"positionSpray",0.9f},{"pitchJitter",250.f},{"panSpread",1.f},
            {"mod1Src",11},{"mod1Dst",2},{"mod1Amt",0.8f},{"mod2Src",11},{"mod2Dst",1},{"mod2Amt",1.f},
            {"fx4Type",4},{"fx4Amount",40.f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Time Stretch Ghost", "Experimental", "Wild", {
            {"grainSize",0.3f},{"density",0.9f},{"speed",0.1f},{"positionSpray",0.4f},{"pitchJitter",50.f},
            {"attack",1.f},{"release",2.5f},{"fx4Type",4},{"fx4Amount",50.f},{"fx5Type",5},{"fx5Amount",50.f} }},
        { "Metallic Resonator", "Experimental", "FX", {
            {"grainSize",0.06f},{"density",0.7f},{"pitchSync",1},{"tone",0.8f},{"octave",1},
            {"lfo1Rate",0.5f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.6f},{"mod1Uni",1},
            {"fx5Type",5},{"fx5Amount",45.f},{"revShimmer",0.7f} }},
        { "Insect Swarm", "Experimental", "Chaos", {
            {"grainSize",0.02f},{"density",0.95f},{"positionSpray",0.8f},{"pitchJitter",600.f},{"panSpread",1.f},
            {"lfo1Rate",9.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",0.4f},
            {"masterHPF",400.f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Reverse Cathedral", "Experimental", "Wild", {
            {"grainSize",0.35f},{"density",0.85f},{"reverseGrain",1},{"positionSpray",0.3f},{"tone",0.3f},
            {"attack",2.f},{"release",3.f},{"fx5Type",5},{"fx5Amount",60.f},{"revDecay",0.95f},{"revShimmer",0.5f} }},
        { "Dial Tone Choir", "Experimental", "FX", {
            {"grainSize",0.18f},{"density",0.8f},{"pitchSync",1},{"tone",0.6f},{"scale",6},
            {"lfo1Rate",6.f},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.3f},
            {"fx2Type",2},{"fx2Amount",40.f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Total Collapse", "Experimental", "Chaos", {
            {"grainSize",0.03f},{"density",0.9f},{"playMode",3},{"pitchJitter",800.f},{"positionSpray",1.f},{"panSpread",1.f},
            {"lfo1Rate",8.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",1.f},
            {"lfo2Rate",5.f},{"lfo2Wave",4},{"mod2Src",2},{"mod2Dst",9},{"mod2Amt",0.9f},
            {"fx1Type",1},{"fx1Amount",40.f},{"fx5Type",5},{"fx5Amount",45.f} }},
        
        // ============================================================
        // EXTRA: A. PADS & AMBIENT
        // ============================================================
        { "Pads Exp 1", "Pads & Ambient", "Warm", { {"grainSize",0.1f},{"density",0.93f},{"positionSpray",0.73f},{"pitchJitter",247.2f},{"attack",0.05f},{"release",2.67f},{"fx5Type",2},{"fx5Amount",86.1f} } },
        { "Pads Exp 2", "Pads & Ambient", "Dark", { {"grainSize",0.29f},{"density",0.45f},{"positionSpray",0.0f},{"pitchJitter",169.7f},{"attack",2.31f},{"release",3.08f},{"lfo1Rate",2.37f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.05f},{"fx5Type",1},{"fx5Amount",62.1f} } },
        { "Pads Exp 3", "Pads & Ambient", "Glass", { {"grainSize",0.18f},{"density",0.88f},{"positionSpray",0.7f},{"pitchJitter",204.0f},{"attack",0.54f},{"release",0.55f},{"fx5Type",4},{"fx5Amount",28.9f} } },
        { "Pads Exp 4", "Pads & Ambient", "Glass", { {"grainSize",0.32f},{"density",0.47f},{"positionSpray",0.66f},{"attack",1.14f},{"release",1.46f},{"fx5Type",1},{"fx5Amount",91.3f} } },
        { "Pads Exp 5", "Pads & Ambient", "Dark", { {"grainSize",0.2f},{"density",0.84f},{"positionSpray",0.72f},{"attack",1.92f},{"release",2.98f},{"lfo1Rate",6.85f},{"mod1Src",1},{"mod1Dst",4},{"mod1Amt",0.61f} } },
        { "Pads Exp 6", "Pads & Ambient", "Evolving", { {"grainSize",0.29f},{"density",0.39f},{"positionSpray",0.47f},{"pitchJitter",324.3f},{"attack",2.98f},{"release",3.19f} } },
        { "Pads Exp 7", "Pads & Ambient", "Glass", { {"grainSize",0.05f},{"density",0.25f},{"positionSpray",0.36f},{"pitchJitter",686.1f},{"attack",0.51f},{"release",0.24f},{"lfo1Rate",9.76f},{"lfo1Sync",1},{"lfo1SyncRate",4},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.35f},{"fx5Type",3},{"fx5Amount",51.2f} } },
        { "Pads Exp 8", "Pads & Ambient", "Dark", { {"grainSize",0.23f},{"density",0.92f},{"positionSpray",0.78f},{"pitchJitter",509.4f},{"attack",2.23f},{"release",0.19f},{"lfo1Rate",1.55f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.78f} } },
        { "Pads Exp 9", "Pads & Ambient", "Evolving", { {"grainSize",0.12f},{"density",0.35f},{"positionSpray",0.76f},{"pitchJitter",824.3f},{"attack",1.19f},{"release",3.47f},{"fx5Type",3},{"fx5Amount",32.5f} } },
        { "Pads Exp 10", "Pads & Ambient", "Warm", { {"grainSize",0.32f},{"density",0.92f},{"positionSpray",0.56f},{"attack",0.93f},{"release",1.39f},{"lfo1Rate",9.14f},{"mod1Src",1},{"mod1Dst",4},{"mod1Amt",-0.88f},{"fx5Type",5},{"fx5Amount",30.2f},{"revDecay",0.74f},{"revShimmer",0.2f} } },
        // ============================================================
        // EXTRA: B. TEXTURES
        // ============================================================
        { "Textures Exp 11", "Textures", "Harsh", { {"grainSize",0.27f},{"density",0.35f},{"positionSpray",0.14f},{"pitchJitter",1110.0f},{"attack",0.14f},{"release",1.47f},{"fx5Type",2},{"fx5Amount",68.4f} } },
        { "Textures Exp 12", "Textures", "Harsh", { {"grainSize",0.45f},{"density",0.86f},{"positionSpray",0.28f},{"pitchJitter",778.2f},{"attack",0.85f},{"release",1.05f} } },
        { "Textures Exp 13", "Textures", "Harsh", { {"grainSize",0.38f},{"density",0.97f},{"positionSpray",0.41f},{"attack",1.04f},{"release",3.86f},{"lfo1Rate",4.12f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.75f},{"fx5Type",1},{"fx5Amount",32.0f} } },
        { "Textures Exp 14", "Textures", "Granular", { {"grainSize",0.17f},{"density",0.36f},{"positionSpray",0.05f},{"pitchJitter",591.0f},{"attack",0.02f},{"release",1.86f},{"lfo1Rate",8.51f},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",-0.44f},{"fx5Type",2},{"fx5Amount",25.7f} } },
        { "Textures Exp 15", "Textures", "Harsh", { {"grainSize",0.37f},{"density",0.77f},{"positionSpray",0.59f},{"pitchJitter",102.1f},{"attack",0.8f},{"release",2.76f},{"lfo1Rate",2.91f},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",0.8f},{"fx5Type",5},{"fx5Amount",92.4f},{"revDecay",0.59f},{"revShimmer",0.92f} } },
        { "Textures Exp 16", "Textures", "Organic", { {"grainSize",0.38f},{"density",0.72f},{"positionSpray",0.24f},{"pitchJitter",825.8f},{"attack",1.71f},{"release",0.97f},{"fx5Type",3},{"fx5Amount",94.6f} } },
        { "Textures Exp 17", "Textures", "Granular", { {"grainSize",0.33f},{"density",0.53f},{"positionSpray",0.66f},{"pitchJitter",933.0f},{"attack",2.51f},{"release",1.98f},{"fx5Type",2},{"fx5Amount",23.8f} } },
        { "Textures Exp 18", "Textures", "Harsh", { {"grainSize",0.05f},{"density",0.14f},{"positionSpray",0.71f},{"attack",0.02f},{"release",3.93f},{"lfo1Rate",9.53f},{"mod1Src",1},{"mod1Dst",14},{"mod1Amt",0.98f},{"fx5Type",2},{"fx5Amount",91.3f} } },
        { "Textures Exp 19", "Textures", "Granular", { {"grainSize",0.15f},{"density",0.6f},{"positionSpray",0.38f},{"pitchJitter",859.0f},{"attack",2.33f},{"release",3.25f},{"lfo1Rate",8.95f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",-0.2f},{"fx5Type",4},{"fx5Amount",64.3f} } },
        { "Textures Exp 20", "Textures", "Harsh", { {"grainSize",0.38f},{"density",0.37f},{"positionSpray",0.79f},{"attack",2.25f},{"release",1.6f},{"fx5Type",3},{"fx5Amount",57.7f} } },
        // ============================================================
        // EXTRA: C. RHYTHMIC
        // ============================================================
        { "Rhythmic Exp 21", "Rhythmic", "Glitch", { {"grainSize",0.18f},{"density",0.99f},{"positionSpray",0.79f},{"pitchJitter",1142.8f},{"attack",0.45f},{"release",2.52f},{"lfo1Rate",1.54f},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.54f} } },
        { "Rhythmic Exp 22", "Rhythmic", "Glitch", { {"grainSize",0.39f},{"density",0.64f},{"positionSpray",0.41f},{"attack",1.2f},{"release",0.81f},{"fx5Type",1},{"fx5Amount",32.3f} } },
        { "Rhythmic Exp 23", "Rhythmic", "Beat", { {"grainSize",0.36f},{"density",0.5f},{"positionSpray",0.27f},{"attack",2.93f},{"release",2.22f} } },
        { "Rhythmic Exp 24", "Rhythmic", "Pulse", { {"grainSize",0.34f},{"density",0.27f},{"positionSpray",0.18f},{"attack",0.65f},{"release",2.37f},{"fx5Type",2},{"fx5Amount",82.2f} } },
        { "Rhythmic Exp 25", "Rhythmic", "Pulse", { {"grainSize",0.09f},{"density",0.18f},{"positionSpray",0.71f},{"pitchJitter",1036.6f},{"attack",1.49f},{"release",0.58f} } },
        { "Rhythmic Exp 26", "Rhythmic", "Pulse", { {"grainSize",0.14f},{"density",0.46f},{"positionSpray",0.54f},{"pitchJitter",479.9f},{"attack",1.24f},{"release",3.58f},{"lfo1Rate",5.91f},{"lfo1Sync",1},{"lfo1SyncRate",5},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",-0.38f},{"fx5Type",2},{"fx5Amount",34.6f} } },
        { "Rhythmic Exp 27", "Rhythmic", "Pulse", { {"grainSize",0.27f},{"density",0.76f},{"positionSpray",0.16f},{"attack",1.87f},{"release",3.76f},{"lfo1Rate",9.33f},{"lfo1Sync",1},{"lfo1SyncRate",6},{"mod1Src",1},{"mod1Dst",4},{"mod1Amt",-0.98f},{"fx5Type",3},{"fx5Amount",92.7f} } },
        { "Rhythmic Exp 28", "Rhythmic", "Glitch", { {"grainSize",0.42f},{"density",0.47f},{"positionSpray",0.45f},{"attack",0.65f},{"release",1.34f},{"lfo1Rate",0.11f},{"lfo1Sync",1},{"lfo1SyncRate",8},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.38f},{"fx5Type",5},{"fx5Amount",25.5f},{"revDecay",0.81f},{"revShimmer",0.61f} } },
        { "Rhythmic Exp 29", "Rhythmic", "Beat", { {"grainSize",0.31f},{"density",0.73f},{"positionSpray",0.04f},{"attack",1.6f},{"release",3.79f},{"lfo1Rate",3.48f},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",-0.64f},{"fx5Type",2},{"fx5Amount",79.6f} } },
        { "Rhythmic Exp 30", "Rhythmic", "Glitch", { {"grainSize",0.32f},{"density",0.77f},{"positionSpray",0.13f},{"attack",2.8f},{"release",3.36f},{"lfo1Rate",8.46f},{"mod1Src",1},{"mod1Dst",7},{"mod1Amt",0.42f} } },
        // ============================================================
        // EXTRA: D. BASS
        // ============================================================
        { "Bass Exp 31", "Bass", "Deep", { {"grainSize",0.04f},{"density",0.12f},{"positionSpray",0.65f},{"octave",-2},{"attack",1.98f},{"release",1.62f},{"lfo1Rate",1.94f},{"lfo1Sync",1},{"lfo1SyncRate",5},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.08f},{"fx5Type",3},{"fx5Amount",22.1f} } },
        { "Bass Exp 32", "Bass", "Gritty", { {"grainSize",0.06f},{"density",0.42f},{"positionSpray",0.41f},{"octave",-2},{"attack",0.89f},{"release",1.58f},{"lfo1Rate",1.96f},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",0.08f} } },
        { "Bass Exp 33", "Bass", "Deep", { {"grainSize",0.05f},{"density",0.15f},{"positionSpray",0.76f},{"octave",-2},{"attack",1.01f},{"release",2.38f},{"fx5Type",2},{"fx5Amount",46.1f} } },
        { "Bass Exp 34", "Bass", "Reese", { {"grainSize",0.35f},{"density",0.33f},{"positionSpray",0.56f},{"pitchJitter",709.7f},{"octave",-2},{"attack",0.48f},{"release",0.24f},{"fx5Type",5},{"fx5Amount",59.0f},{"revDecay",0.57f},{"revShimmer",0.91f} } },
        { "Bass Exp 35", "Bass", "Deep", { {"grainSize",0.4f},{"density",0.97f},{"positionSpray",0.6f},{"octave",-2},{"attack",1.83f},{"release",1.47f},{"lfo1Rate",8.23f},{"lfo1Sync",1},{"lfo1SyncRate",10},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",0.5f},{"fx5Type",2},{"fx5Amount",60.5f} } },
        { "Bass Exp 36", "Bass", "Sub", { {"grainSize",0.24f},{"density",0.87f},{"positionSpray",0.43f},{"octave",-1},{"attack",0.15f},{"release",1.9f},{"lfo1Rate",9.36f},{"lfo1Sync",1},{"lfo1SyncRate",3},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.43f},{"fx5Type",5},{"fx5Amount",23.0f},{"revDecay",0.89f},{"revShimmer",0.68f} } },
        { "Bass Exp 37", "Bass", "Deep", { {"grainSize",0.24f},{"density",0.61f},{"positionSpray",0.25f},{"octave",-2},{"attack",1.5f},{"release",2.02f},{"lfo1Rate",6.23f},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.28f} } },
        { "Bass Exp 38", "Bass", "Reese", { {"grainSize",0.45f},{"density",0.76f},{"positionSpray",0.32f},{"octave",-1},{"attack",0.96f},{"release",2.05f},{"lfo1Rate",1.58f},{"lfo1Sync",1},{"lfo1SyncRate",5},{"mod1Src",1},{"mod1Dst",4},{"mod1Amt",0.95f},{"fx5Type",3},{"fx5Amount",20.2f} } },
        { "Bass Exp 39", "Bass", "Deep", { {"grainSize",0.17f},{"density",0.42f},{"positionSpray",0.18f},{"octave",-1},{"attack",1.78f},{"release",3.53f},{"lfo1Rate",7.49f},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",0.1f},{"fx5Type",4},{"fx5Amount",75.8f} } },
        { "Bass Exp 40", "Bass", "Reese", { {"grainSize",0.22f},{"density",0.28f},{"positionSpray",0.31f},{"octave",-2},{"attack",2.45f},{"release",0.86f} } },
        // ============================================================
        // EXTRA: E. LEADS & KEYS
        // ============================================================
        { "Leads Exp 41", "Leads & Keys", "Synth", { {"grainSize",0.17f},{"density",0.55f},{"positionSpray",0.05f},{"pitchJitter",700.0f},{"octave",1},{"attack",1.19f},{"release",1.26f},{"lfo1Rate",6.93f},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",-0.77f} } },
        { "Leads Exp 42", "Leads & Keys", "Synth", { {"grainSize",0.37f},{"density",0.18f},{"positionSpray",0.02f},{"octave",0},{"attack",1.72f},{"release",0.4f},{"lfo1Rate",6.54f},{"lfo1Sync",1},{"lfo1SyncRate",4},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",-0.87f},{"fx5Type",5},{"fx5Amount",40.4f},{"revDecay",0.67f},{"revShimmer",0.59f} } },
        { "Leads Exp 43", "Leads & Keys", "Synth", { {"grainSize",0.2f},{"density",0.88f},{"positionSpray",0.18f},{"pitchJitter",412.3f},{"octave",1},{"attack",2.87f},{"release",2.33f},{"fx5Type",2},{"fx5Amount",74.8f} } },
        { "Leads Exp 44", "Leads & Keys", "Pluck", { {"grainSize",0.1f},{"density",0.7f},{"positionSpray",0.66f},{"pitchJitter",961.6f},{"octave",0},{"attack",0.92f},{"release",0.45f},{"lfo1Rate",0.66f},{"lfo1Sync",1},{"lfo1SyncRate",8},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.13f},{"fx5Type",1},{"fx5Amount",22.9f} } },
        { "Leads Exp 45", "Leads & Keys", "Bells", { {"grainSize",0.17f},{"density",0.33f},{"positionSpray",0.06f},{"octave",0},{"attack",0.49f},{"release",0.27f},{"lfo1Rate",4.17f},{"lfo1Sync",1},{"lfo1SyncRate",3},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",-0.99f},{"fx5Type",1},{"fx5Amount",55.8f} } },
        { "Leads Exp 46", "Leads & Keys", "Synth", { {"grainSize",0.37f},{"density",0.43f},{"positionSpray",0.01f},{"octave",0},{"attack",2.61f},{"release",2.3f},{"lfo1Rate",4.63f},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.93f},{"fx5Type",4},{"fx5Amount",59.9f} } },
        { "Leads Exp 47", "Leads & Keys", "Synth", { {"grainSize",0.16f},{"density",0.69f},{"positionSpray",0.59f},{"octave",1},{"attack",2.13f},{"release",0.77f},{"lfo1Rate",6.36f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.7f},{"fx5Type",1},{"fx5Amount",55.1f} } },
        { "Leads Exp 48", "Leads & Keys", "Bells", { {"grainSize",0.14f},{"density",0.29f},{"positionSpray",0.51f},{"pitchJitter",981.5f},{"octave",0},{"attack",3.0f},{"release",0.42f},{"lfo1Rate",7.48f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",-0.8f},{"fx5Type",1},{"fx5Amount",54.4f} } },
        { "Leads Exp 49", "Leads & Keys", "Pluck", { {"grainSize",0.39f},{"density",0.37f},{"positionSpray",0.48f},{"pitchJitter",187.7f},{"octave",0},{"attack",2.1f},{"release",1.77f},{"lfo1Rate",8.86f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.13f},{"fx5Type",1},{"fx5Amount",46.1f} } },
        { "Leads Exp 50", "Leads & Keys", "Synth", { {"grainSize",0.1f},{"density",0.51f},{"positionSpray",0.07f},{"octave",1},{"attack",0.38f},{"release",0.64f},{"lfo1Rate",9.71f},{"mod1Src",1},{"mod1Dst",14},{"mod1Amt",0.48f} } },
        // ============================================================
        // EXTRA: F. CINEMATIC
        // ============================================================
        { "Cinematic Exp 51", "Cinematic", "Drone", { {"grainSize",0.07f},{"density",0.29f},{"positionSpray",0.8f},{"pitchJitter",346.4f},{"attack",1.14f},{"release",3.29f},{"lfo1Rate",1.65f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",-0.05f},{"fx5Type",5},{"fx5Amount",71.5f},{"revDecay",0.86f},{"revShimmer",0.17f} } },
        { "Cinematic Exp 52", "Cinematic", "Sweep", { {"grainSize",0.11f},{"density",0.36f},{"positionSpray",0.7f},{"attack",1.5f},{"release",3.88f},{"lfo1Rate",1.57f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.08f},{"fx5Type",4},{"fx5Amount",91.8f} } },
        { "Cinematic Exp 53", "Cinematic", "Drone", { {"grainSize",0.38f},{"density",0.16f},{"positionSpray",0.62f},{"pitchJitter",624.3f},{"attack",0.05f},{"release",1.97f},{"lfo1Rate",9.27f},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.5f},{"fx5Type",1},{"fx5Amount",43.6f} } },
        { "Cinematic Exp 54", "Cinematic", "Sweep", { {"grainSize",0.1f},{"density",0.71f},{"positionSpray",0.44f},{"attack",0.3f},{"release",1.77f},{"fx5Type",5},{"fx5Amount",65.3f},{"revDecay",0.93f},{"revShimmer",0.24f} } },
        { "Cinematic Exp 55", "Cinematic", "Sweep", { {"grainSize",0.05f},{"density",0.51f},{"positionSpray",0.1f},{"attack",1.59f},{"release",2.94f},{"lfo1Rate",1.33f},{"lfo1Sync",1},{"lfo1SyncRate",8},{"mod1Src",1},{"mod1Dst",2},{"mod1Amt",1.0f},{"fx5Type",3},{"fx5Amount",54.6f} } },
        { "Cinematic Exp 56", "Cinematic", "Drone", { {"grainSize",0.45f},{"density",0.66f},{"positionSpray",0.55f},{"pitchJitter",1150.7f},{"attack",0.06f},{"release",2.46f},{"lfo1Rate",8.6f},{"lfo1Sync",1},{"lfo1SyncRate",6},{"mod1Src",1},{"mod1Dst",14},{"mod1Amt",0.12f},{"fx5Type",3},{"fx5Amount",66.7f} } },
        { "Cinematic Exp 57", "Cinematic", "Tension", { {"grainSize",0.42f},{"density",0.6f},{"positionSpray",0.6f},{"pitchJitter",190.9f},{"attack",1.42f},{"release",1.42f},{"lfo1Rate",1.69f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.28f},{"fx5Type",3},{"fx5Amount",75.9f} } },
        { "Cinematic Exp 58", "Cinematic", "Drone", { {"grainSize",0.34f},{"density",0.49f},{"positionSpray",0.11f},{"pitchJitter",292.0f},{"attack",2.93f},{"release",0.72f},{"lfo1Rate",8.01f},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.54f} } },
        { "Cinematic Exp 59", "Cinematic", "Impact", { {"grainSize",0.02f},{"density",0.91f},{"positionSpray",0.2f},{"pitchJitter",1023.1f},{"attack",0.37f},{"release",1.44f},{"lfo1Rate",4.42f},{"lfo1Sync",1},{"lfo1SyncRate",11},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.57f},{"fx5Type",4},{"fx5Amount",91.4f} } },
        { "Cinematic Exp 60", "Cinematic", "Tension", { {"grainSize",0.2f},{"density",0.33f},{"positionSpray",0.37f},{"attack",0.19f},{"release",2.48f},{"fx5Type",5},{"fx5Amount",55.4f},{"revDecay",0.62f},{"revShimmer",0.48f} } },
        // ============================================================
        // EXTRA: G. GLITCH & IDM
        // ============================================================
        { "Glitch Exp 61", "Glitch & IDM", "Chaos", { {"grainSize",0.04f},{"density",0.56f},{"positionSpray",0.17f},{"pitchJitter",79.3f},{"attack",2.51f},{"release",0.57f} } },
        { "Glitch Exp 62", "Glitch & IDM", "Digital", { {"grainSize",0.39f},{"density",0.94f},{"positionSpray",0.38f},{"attack",0.89f},{"release",1.05f},{"lfo1Rate",1.3f},{"lfo1Sync",1},{"lfo1SyncRate",3},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",-0.91f} } },
        { "Glitch Exp 63", "Glitch & IDM", "Digital", { {"grainSize",0.04f},{"density",0.25f},{"positionSpray",0.4f},{"attack",2.82f},{"release",2.58f},{"fx5Type",4},{"fx5Amount",61.2f} } },
        { "Glitch Exp 64", "Glitch & IDM", "Chaos", { {"grainSize",0.35f},{"density",0.91f},{"positionSpray",0.34f},{"pitchJitter",128.9f},{"attack",1.59f},{"release",0.7f},{"lfo1Rate",7.55f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",-0.09f},{"fx5Type",2},{"fx5Amount",69.1f} } },
        { "Glitch Exp 65", "Glitch & IDM", "Digital", { {"grainSize",0.36f},{"density",0.81f},{"positionSpray",0.36f},{"pitchJitter",1019.2f},{"attack",0.66f},{"release",0.71f},{"lfo1Rate",0.13f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",-0.23f},{"fx5Type",3},{"fx5Amount",31.2f} } },
        { "Glitch Exp 66", "Glitch & IDM", "Random", { {"grainSize",0.21f},{"density",0.72f},{"positionSpray",0.2f},{"attack",2.48f},{"release",2.41f},{"lfo1Rate",0.3f},{"lfo1Sync",1},{"lfo1SyncRate",3},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",-0.55f},{"fx5Type",1},{"fx5Amount",90.6f} } },
        { "Glitch Exp 67", "Glitch & IDM", "Stutter", { {"grainSize",0.43f},{"density",0.9f},{"positionSpray",0.8f},{"attack",2.23f},{"release",0.13f} } },
        { "Glitch Exp 68", "Glitch & IDM", "Digital", { {"grainSize",0.43f},{"density",0.79f},{"positionSpray",0.08f},{"attack",2.92f},{"release",3.69f},{"lfo1Rate",6.21f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",14},{"mod1Amt",0.98f} } },
        { "Glitch Exp 69", "Glitch & IDM", "Random", { {"grainSize",0.34f},{"density",0.82f},{"positionSpray",0.21f},{"attack",2.72f},{"release",2.7f},{"lfo1Rate",3.41f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.96f},{"fx5Type",4},{"fx5Amount",89.0f} } },
        { "Glitch Exp 70", "Glitch & IDM", "Digital", { {"grainSize",0.14f},{"density",0.81f},{"positionSpray",0.47f},{"attack",2.08f},{"release",1.64f},{"lfo1Rate",7.14f},{"lfo1Sync",1},{"lfo1SyncRate",7},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.74f},{"fx5Type",4},{"fx5Amount",72.8f} } },
        // ============================================================
        // EXTRA: H. EXPERIMENTAL
        // ============================================================
        { "Experimental Exp 71", "Experimental", "Audio-Rate", { {"grainSize",0.19f},{"density",0.6f},{"positionSpray",0.27f},{"pitchJitter",104.9f},{"attack",0.4f},{"release",0.58f},{"lfo1Rate",1.93f},{"lfo1Sync",1},{"lfo1SyncRate",4},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",-0.16f},{"fx5Type",1},{"fx5Amount",97.3f} } },
        { "Experimental Exp 72", "Experimental", "Alien", { {"grainSize",0.1f},{"density",0.7f},{"positionSpray",0.52f},{"pitchJitter",1089.3f},{"attack",0.47f},{"release",2.78f},{"lfo1Rate",4.9f},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",0.54f},{"fx5Type",3},{"fx5Amount",30.6f} } },
        { "Experimental Exp 73", "Experimental", "Alien", { {"grainSize",0.26f},{"density",0.2f},{"positionSpray",0.09f},{"attack",0.61f},{"release",2.87f},{"lfo1Rate",2.25f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",-0.17f} } },
        { "Experimental Exp 74", "Experimental", "Alien", { {"grainSize",0.36f},{"density",0.64f},{"positionSpray",0.69f},{"attack",2.83f},{"release",1.3f},{"lfo1Rate",4.52f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.73f},{"fx5Type",3},{"fx5Amount",95.5f} } },
        { "Experimental Exp 75", "Experimental", "Extreme", { {"grainSize",0.27f},{"density",0.78f},{"positionSpray",0.74f},{"attack",1.47f},{"release",3.62f},{"fx5Type",1},{"fx5Amount",57.7f} } },
        { "Experimental Exp 76", "Experimental", "Unpredictable", { {"grainSize",0.08f},{"density",0.14f},{"positionSpray",0.06f},{"pitchJitter",445.6f},{"attack",2.68f},{"release",3.97f},{"lfo1Rate",8.67f},{"lfo1Sync",1},{"lfo1SyncRate",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.04f} } },
        { "Experimental Exp 77", "Experimental", "Extreme", { {"grainSize",0.43f},{"density",0.59f},{"positionSpray",0.47f},{"attack",1.27f},{"release",1.71f},{"lfo1Rate",6.74f},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",-0.49f},{"fx5Type",1},{"fx5Amount",80.8f} } },
        { "Experimental Exp 78", "Experimental", "Audio-Rate", { {"grainSize",0.08f},{"density",0.11f},{"positionSpray",0.46f},{"attack",0.58f},{"release",0.86f},{"lfo1Rate",5.33f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.8f},{"fx5Type",5},{"fx5Amount",98.6f},{"revDecay",0.66f},{"revShimmer",0.59f} } },
        { "Experimental Exp 79", "Experimental", "Unpredictable", { {"grainSize",0.4f},{"density",0.11f},{"positionSpray",0.13f},{"pitchJitter",278.5f},{"attack",0.45f},{"release",2.67f},{"lfo1Rate",3.63f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",-0.69f} } },
        { "Experimental Exp 80", "Experimental", "Extreme", { {"grainSize",0.14f},{"density",0.28f},{"positionSpray",0.08f},{"attack",0.93f},{"release",1.14f},{"lfo1Rate",5.33f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",-0.74f},{"fx5Type",4},{"fx5Amount",54.6f} } },
        };
        return P;
    }
}
