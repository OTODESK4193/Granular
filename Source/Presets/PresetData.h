// ==========================================
// File: PresetData.h
// ファクトリープリセット 120種（パラメータ上書き方式・ハードコード）
//   各プリセットは「基準デフォルトからの上書き」を持つ。
//   Sampleは含まない（プリセットはノブ/MOD/FX設定のみ。音は各自ロード）。
//   カテゴリ8種 × 各10 = 80 + 新カテゴリ「Pitch」(Tone全開特化) 40 = 計120種。
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
        // A. PADS & AMBIENT (10)
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
            {"grainSize",0.22f},{"density",0.65f},{"pitchSync",1},{"tone",0.55f},{"scaleOn",1},{"scale",1},
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
        // B. TEXTURES (10)
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
            {"grainSize",0.15f},{"density",0.75f},{"pitchSync",1},{"octave",2},{"tone",0.5f},{"scaleOn",1},{"scale",1},
            {"attack",0.5f},{"release",1.8f},{"fx5Type",5},{"fx5Amount",46.f},{"revShimmer",0.9f} }},
        { "Tape Erosion", "Textures", "Grainy", {
            {"grainSize",0.09f},{"density",0.65f},{"positionSpray",0.45f},{"reverseGrain",0},
            {"masterLPF",4000.f},{"fx1Type",1},{"fx1Amount",35.f},{"fx3Type",3},{"fx3Amount",28.f},{"dlyFeedback",0.5f} }},
        { "Spectral Haze", "Textures", "Cloud", {
            {"grainSize",0.14f},{"density",0.88f},{"positionSpray",0.55f},{"pitchJitter",40.f},{"panSpread",1.f},
            {"lfo1Rate",0.25f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.3f},{"fx5Type",5},{"fx5Amount",42.f} }},

        // ============================================================
        // C. RHYTHMIC (10)
        // ============================================================
        { "Pulse Engine", "Rhythmic", "Pulse", {
            {"grainSize",0.08f},{"density",0.6f},{"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.7f},{"mod1Uni",1},{"attack",0.01f},{"release",0.3f} }},
        { "Arp Runner", "Rhythmic", "Arp", {
            {"grainSize",0.1f},{"density",0.5f},{"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.6f},{"scaleOn",1},{"scale",1},{"attack",0.005f},{"decay",0.15f},{"sustain",0.3f},{"release",0.2f},
            {"fx3Type",3},{"fx3Amount",30.f} }},
        { "Stutter Chop", "Rhythmic", "Stutter", {
            {"grainSize",0.05f},{"density",0.7f},{"lfo1Rate",8.f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.8f},{"attack",0.002f},{"release",0.15f} }},
        { "Sequenced Motion", "Rhythmic", "Sequence", {
            {"grainSize",0.12f},{"density",0.55f},{"speedSync",1},{"speedSyncDiv",2},{"playMode",0},
            {"arpEnable",1},{"arpPattern",2},{"arpRateSync",9},{"arpOctaves",1},{"scaleOn",1},{"scale",2},
            {"attack",0.01f},{"release",0.4f} }},
        { "Gate Rider", "Rhythmic", "Pulse", {
            {"grainSize",0.15f},{"density",0.8f},{"fx6Type",6},{"fx6Amount",85.f},{"gateRateSync",12},{"gateWidth",0.5f},
            {"attack",0.01f},{"release",0.3f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Trance Pluck Arp", "Rhythmic", "Arp", {
            {"grainSize",0.07f},{"density",0.5f},{"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.4f},{"scaleOn",1},{"scale",1},{"attack",0.002f},{"decay",0.12f},{"sustain",0.2f},{"release",0.15f},
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
            {"grainSize",0.09f},{"density",0.5f},{"arpEnable",1},{"arpPattern",18},{"arpRateSync",12},{"arpOctaves",3},
            {"arpGate",0.5f},{"scaleOn",1},{"scale",1},{"attack",0.004f},{"decay",0.2f},{"sustain",0.25f},{"release",0.25f} }},
        { "Polyrhythm Grains", "Rhythmic", "Sequence", {
            {"grainSize",0.11f},{"density",0.65f},
            {"lfo1Rate",3.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.5f},
            {"lfo2Rate",2.f},{"lfo2Sync",1},{"lfo2SyncRate",7},{"mod2Src",2},{"mod2Dst",6},{"mod2Amt",0.6f} }},

        // ============================================================
        // D. BASS (10)
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
        // E. LEADS & KEYS (10)
        // ============================================================
        { "Glass Pluck", "Leads & Keys", "Pluck", {
            {"grainSize",0.08f},{"density",0.6f},{"pitchSync",1},{"tone",0.4f},{"scaleOn",1},{"scale",1},
            {"attack",0.002f},{"decay",0.3f},{"sustain",0.2f},{"release",0.4f},
            {"fx3Type",3},{"fx3Amount",25.f} }},
        { "Music Box", "Leads & Keys", "Bell", {
            {"grainSize",0.06f},{"density",0.5f},{"pitchSync",1},{"octave",1},{"tone",0.6f},{"scaleOn",1},{"scale",1},
            {"attack",0.001f},{"decay",0.6f},{"sustain",0.0f},{"release",0.8f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Marimba Grains", "Leads & Keys", "Mallet", {
            {"grainSize",0.05f},{"density",0.55f},{"pitchSync",1},{"tone",0.5f},{"scaleOn",1},{"scale",1},
            {"attack",0.001f},{"decay",0.4f},{"sustain",0.0f},{"release",0.3f} }},
        { "Vox Lead", "Leads & Keys", "Vox", {
            {"grainSize",0.16f},{"density",0.8f},{"pitchSync",1},{"tone",0.55f},{"scaleOn",1},{"scale",1},
            {"attack",0.05f},{"decay",0.2f},{"sustain",0.8f},{"release",0.4f},
            {"fx2Type",2},{"fx2Amount",30.f},{"fx5Type",5},{"fx5Amount",25.f} }},
        { "Bell Choir", "Leads & Keys", "Bell", {
            {"grainSize",0.1f},{"density",0.7f},{"pitchSync",1},{"octave",1},{"tone",0.65f},{"scaleOn",1},{"scale",1},
            {"attack",0.003f},{"decay",0.9f},{"sustain",0.1f},{"release",1.2f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Soft Keys", "Leads & Keys", "Vox", {
            {"grainSize",0.18f},{"density",0.75f},{"pitchSync",1},{"tone",0.35f},
            {"attack",0.02f},{"decay",0.4f},{"sustain",0.7f},{"release",0.5f},{"masterLPF",7000.f} }},
        { "Detuned Lead", "Leads & Keys", "Pluck", {
            {"grainSize",0.09f},{"density",0.7f},{"pitchJitter",20.f},{"panSpread",0.7f},{"scaleOn",1},{"scale",1},
            {"attack",0.004f},{"decay",0.25f},{"sustain",0.5f},{"release",0.3f},
            {"fx2Type",2},{"fx2Amount",35.f},{"fx3Type",3},{"fx3Amount",25.f} }},
        { "Kalimba Dream", "Leads & Keys", "Mallet", {
            {"grainSize",0.055f},{"density",0.5f},{"pitchSync",1},{"tone",0.55f},{"octave",1},{"scaleOn",1},{"scale",4},
            {"attack",0.001f},{"decay",0.5f},{"sustain",0.0f},{"release",0.5f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Ghost Choir", "Leads & Keys", "Vox", {
            {"grainSize",0.2f},{"density",0.82f},{"pitchSync",1},{"tone",0.5f},{"positionSpray",0.2f},
            {"attack",0.4f},{"release",1.5f},{"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.6f} }},
        { "Harp Cascade", "Leads & Keys", "Pluck", {
            {"grainSize",0.07f},{"density",0.6f},{"pitchSync",1},{"scaleOn",1},{"scale",4},{"octave",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",3},{"arpGate",0.7f},
            {"attack",0.002f},{"decay",0.4f},{"sustain",0.1f},{"release",0.6f},{"fx5Type",5},{"fx5Amount",30.f} }},

        // ============================================================
        // F. CINEMATIC (10)
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
        // G. GLITCH & IDM (10)
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
        // H. EXPERIMENTAL (10)
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
            {"grainSize",0.18f},{"density",0.8f},{"pitchSync",1},{"tone",0.6f},{"scaleOn",1},{"scale",6},
            {"lfo1Rate",6.f},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.3f},
            {"fx2Type",2},{"fx2Amount",40.f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Total Collapse", "Experimental", "Chaos", {
            {"grainSize",0.03f},{"density",0.9f},{"playMode",3},{"pitchJitter",800.f},{"positionSpray",1.f},{"panSpread",1.f},
            {"lfo1Rate",8.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",1.f},
            {"lfo2Rate",5.f},{"lfo2Wave",4},{"mod2Src",2},{"mod2Dst",9},{"mod2Amt",0.9f},
            {"fx1Type",1},{"fx1Amount",40.f},{"fx5Type",5},{"fx5Amount",45.f} }},

        // ============================================================
        // I. PITCH (新設: Tone 1.0f 全開 / ピッチ感特化 40選)
        // ============================================================
        { "Reso Pluck C3", "Pitch", "Pluck", {
            {"tone",1.0f},{"grainSize",0.06f},{"density",0.6f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.35f},{"sustain",0.0f},{"release",0.4f},
            {"fx3Type",3},{"fx3Amount",25.f},{"masterHPF",80.f} }},
        { "Crystal Harp", "Pitch", "Pluck", {
            {"tone",1.0f},{"grainSize",0.09f},{"density",0.75f},{"pitchSync",1},{"scaleOn",1},{"octave",1},
            {"attack",0.002f},{"decay",0.6f},{"sustain",0.1f},{"release",0.8f},
            {"fx5Type",5},{"fx5Amount",35.f},{"revShimmer",0.6f} }},
        { "Karplus String", "Pitch", "Pluck", {
            {"tone",1.0f},{"grainSize",0.04f},{"density",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.45f},{"sustain",0.05f},{"release",0.5f},
            {"fx1Type",1},{"fx1Amount",20.f},{"satDrive",3.f} }},
        { "Steel Kalimba", "Pitch", "Pluck", {
            {"tone",1.0f},{"grainSize",0.05f},{"density",0.55f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.5f},{"sustain",0.0f},{"release",0.4f},
            {"fx2Type",2},{"fx2Amount",25.f} }},
        { "Glass Marimba", "Pitch", "Perc", {
            {"tone",1.0f},{"grainSize",0.045f},{"density",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.3f},{"sustain",0.0f},{"release",0.25f},
            {"masterLPF",9000.f},{"fx5Type",5},{"fx5Amount",20.f} }},
        { "Harmonic Bell", "Pitch", "Bell", {
            {"tone",1.0f},{"grainSize",0.12f},{"density",0.65f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",0.002f},{"decay",1.2f},{"sustain",0.15f},{"release",1.5f},
            {"fx5Type",5},{"fx5Amount",40.f},{"revDecay",0.85f} }},
        { "Temple Chime", "Pitch", "Bell", {
            {"tone",1.0f},{"grainSize",0.18f},{"density",0.7f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.005f},{"decay",1.8f},{"sustain",0.1f},{"release",2.0f},
            {"fx3Type",3},{"fx3Amount",30.f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Silver Music Box", "Pitch", "Bell", {
            {"tone",1.0f},{"grainSize",0.05f},{"density",0.45f},{"pitchSync",1},{"octave",2},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.8f},{"sustain",0.0f},{"release",0.9f},
            {"fx5Type",5},{"fx5Amount",32.f},{"revShimmer",0.5f} }},
        { "Glockenspiel Grain", "Pitch", "Bell", {
            {"tone",1.0f},{"grainSize",0.04f},{"density",0.5f},{"pitchSync",1},{"octave",2},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.5f},{"sustain",0.0f},{"release",0.4f},
            {"masterHPF",300.f},{"fx2Type",2},{"fx2Amount",20.f} }},
        { "Tubular Resonance", "Pitch", "Bell", {
            {"tone",1.0f},{"grainSize",0.15f},{"density",0.8f},{"pitchSync",1},{"octave",0},{"scaleOn",1},
            {"attack",0.003f},{"decay",1.5f},{"sustain",0.2f},{"release",1.8f},
            {"fx1Type",1},{"fx1Amount",15.f},{"fx5Type",5},{"fx5Amount",38.f} }},
        { "Singing Glass Lead", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.14f},{"density",0.85f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.04f},{"decay",0.3f},{"sustain",0.85f},{"release",0.5f},
            {"fx2Type",2},{"fx2Amount",35.f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Vocal Formant Lead", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.16f},{"density",0.8f},{"pitchSync",1},{"scaleOn",1},{"spraySpeed",0.1f},
            {"attack",0.03f},{"decay",0.25f},{"sustain",0.8f},{"release",0.45f},
            {"lfo1Rate",5.0f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.25f} }},
        { "Cyber Reed Lead", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.08f},{"density",0.9f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.008f},{"decay",0.2f},{"sustain",0.75f},{"release",0.3f},
            {"fx1Type",1},{"fx1Amount",45.f},{"satDrive",6.f},{"masterLPF",6500.f} }},
        { "Neon Flute", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.12f},{"density",0.75f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",0.06f},{"decay",0.3f},{"sustain",0.8f},{"release",0.4f},
            {"fx3Type",3},{"fx3Amount",28.f},{"dlyFeedback",0.4f} }},
        { "Solo Cello Grain", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.22f},{"density",0.85f},{"pitchSync",1},{"octave",-1},{"scaleOn",1},
            {"attack",0.08f},{"decay",0.4f},{"sustain",0.85f},{"release",0.6f},
            {"masterLPF",4500.f},{"fx2Type",2},{"fx2Amount",25.f} }},
        { "Whistle Synth", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.1f},{"density",0.7f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",0.02f},{"decay",0.2f},{"sustain",0.8f},{"release",0.35f},
            {"fx5Type",5},{"fx5Amount",35.f},{"revShimmer",0.4f} }},
        { "Sine Reso Lead", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.15f},{"density",0.8f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.01f},{"decay",0.2f},{"sustain",0.9f},{"release",0.4f},
            {"masterLPF",5000.f},{"fx5Type",5},{"fx5Amount",25.f} }},
        { "Electric Koto", "Pitch", "Pluck", {
            {"tone",1.0f},{"grainSize",0.07f},{"density",0.65f},{"pitchSync",1},{"scaleOn",1},{"scale",4},
            {"attack",0.002f},{"decay",0.4f},{"sustain",0.0f},{"release",0.45f},
            {"fx1Type",1},{"fx1Amount",30.f},{"panSpread",0.8f} }},
        { "Bowed Metal Lead", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.25f},{"density",0.9f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.15f},{"decay",0.5f},{"sustain",0.9f},{"release",0.8f},
            {"fx4Type",4},{"fx4Amount",35.f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Overdrive Harmonics", "Pitch", "Lead", {
            {"tone",1.0f},{"grainSize",0.09f},{"density",0.92f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.005f},{"decay",0.2f},{"sustain",0.8f},{"release",0.35f},
            {"fx1Type",1},{"fx1Amount",60.f},{"satDrive",8.f},{"masterLPF",5500.f} }},
        { "Ether Choir Pad", "Pitch", "Pad", {
            {"tone",1.0f},{"grainSize",0.32f},{"density",0.85f},{"pitchSync",1},{"scaleOn",1},
            {"attack",1.2f},{"decay",1.5f},{"sustain",0.9f},{"release",2.2f},
            {"fx2Type",2},{"fx2Amount",30.f},{"fx5Type",5},{"fx5Amount",45.f},{"revDecay",0.88f} }},
        { "Resonant Ambient Wash", "Pitch", "Pad", {
            {"tone",1.0f},{"grainSize",0.38f},{"density",0.8f},{"pitchSync",1},{"scaleOn",1},
            {"attack",1.8f},{"decay",2.0f},{"sustain",0.95f},{"release",3.0f},
            {"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.7f} }},
        { "Glass Cathedral", "Pitch", "Pad", {
            {"tone",1.0f},{"grainSize",0.28f},{"density",0.82f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",1.5f},{"decay",1.8f},{"sustain",0.85f},{"release",2.5f},
            {"fx3Type",3},{"fx3Amount",25.f},{"fx5Type",5},{"fx5Amount",48.f} }},
        { "Twilight Strings", "Pitch", "Pad", {
            {"tone",1.0f},{"grainSize",0.35f},{"density",0.88f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.8f},{"decay",1.2f},{"sustain",0.88f},{"release",2.0f},
            {"masterLPF",5500.f},{"fx2Type",2},{"fx2Amount",25.f},{"fx5Type",5},{"fx5Amount",38.f} }},
        { "Frozen Harmonics", "Pitch", "Pad", {
            {"tone",1.0f},{"grainSize",0.26f},{"density",0.78f},{"pitchSync",1},{"pitchJitter",0.f},{"scaleOn",1},
            {"attack",1.0f},{"decay",1.5f},{"sustain",0.9f},{"release",2.4f},
            {"fx4Type",4},{"fx4Amount",30.f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Dark Reso Drone", "Pitch", "Drone", {
            {"tone",1.0f},{"grainSize",0.45f},{"density",0.9f},{"pitchSync",1},{"octave",-2},{"scaleOn",1},
            {"attack",2.0f},{"decay",2.5f},{"sustain",0.95f},{"release",3.5f},
            {"masterLPF",1800.f},{"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Submarine Echo", "Pitch", "Drone", {
            {"tone",1.0f},{"grainSize",0.4f},{"density",0.85f},{"pitchSync",1},{"octave",-1},{"scaleOn",1},
            {"attack",1.5f},{"decay",2.0f},{"sustain",0.9f},{"release",3.0f},
            {"fx3Type",3},{"fx3Amount",40.f},{"dlyFeedback",0.65f},{"masterLPF",2500.f} }},
        { "Solar Flare Drone", "Pitch", "Drone", {
            {"tone",1.0f},{"grainSize",0.38f},{"density",0.82f},{"pitchSync",1},{"scaleOn",1},
            {"attack",2.5f},{"decay",2.0f},{"sustain",0.9f},{"release",3.2f},
            {"lfo1Rate",0.15f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.4f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Reso Bass Pluck", "Pitch", "Bass", {
            {"tone",1.0f},{"grainSize",0.08f},{"density",0.85f},{"pitchSync",1},{"octave",-2},{"scaleOn",1},
            {"attack",0.002f},{"decay",0.4f},{"sustain",0.1f},{"release",0.3f},
            {"masterLPF",2200.f},{"fx1Type",1},{"fx1Amount",25.f} }},
        { "Tuned 808 Resonator", "Pitch", "Bass", {
            {"tone",1.0f},{"grainSize",0.15f},{"density",0.88f},{"pitchSync",1},{"octave",-2},{"scaleOn",1},
            {"attack",0.002f},{"decay",0.9f},{"sustain",0.3f},{"release",0.5f},
            {"masterLPF",1500.f},{"satDrive",4.f},{"fx1Type",1},{"fx1Amount",30.f} }},
        { "Vocaloid Bass", "Pitch", "Bass", {
            {"tone",1.0f},{"grainSize",0.12f},{"density",0.9f},{"pitchSync",1},{"octave",-1},{"scaleOn",1},
            {"attack",0.004f},{"decay",0.35f},{"sustain",0.75f},{"release",0.3f},
            {"lfo1Rate",1.0f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.4f},{"masterLPF",2800.f} }},
        { "Metallic Reese", "Pitch", "Bass", {
            {"tone",1.0f},{"grainSize",0.1f},{"density",0.92f},{"pitchSync",1},{"octave",-1},{"scaleOn",1},
            {"attack",0.005f},{"decay",0.3f},{"sustain",0.8f},{"release",0.35f},
            {"fx1Type",1},{"fx1Amount",40.f},{"fx2Type",2},{"fx2Amount",25.f},{"masterLPF",2400.f} }},
        { "Gate Reso Staccato", "Pitch", "Gate", {
            {"tone",1.0f},{"grainSize",0.12f},{"density",0.85f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.005f},{"decay",0.3f},{"sustain",0.8f},{"release",0.3f},
            {"fx6Type",6},{"fx6Amount",80.f},{"gateRateSync",12},{"gateWidth",0.5f} }},
        { "Pulsing Chime Gate", "Pitch", "Gate", {
            {"tone",1.0f},{"grainSize",0.15f},{"density",0.8f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"attack",0.01f},{"decay",0.4f},{"sustain",0.75f},{"release",0.5f},
            {"fx6Type",6},{"fx6Amount",85.f},{"gateRateSync",12},{"gateWidth",0.4f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Arp Reso Walker", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.06f},{"density",0.6f},{"pitchSync",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",22},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.5f},
            {"attack",0.002f},{"decay",0.25f},{"sustain",0.1f},{"release",0.25f},{"fx3Type",3},{"fx3Amount",25.f} }},
        { "Strumming Resonator", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.07f},{"density",0.65f},{"pitchSync",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",20},{"arpRateSync",9},{"arpOctaves",1},{"arpGate",0.6f},
            {"attack",0.003f},{"decay",0.35f},{"sustain",0.1f},{"release",0.35f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Pentatonic Rain", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.05f},{"density",0.55f},{"pitchSync",1},{"octave",1},{"scaleOn",1},{"scale",4},
            {"arpEnable",1},{"arpPattern",4},{"arpRateSync",14},{"arpOctaves",3},{"arpGate",0.45f},
            {"attack",0.001f},{"decay",0.3f},{"sustain",0.0f},{"release",0.3f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Inward Tone Spiral", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.08f},{"density",0.7f},{"pitchSync",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",16},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.55f},
            {"attack",0.002f},{"decay",0.28f},{"sustain",0.15f},{"release",0.3f},{"fx2Type",2},{"fx2Amount",20.f} }},
        { "Double Step Lead", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.09f},{"density",0.75f},{"pitchSync",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",18},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.6f},
            {"attack",0.003f},{"decay",0.25f},{"sustain",0.2f},{"release",0.25f},{"fx1Type",1},{"fx1Amount",25.f} }},
        { "Celestial Quantize", "Pitch", "Arp", {
            {"tone",1.0f},{"grainSize",0.14f},{"density",0.8f},{"pitchSync",1},{"octave",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.7f},
            {"attack",0.005f},{"decay",0.5f},{"sustain",0.25f},{"release",0.7f},
            {"fx5Type",5},{"fx5Amount",45.f},{"revShimmer",0.8f} }},
        };
        return P;
    }
}
