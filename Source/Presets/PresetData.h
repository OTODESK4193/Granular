// ==========================================
// File: PresetData.h
// ファクトリープリセット 120種（パラメータ上書き方式・ハードコード）
//   各プリセットは「基準デフォルトからの上書き」を持つ。
//   Sampleは含まない（プリセットはノブ/MOD/FX設定のみ。音は各自ロード）。
//   既存80種（Pads, Textures, Rhythmic, Bass, Leads, Cinematic, Glitch, Experimental）
//     はピッチ感・音楽性を大幅に底上げ（Tone付加、P-Sync、ScaleQuantize活用）。
//   新設40種は独立カテゴリ「Pitch」（Tone共鳴×グラニュラー多彩化特化、サブカテゴリにPluck, Bell, Lead, Pad, Bass, Arp, Gate, Drone等）。
//   合計120種。
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
        // A. PADS & AMBIENT (10) - ピッチ感強化版
        // ============================================================
        { "Warm Cathedral", "Pads & Ambient", "Warm", {
            {"grainSize",0.34f},{"density",0.85f},{"positionSpray",0.12f},{"panSpread",0.9f},
            {"attack",1.8f},{"decay",2.0f},{"sustain",0.9f},{"release",3.0f},
            {"tone",0.45f},{"pitchSync",1},{"scaleOn",1},{"masterLPF",8500.f},
            {"fx5Type",5},{"fx5Amount",45.f},{"revDecay",0.85f},{"fxMix",1.f} }},
        { "Glass Choir", "Pads & Ambient", "Glass", {
            {"grainSize",0.28f},{"density",0.8f},{"positionSpray",0.08f},{"pitchSync",1},{"tone",0.6f},
            {"attack",1.2f},{"release",2.4f},{"octave",1},{"pitchJitter",10.f},{"scaleOn",1},{"masterHPF",120.f},
            {"fx2Type",2},{"fx2Amount",35.f},{"fx5Type",5},{"fx5Amount",38.f} }},
        { "Slow Bloom", "Pads & Ambient", "Evolving", {
            {"grainSize",0.38f},{"density",0.75f},{"positionSpray",0.18f},{"speed",0.25f},
            {"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"attack",2.5f},{"release",3.8f},{"sustain",1.f},
            {"lfo1Rate",0.15f},{"lfo1Wave",0},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.3f},
            {"fx5Type",5},{"fx5Amount",48.f},{"revDecay",0.88f} }},
        { "Dark Vespers", "Pads & Ambient", "Dark", {
            {"grainSize",0.42f},{"density",0.9f},{"octave",-1},{"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"masterLPF",3500.f},{"attack",2.0f},{"release",3.2f},
            {"fx1Type",1},{"fx1Amount",20.f},{"fx5Type",5},{"fx5Amount",50.f},{"revDecay",0.9f} }},
        { "Aurora Wash", "Pads & Ambient", "Evolving", {
            {"grainSize",0.3f},{"density",0.82f},{"positionSpray",0.2f},{"panSpread",1.f},
            {"tone",0.48f},{"pitchSync",1},{"scaleOn",1},{"pitchJitter",15.f},
            {"lfo1Rate",0.2f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.25f},{"mod1Uni",1},
            {"attack",2.2f},{"release",3.0f},{"fx5Type",5},{"fx5Amount",45.f},{"revShimmer",0.55f} }},
        { "Velvet Drift", "Pads & Ambient", "Warm", {
            {"grainSize",0.34f},{"density",0.78f},{"positionSpray",0.15f},{"speed",0.4f},
            {"tone",0.45f},{"pitchSync",1},{"scaleOn",1},{"masterLPF",6500.f},{"attack",1.5f},{"release",2.6f},
            {"fx2Type",2},{"fx2Amount",28.f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Frozen Lake", "Pads & Ambient", "Glass", {
            {"grainSize",0.26f},{"density",0.75f},{"pitchJitter",10.f},{"octave",1},
            {"tone",0.55f},{"pitchSync",1},{"scaleOn",1},
            {"attack",1.8f},{"release",2.8f},{"masterHPF",180.f},
            {"fx4Type",4},{"fx4Amount",28.f},{"fx5Type",5},{"fx5Amount",42.f},{"revShimmer",0.65f} }},
        { "Distant Bells Pad", "Pads & Ambient", "Glass", {
            {"grainSize",0.22f},{"density",0.7f},{"pitchSync",1},{"tone",0.65f},{"scaleOn",1},{"scale",1},
            {"attack",0.8f},{"release",2.4f},{"fx5Type",5},{"fx5Amount",44.f},{"revDecay",0.82f} }},
        { "Undertow", "Pads & Ambient", "Dark", {
            {"grainSize",0.4f},{"density",0.88f},{"octave",-2},{"speed",0.3f},{"tone",0.38f},{"pitchSync",1},{"scaleOn",1},
            {"masterLPF",2500.f},{"attack",2.5f},{"release",3.5f},
            {"lfo1Rate",0.1f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.35f},{"fx5Type",5},{"fx5Amount",48.f} }},
        { "Breathing Room", "Pads & Ambient", "Evolving", {
            {"grainSize",0.32f},{"density",0.8f},{"positionSpray",0.18f},
            {"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"env1Attack",1.8f},{"env1Decay",1.8f},{"env1Loop",1},{"mod1Src",5},{"mod1Dst",2},{"mod1Amt",0.35f},
            {"attack",1.6f},{"release",2.8f},{"fx5Type",5},{"fx5Amount",42.f} }},

        // ============================================================
        // B. TEXTURES (10) - ピッチ感強化版
        // ============================================================
        { "Grain Cloud", "Textures", "Cloud", {
            {"grainSize",0.14f},{"density",0.88f},{"positionSpray",0.35f},{"pitchJitter",25.f},{"panSpread",0.9f},
            {"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.6f},{"release",1.6f},{"fx5Type",5},{"fx5Amount",38.f} }},
        { "Dust Motes", "Textures", "Grainy", {
            {"grainSize",0.08f},{"density",0.65f},{"positionSpray",0.4f},{"pitchJitter",30.f},{"panSpread",0.8f},
            {"tone",0.38f},{"pitchSync",1},{"scaleOn",1},
            {"speed",0.3f},{"attack",0.3f},{"release",1.2f},{"fx2Type",2},{"fx2Amount",25.f} }},
        { "Static Field", "Textures", "Noise", {
            {"grainSize",0.06f},{"density",0.85f},{"positionSpray",0.5f},{"pitchJitter",40.f},
            {"tone",0.42f},{"scaleOn",1},
            {"masterHPF",250.f},{"fx1Type",1},{"fx1Amount",20.f},{"fx5Type",5},{"fx5Amount",32.f} }},
        { "Shimmer Veil", "Textures", "Shimmer", {
            {"grainSize",0.18f},{"density",0.85f},{"positionSpray",0.25f},{"octave",1},{"pitchJitter",15.f},
            {"tone",0.52f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.8f},{"release",2.0f},{"fx5Type",5},{"fx5Amount",48.f},{"revShimmer",0.75f} }},
        { "Particle Stream", "Textures", "Cloud", {
            {"grainSize",0.12f},{"density",0.82f},{"positionSpray",0.3f},{"speed",0.5f},{"panSpread",0.85f},
            {"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",0.3f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.4f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Frostbite", "Textures", "Grainy", {
            {"grainSize",0.1f},{"density",0.75f},{"pitchJitter",20.f},{"octave",2},
            {"tone",0.58f},{"pitchSync",1},{"scaleOn",1},
            {"masterHPF",450.f},{"attack",0.25f},{"release",1.4f},{"fx5Type",5},{"fx5Amount",42.f},{"revShimmer",0.55f} }},
        { "Whisper Grains", "Textures", "Noise", {
            {"grainSize",0.08f},{"density",0.55f},{"positionSpray",0.35f},{"pitchJitter",18.f},{"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"masterLPF",5500.f},{"attack",0.5f},{"release",1.5f},{"fx2Type",2},{"fx2Amount",30.f} }},
        { "Crystalline", "Textures", "Shimmer", {
            {"grainSize",0.15f},{"density",0.78f},{"pitchSync",1},{"octave",2},{"tone",0.68f},{"scaleOn",1},{"scale",1},
            {"attack",0.4f},{"release",1.8f},{"fx5Type",5},{"fx5Amount",46.f},{"revShimmer",0.85f} }},
        { "Tape Erosion", "Textures", "Grainy", {
            {"grainSize",0.11f},{"density",0.7f},{"positionSpray",0.25f},{"pitchJitter",20.f},{"tone",0.42f},{"scaleOn",1},
            {"masterLPF",4500.f},{"fx1Type",1},{"fx1Amount",30.f},{"fx3Type",3},{"fx3Amount",28.f},{"dlyFeedback",0.48f} }},
        { "Spectral Haze", "Textures", "Cloud", {
            {"grainSize",0.16f},{"density",0.88f},{"positionSpray",0.3f},{"pitchJitter",15.f},{"panSpread",0.9f},
            {"tone",0.46f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",0.25f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.25f},{"fx5Type",5},{"fx5Amount",44.f} }},

        // ============================================================
        // C. RHYTHMIC (10) - ピッチ感強化版
        // ============================================================
        { "Pulse Engine", "Rhythmic", "Pulse", {
            {"grainSize",0.1f},{"density",0.7f},{"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.6f},{"mod1Uni",1},{"attack",0.01f},{"release",0.3f} }},
        { "Arp Runner", "Rhythmic", "Arp", {
            {"grainSize",0.1f},{"density",0.6f},{"tone",0.52f},{"pitchSync",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.6f},{"scaleOn",1},{"scale",1},{"attack",0.005f},{"decay",0.18f},{"sustain",0.3f},{"release",0.25f},
            {"fx3Type",3},{"fx3Amount",30.f} }},
        { "Stutter Chop", "Rhythmic", "Stutter", {
            {"grainSize",0.08f},{"density",0.75f},{"tone",0.45f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",8.f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.65f},{"attack",0.002f},{"release",0.18f} }},
        { "Sequenced Motion", "Rhythmic", "Sequence", {
            {"grainSize",0.12f},{"density",0.65f},{"speedSync",1},{"speedSyncDiv",2},{"playMode",0},
            {"tone",0.48f},{"pitchSync",1},
            {"arpEnable",1},{"arpPattern",2},{"arpRateSync",9},{"arpOctaves",1},{"scaleOn",1},{"scale",2},
            {"attack",0.01f},{"release",0.4f} }},
        { "Gate Rider", "Rhythmic", "Pulse", {
            {"grainSize",0.16f},{"density",0.82f},{"tone",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"fx6Type",6},{"fx6Amount",85.f},{"gateRateSync",12},{"gateWidth",0.5f},
            {"attack",0.01f},{"release",0.35f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Trance Pluck Arp", "Rhythmic", "Arp", {
            {"grainSize",0.08f},{"density",0.6f},{"tone",0.62f},{"pitchSync",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",2},
            {"arpGate",0.45f},{"scaleOn",1},{"scale",1},{"attack",0.002f},{"decay",0.15f},{"sustain",0.2f},{"release",0.2f},
            {"masterHPF",150.f},{"fx3Type",3},{"fx3Amount",35.f},{"dlyFeedback",0.52f} }},
        { "Broken Clock", "Rhythmic", "Stutter", {
            {"grainSize",0.08f},{"density",0.65f},{"playMode",3},{"positionSpray",0.2f},{"tone",0.42f},{"scaleOn",1},
            {"lfo1Rate",6.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.5f},{"attack",0.003f},{"release",0.2f} }},
        { "Motorik", "Rhythmic", "Pulse", {
            {"grainSize",0.12f},{"density",0.75f},{"tone",0.48f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",1},
            {"mod1Src",1},{"mod1Dst",7},{"mod1Amt",0.3f},{"attack",0.01f},{"release",0.28f},
            {"fx1Type",1},{"fx1Amount",20.f} }},
        { "Ratchet Keys", "Rhythmic", "Sequence", {
            {"grainSize",0.1f},{"density",0.6f},{"tone",0.56f},{"pitchSync",1},
            {"arpEnable",1},{"arpPattern",18},{"arpRateSync",12},{"arpOctaves",3},
            {"arpGate",0.5f},{"scaleOn",1},{"scale",1},{"attack",0.004f},{"decay",0.22f},{"sustain",0.25f},{"release",0.25f} }},
        { "Polyrhythm Grains", "Rhythmic", "Sequence", {
            {"grainSize",0.12f},{"density",0.7f},{"tone",0.45f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",3.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.45f},
            {"lfo2Rate",2.f},{"lfo2Sync",1},{"lfo2SyncRate",7},{"mod2Src",2},{"mod2Dst",6},{"mod2Amt",0.5f} }},

        // ============================================================
        // D. BASS (10) - ピッチ感強化版
        // ============================================================
        { "Sub Foundation", "Bass", "Sub", {
            {"grainSize",0.22f},{"density",0.9f},{"octave",-2},{"pitchSync",1},{"tone",0.38f},{"scaleOn",1},
            {"masterLPF",1300.f},{"attack",0.005f},{"decay",0.3f},{"sustain",0.85f},{"release",0.3f} }},
        { "Movement Bass", "Bass", "Movement", {
            {"grainSize",0.16f},{"density",0.88f},{"octave",-1},{"tone",0.45f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",1.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.35f},{"mod1Uni",1},
            {"masterLPF",2200.f},{"attack",0.005f},{"decay",0.25f},{"sustain",0.75f},{"release",0.22f} }},
        { "Growl Machine", "Bass", "Gritty", {
            {"grainSize",0.12f},{"density",0.92f},{"octave",-1},{"pitchJitter",12.f},{"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"fx1Type",1},{"fx1Amount",50.f},{"satDrive",5.5f},{"masterLPF",3200.f},
            {"lfo1Rate",5.f},{"mod1Src",1},{"mod1Dst",9},{"mod1Amt",0.4f},
            {"attack",0.003f},{"decay",0.2f},{"sustain",0.75f},{"release",0.18f} }},
        { "Reese Grains", "Bass", "Gritty", {
            {"grainSize",0.14f},{"density",0.95f},{"octave",-1},{"pitchJitter",14.f},{"panSpread",0.55f},{"tone",0.44f},{"pitchSync",1},{"scaleOn",1},
            {"fx1Type",1},{"fx1Amount",30.f},{"fx2Type",2},{"fx2Amount",22.f},{"masterLPF",2700.f},
            {"attack",0.004f},{"sustain",0.82f},{"release",0.22f} }},
        { "Deep Pulse Bass", "Bass", "Movement", {
            {"grainSize",0.18f},{"density",0.9f},{"octave",-2},{"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",2.f},{"lfo1Sync",1},{"lfo1SyncRate",6},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",19},{"mod1Amt",-0.45f},{"mod1Uni",1},
            {"masterLPF",1600.f},{"attack",0.005f},{"decay",0.2f},{"sustain",0.85f},{"release",0.2f} }},
        { "Wobble Grain", "Bass", "Movement", {
            {"grainSize",0.15f},{"density",0.9f},{"octave",-1},{"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",1.5f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",0},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.5f},{"mod1Uni",1},
            {"masterLPF",2400.f},{"attack",0.004f},{"sustain",0.8f},{"release",0.2f} }},
        { "Round Sub", "Bass", "Sub", {
            {"grainSize",0.24f},{"density",0.88f},{"octave",-2},{"pitchSync",1},{"tone",0.32f},{"scaleOn",1},
            {"masterLPF",1000.f},{"masterHPF",30.f},{"attack",0.008f},{"sustain",0.9f},{"release",0.25f} }},
        { "Distorted Depths", "Bass", "Gritty", {
            {"grainSize",0.11f},{"density",0.92f},{"octave",-2},{"pitchJitter",16.f},{"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"fx1Type",1},{"fx1Amount",65.f},{"satDrive",8.f},{"masterLPF",2000.f},
            {"attack",0.003f},{"sustain",0.8f},{"release",0.18f} }},
        { "Formant Bass", "Bass", "Movement", {
            {"grainSize",0.16f},{"density",0.9f},{"octave",-1},{"tone",0.52f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",0.5f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.45f},{"mod1Uni",1},
            {"attack",0.005f},{"sustain",0.82f},{"release",0.22f} }},
        { "Granular 808", "Bass", "Sub", {
            {"grainSize",0.25f},{"density",0.82f},{"octave",-2},{"pitchSync",1},{"tone",0.38f},{"scaleOn",1},
            {"fx1Type",1},{"fx1Amount",25.f},{"masterLPF",1500.f},
            {"attack",0.002f},{"decay",0.8f},{"sustain",0.4f},{"release",0.55f} }},

        // ============================================================
        // E. LEADS & KEYS (10) - ピッチ感強化版
        // ============================================================
        { "Glass Pluck", "Leads & Keys", "Pluck", {
            {"grainSize",0.08f},{"density",0.65f},{"pitchSync",1},{"tone",0.68f},{"scaleOn",1},{"scale",1},
            {"attack",0.002f},{"decay",0.35f},{"sustain",0.15f},{"release",0.42f},
            {"fx3Type",3},{"fx3Amount",25.f} }},
        { "Music Box", "Leads & Keys", "Bell", {
            {"grainSize",0.06f},{"density",0.55f},{"pitchSync",1},{"octave",1},{"tone",0.75f},{"scaleOn",1},{"scale",1},
            {"attack",0.001f},{"decay",0.65f},{"sustain",0.0f},{"release",0.85f},{"fx5Type",5},{"fx5Amount",32.f} }},
        { "Marimba Grains", "Leads & Keys", "Mallet", {
            {"grainSize",0.06f},{"density",0.6f},{"pitchSync",1},{"tone",0.65f},{"scaleOn",1},{"scale",1},
            {"attack",0.001f},{"decay",0.42f},{"sustain",0.0f},{"release",0.32f} }},
        { "Vox Lead", "Leads & Keys", "Vox", {
            {"grainSize",0.16f},{"density",0.85f},{"pitchSync",1},{"tone",0.7f},{"scaleOn",1},{"scale",1},
            {"attack",0.04f},{"decay",0.25f},{"sustain",0.82f},{"release",0.45f},
            {"fx2Type",2},{"fx2Amount",30.f},{"fx5Type",5},{"fx5Amount",28.f} }},
        { "Bell Choir", "Leads & Keys", "Bell", {
            {"grainSize",0.11f},{"density",0.75f},{"pitchSync",1},{"octave",1},{"tone",0.75f},{"scaleOn",1},{"scale",1},
            {"attack",0.003f},{"decay",0.95f},{"sustain",0.15f},{"release",1.3f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Soft Keys", "Leads & Keys", "Vox", {
            {"grainSize",0.18f},{"density",0.78f},{"pitchSync",1},{"tone",0.58f},{"scaleOn",1},
            {"attack",0.02f},{"decay",0.4f},{"sustain",0.75f},{"release",0.52f},{"masterLPF",7500.f} }},
        { "Detuned Lead", "Leads & Keys", "Pluck", {
            {"grainSize",0.1f},{"density",0.72f},{"pitchJitter",12.f},{"panSpread",0.6f},{"tone",0.6f},{"pitchSync",1},{"scaleOn",1},{"scale",1},
            {"attack",0.004f},{"decay",0.28f},{"sustain",0.55f},{"release",0.32f},
            {"fx2Type",2},{"fx2Amount",32.f},{"fx3Type",3},{"fx3Amount",25.f} }},
        { "Kalimba Dream", "Leads & Keys", "Mallet", {
            {"grainSize",0.06f},{"density",0.55f},{"pitchSync",1},{"tone",0.72f},{"octave",1},{"scaleOn",1},{"scale",4},
            {"attack",0.001f},{"decay",0.52f},{"sustain",0.0f},{"release",0.52f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Ghost Choir", "Leads & Keys", "Vox", {
            {"grainSize",0.2f},{"density",0.85f},{"pitchSync",1},{"tone",0.62f},{"positionSpray",0.15f},{"scaleOn",1},
            {"attack",0.35f},{"release",1.6f},{"fx5Type",5},{"fx5Amount",50.f},{"revShimmer",0.6f} }},
        { "Harp Cascade", "Leads & Keys", "Pluck", {
            {"grainSize",0.08f},{"density",0.65f},{"pitchSync",1},{"tone",0.72f},{"scaleOn",1},{"scale",4},{"octave",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",12},{"arpOctaves",3},{"arpGate",0.7f},
            {"attack",0.002f},{"decay",0.42f},{"sustain",0.1f},{"release",0.65f},{"fx5Type",5},{"fx5Amount",30.f} }},

        // ============================================================
        // F. CINEMATIC (10) - ピッチ感強化版
        // ============================================================
        { "Rising Tension", "Cinematic", "Riser", {
            {"grainSize",0.15f},{"density",0.85f},{"positionSpray",0.25f},{"tone",0.42f},{"pitchSync",1},{"scaleOn",1},
            {"env1Attack",5.5f},{"env1Decay",0.5f},{"mod1Src",5},{"mod1Dst",9},{"mod1Amt",0.75f},{"mod1Uni",1},
            {"mod2Src",5},{"mod2Dst",8},{"mod2Amt",0.5f},{"attack",0.4f},{"release",1.2f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Impact Bloom", "Cinematic", "Impact", {
            {"grainSize",0.28f},{"density",0.92f},{"octave",-1},{"pitchJitter",20.f},{"tone",0.4f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.001f},{"decay",1.4f},{"sustain",0.25f},{"release",2.4f},
            {"fx1Type",1},{"fx1Amount",35.f},{"fx5Type",5},{"fx5Amount",58.f},{"revDecay",0.92f} }},
        { "Deep Drone", "Cinematic", "Drone", {
            {"grainSize",0.45f},{"density",0.92f},{"octave",-2},{"tone",0.45f},{"pitchSync",1},{"scaleOn",1},{"speed",0.2f},
            {"masterLPF",2400.f},{"attack",2.8f},{"release",3.8f},
            {"lfo1Rate",0.08f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.3f},{"fx5Type",5},{"fx5Amount",52.f} }},
        { "Underscore Pad", "Cinematic", "Underscore", {
            {"grainSize",0.34f},{"density",0.82f},{"positionSpray",0.18f},{"tone",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"attack",1.8f},{"release",2.8f},{"masterLPF",5500.f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Doom Riser", "Cinematic", "Riser", {
            {"grainSize",0.12f},{"density",0.88f},{"octave",-1},{"pitchJitter",25.f},{"tone",0.4f},{"scaleOn",1},
            {"env1Attack",7.f},{"mod1Src",5},{"mod1Dst",5},{"mod1Amt",0.7f},{"mod1Uni",1},
            {"mod2Src",5},{"mod2Dst",3},{"mod2Amt",0.6f},{"fx1Type",1},{"fx1Amount",30.f},{"fx5Type",5},{"fx5Amount",50.f} }},
        { "Sub Impact", "Cinematic", "Impact", {
            {"grainSize",0.26f},{"density",0.88f},{"octave",-2},{"pitchSync",1},{"tone",0.38f},{"scaleOn",1},
            {"attack",0.001f},{"decay",0.95f},{"sustain",0.12f},{"release",1.7f},
            {"fx1Type",1},{"fx1Amount",28.f},{"masterLPF",1800.f} }},
        { "Endless Fall", "Cinematic", "Drone", {
            {"grainSize",0.25f},{"density",0.85f},{"reverseGrain",1},{"speed",0.3f},{"positionSpray",0.25f},{"tone",0.44f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",0.12f},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",-0.35f},
            {"attack",1.4f},{"release",2.8f},{"fx5Type",5},{"fx5Amount",52.f} }},
        { "War Horns", "Cinematic", "Underscore", {
            {"grainSize",0.22f},{"density",0.9f},{"octave",-1},{"tone",0.58f},{"pitchSync",1},{"scaleOn",1},
            {"fx1Type",1},{"fx1Amount",30.f},{"attack",0.25f},{"release",1.3f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Cryo Chamber", "Cinematic", "Drone", {
            {"grainSize",0.38f},{"density",0.85f},{"pitchJitter",15.f},{"positionSpray",0.2f},{"tone",0.48f},{"pitchSync",1},{"scaleOn",1},
            {"masterHPF",160.f},{"attack",2.2f},{"release",3.2f},
            {"fx4Type",4},{"fx4Amount",32.f},{"fx5Type",5},{"fx5Amount",48.f},{"revShimmer",0.52f} }},
        { "Signal Lost", "Cinematic", "Riser", {
            {"grainSize",0.08f},{"density",0.75f},{"positionSpray",0.3f},{"pitchJitter",28.f},{"tone",0.42f},{"scaleOn",1},
            {"env1Attack",4.5f},{"env1Loop",0},{"mod1Src",5},{"mod1Dst",13},{"mod1Amt",0.75f},{"mod1Uni",1},
            {"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",45.f} }},

        // ============================================================
        // G. GLITCH & IDM (10) - ピッチ感強化版
        // ============================================================
        { "Data Corrupt", "Glitch & IDM", "Broken", {
            {"grainSize",0.05f},{"density",0.65f},{"playMode",3},{"positionSpray",0.3f},{"pitchJitter",25.f},{"tone",0.4f},{"scaleOn",1},
            {"lfo1Rate",7.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.5f},
            {"attack",0.002f},{"release",0.22f} }},
        { "Micro Chop", "Glitch & IDM", "Chop", {
            {"grainSize",0.04f},{"density",0.75f},{"tone",0.45f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",12.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",3},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.75f},{"attack",0.001f},{"release",0.12f} }},
        { "Random Access", "Glitch & IDM", "Random", {
            {"grainSize",0.06f},{"density",0.7f},{"playMode",3},{"pitchJitter",20.f},{"tone",0.42f},{"scaleOn",1},
            {"mod1Src",11},{"mod1Dst",1},{"mod1Amt",0.6f},{"mod2Src",11},{"mod2Dst",8},{"mod2Amt",0.35f},
            {"attack",0.002f},{"release",0.25f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Buffer Error", "Glitch & IDM", "Broken", {
            {"grainSize",0.05f},{"density",0.62f},{"reverseGrain",1},{"positionSpray",0.25f},{"tone",0.42f},{"scaleOn",1},
            {"lfo1Rate",9.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",7},{"mod1Amt",0.65f},
            {"fx4Type",4},{"fx4Amount",38.f} }},
        { "Bit Rattle", "Glitch & IDM", "Chop", {
            {"grainSize",0.06f},{"density",0.78f},{"pitchJitter",15.f},{"tone",0.48f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",8.f},{"lfo1Sync",1},{"lfo1SyncRate",14},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",3},{"mod1Amt",0.65f},
            {"fx1Type",1},{"fx1Amount",30.f},{"attack",0.001f},{"release",0.14f} }},
        { "Neural Noise", "Glitch & IDM", "Random", {
            {"grainSize",0.05f},{"density",0.65f},{"playMode",3},{"positionSpray",0.35f},{"pitchJitter",28.f},{"tone",0.4f},{"scaleOn",1},
            {"mod1Src",11},{"mod1Dst",9},{"mod1Amt",0.5f},{"mod2Src",11},{"mod2Dst",6},{"mod2Amt",0.7f},
            {"masterHPF",200.f},{"fx5Type",5},{"fx5Amount",30.f} }},
        { "Skip Groove", "Glitch & IDM", "Chop", {
            {"grainSize",0.08f},{"density",0.72f},{"tone",0.52f},{"pitchSync",1},{"scaleOn",1},
            {"arpEnable",1},{"arpPattern",10},{"arpRateSync",14},{"arpOctaves",2},
            {"lfo1Rate",6.f},{"lfo1Sync",1},{"lfo1SyncRate",12},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.4f},
            {"attack",0.002f},{"release",0.18f} }},
        { "Fractal Grains", "Glitch & IDM", "Random", {
            {"grainSize",0.06f},{"density",0.72f},{"pitchJitter",18.f},{"tone",0.45f},{"scaleOn",1},
            {"lfo1Rate",5.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.45f},
            {"lfo2Rate",3.f},{"lfo2Wave",5},{"mod2Src",2},{"mod2Dst",8},{"mod2Amt",0.35f},{"fx3Type",3},{"fx3Amount",28.f} }},
        { "Clock Divider", "Glitch & IDM", "Broken", {
            {"grainSize",0.08f},{"density",0.7f},{"tone",0.48f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",4.f},{"lfo1Sync",1},{"lfo1SyncRate",9},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",19},{"mod1Amt",-0.75f},{"mod1Uni",1},
            {"fx4Type",4},{"fx4Amount",32.f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Granular Stutter", "Glitch & IDM", "Chop", {
            {"grainSize",0.04f},{"density",0.82f},{"tone",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"lfo1Rate",10.f},{"lfo1Sync",1},{"lfo1SyncRate",15},{"lfo1Wave",4},
            {"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.8f},{"attack",0.001f},{"release",0.1f},{"fx5Type",5},{"fx5Amount",28.f} }},

        // ============================================================
        // H. EXPERIMENTAL (10) - ピッチ感強化版
        // ============================================================
        { "Chaos Engine", "Experimental", "Chaos", {
            {"grainSize",0.1f},{"density",0.75f},{"pitchJitter",30.f},{"positionSpray",0.35f},{"panSpread",0.9f},{"tone",0.45f},{"scaleOn",1},
            {"lfo1Rate",3.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.55f},
            {"lfo2Rate",7.f},{"lfo2Wave",5},{"mod2Src",2},{"mod2Dst",8},{"mod2Amt",0.45f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Alien Broadcast", "Experimental", "FX", {
            {"grainSize",0.07f},{"density",0.68f},{"octave",1},{"pitchJitter",35.f},{"tone",0.52f},{"scaleOn",1},
            {"lfo1Rate",2.f},{"lfo1Wave",4},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.6f},{"mod1Uni",1},
            {"fx1Type",1},{"fx1Amount",28.f},{"fx3Type",3},{"fx3Amount",38.f},{"dlyFeedback",0.62f} }},
        { "Mobius Loop", "Experimental", "Wild", {
            {"grainSize",0.14f},{"density",0.82f},{"reverseGrain",1},{"speed",0.5f},{"tone",0.48f},{"pitchSync",1},{"scaleOn",1},
            {"env1Attack",1.8f},{"env1Decay",1.8f},{"env1Loop",1},{"mod1Src",5},{"mod1Dst",1},{"mod1Amt",0.5f},
            {"env2Attack",1.f},{"env2Decay",2.5f},{"env2Loop",1},{"mod2Src",6},{"mod2Dst",8},{"mod2Amt",0.35f},
            {"fx5Type",5},{"fx5Amount",45.f} }},
        { "Quantum Foam", "Experimental", "Chaos", {
            {"grainSize",0.05f},{"density",0.85f},{"positionSpray",0.45f},{"pitchJitter",28.f},{"panSpread",0.9f},{"tone",0.42f},{"scaleOn",1},
            {"mod1Src",11},{"mod1Dst",2},{"mod1Amt",0.55f},{"mod2Src",11},{"mod2Dst",1},{"mod2Amt",0.7f},
            {"fx4Type",4},{"fx4Amount",38.f},{"fx5Type",5},{"fx5Amount",40.f} }},
        { "Time Stretch Ghost", "Experimental", "Wild", {
            {"grainSize",0.32f},{"density",0.92f},{"speed",0.12f},{"positionSpray",0.2f},{"pitchJitter",15.f},{"tone",0.5f},{"pitchSync",1},{"scaleOn",1},
            {"attack",0.9f},{"release",2.4f},{"fx4Type",4},{"fx4Amount",45.f},{"fx5Type",5},{"fx5Amount",48.f} }},
        { "Metallic Resonator", "Experimental", "FX", {
            {"grainSize",0.08f},{"density",0.75f},{"pitchSync",1},{"tone",0.85f},{"octave",1},{"scaleOn",1},
            {"lfo1Rate",0.5f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.45f},{"mod1Uni",1},
            {"fx5Type",5},{"fx5Amount",45.f},{"revShimmer",0.72f} }},
        { "Insect Swarm", "Experimental", "Chaos", {
            {"grainSize",0.03f},{"density",0.92f},{"positionSpray",0.45f},{"pitchJitter",40.f},{"panSpread",0.9f},{"tone",0.4f},{"scaleOn",1},
            {"lfo1Rate",8.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",8},{"mod1Amt",0.3f},
            {"masterHPF",350.f},{"fx3Type",3},{"fx3Amount",30.f} }},
        { "Reverse Cathedral", "Experimental", "Wild", {
            {"grainSize",0.35f},{"density",0.86f},{"reverseGrain",1},{"positionSpray",0.18f},{"tone",0.52f},{"pitchSync",1},{"scaleOn",1},
            {"attack",1.8f},{"release",2.8f},{"fx5Type",5},{"fx5Amount",58.f},{"revDecay",0.92f},{"revShimmer",0.55f} }},
        { "Dial Tone Choir", "Experimental", "FX", {
            {"grainSize",0.18f},{"density",0.82f},{"pitchSync",1},{"tone",0.72f},{"scaleOn",1},{"scale",6},
            {"lfo1Rate",6.f},{"lfo1Wave",3},{"mod1Src",1},{"mod1Dst",11},{"mod1Amt",0.25f},
            {"fx2Type",2},{"fx2Amount",38.f},{"fx5Type",5},{"fx5Amount",35.f} }},
        { "Total Collapse", "Experimental", "Chaos", {
            {"grainSize",0.04f},{"density",0.88f},{"playMode",3},{"pitchJitter",45.f},{"positionSpray",0.5f},{"panSpread",0.9f},{"tone",0.42f},{"scaleOn",1},
            {"lfo1Rate",7.f},{"lfo1Wave",5},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.65f},
            {"lfo2Rate",5.f},{"lfo2Wave",4},{"mod2Src",2},{"mod2Dst",9},{"mod2Amt",0.65f},
            {"fx1Type",1},{"fx1Amount",35.f},{"fx5Type",5},{"fx5Amount",42.f} }},

        // ============================================================
        // I. 新設カテゴリ「Pitch」40選（グラニュラー×Tone共鳴 特化版）
        //   P-Sync依存を脱却し、Tone(0.7〜0.95)でピッチ感を自然に確保。
        //   Size/Density/Spray/Jitter/Speed/PlayMode/Arp/Gate/MODを駆使した
        //   40種類の全く異なる音響世界。
        // ============================================================

        // --- 1. Pluck & Keys (8) ---
        { "Reso Pluck C3", "Pitch", "Pluck", {
            {"tone",0.88f},{"grainSize",0.035f},{"density",0.42f},{"windowShape",3},{"panSpread",0.7f},
            {"attack",0.001f},{"decay",0.35f},{"sustain",0.0f},{"release",0.35f},
            {"masterHPF",120.f},{"scaleOn",1},{"fx3Type",3},{"fx3Amount",22.f} }},
        { "Crystal Harp", "Pitch", "Harp", {
            {"tone",0.82f},{"grainSize",0.08f},{"density",0.55f},{"positionSpray",0.25f},{"spraySpeed",0.15f},
            {"octave",1},{"pitchJitter",8.f},{"attack",0.002f},{"decay",0.8f},{"sustain",0.1f},{"release",1.0f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",38.f},{"revShimmer",0.65f} }},
        { "Steel Kalimba", "Pitch", "Pluck", {
            {"tone",0.92f},{"grainSize",0.025f},{"density",0.38f},{"windowShape",2},{"speed",0.5f},
            {"octave",1},{"attack",0.001f},{"decay",0.5f},{"sustain",0.0f},{"release",0.4f},
            {"scaleOn",1},{"fx2Type",2},{"fx2Amount",20.f},{"fx3Type",3},{"fx3Amount",28.f} }},
        { "Bamboo Marimba", "Pitch", "Pluck", {
            {"tone",0.78f},{"grainSize",0.045f},{"density",0.48f},{"windowShape",1},{"positionSpray",0.1f},
            {"attack",0.001f},{"decay",0.32f},{"sustain",0.0f},{"release",0.28f},
            {"scaleOn",1},{"masterLPF",8000.f},{"fx1Type",1},{"fx1Amount",18.f},{"satDrive",2.5f} }},
        { "Music Box Fairy", "Pitch", "Bell", {
            {"tone",0.9f},{"grainSize",0.04f},{"density",0.45f},{"reverseGrain",1},{"octave",2},
            {"attack",0.001f},{"decay",0.75f},{"sustain",0.0f},{"release",0.85f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",35.f},{"revShimmer",0.55f} }},
        { "Celestial Glocken", "Pitch", "Bell", {
            {"tone",0.88f},{"grainSize",0.05f},{"density",0.52f},{"octave",2},{"panSpread",0.9f},
            {"lfo1Rate",4.5f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.15f},
            {"attack",0.001f},{"decay",0.6f},{"sustain",0.0f},{"release",0.6f},
            {"scaleOn",1},{"masterHPF",250.f},{"fx2Type",2},{"fx2Amount",25.f} }},
        { "Tibetan Singing Bowl", "Pitch", "Bell", {
            {"tone",0.95f},{"grainSize",0.35f},{"density",0.85f},{"speed",0.08f},{"playMode",2},
            {"attack",0.6f},{"decay",1.8f},{"sustain",0.85f},{"release",2.5f},
            {"scaleOn",1},{"fx4Type",4},{"fx4Amount",30.f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Glass Harmonica", "Pitch", "Bell", {
            {"tone",0.92f},{"grainSize",0.22f},{"density",0.72f},{"positionSpray",0.3f},{"spraySpeed",0.05f},
            {"attack",0.4f},{"decay",1.2f},{"sustain",0.75f},{"release",1.8f},
            {"scaleOn",1},{"fx3Type",3},{"fx3Amount",25.f},{"fx5Type",5},{"fx5Amount",40.f} }},

        // --- 2. Leads & Solos (8) ---
        { "Singing Cello Lead", "Pitch", "Lead", {
            {"tone",0.84f},{"grainSize",0.16f},{"density",0.78f},{"panSpread",0.6f},
            {"lfo1Rate",5.2f},{"mod1Src",1},{"mod1Dst",12},{"mod1Amt",0.22f},
            {"attack",0.08f},{"decay",0.35f},{"sustain",0.85f},{"release",0.45f},
            {"scaleOn",1},{"masterLPF",5500.f},{"fx1Type",1},{"fx1Amount",25.f},{"satDrive",3.5f} }},
        { "Cyber Flute", "Pitch", "Lead", {
            {"tone",0.78f},{"grainSize",0.11f},{"density",0.65f},{"octave",1},{"positionSpray",0.25f},
            {"attack",0.05f},{"decay",0.25f},{"sustain",0.8f},{"release",0.38f},
            {"scaleOn",1},{"fx3Type",3},{"fx3Amount",30.f},{"dlyFeedback",0.45f} }},
        { "Analog Brass Lead", "Pitch", "Lead", {
            {"tone",0.8f},{"grainSize",0.08f},{"density",0.88f},{"windowShape",1},
            {"attack",0.04f},{"decay",0.3f},{"sustain",0.8f},{"release",0.35f},
            {"scaleOn",1},{"filterSlope",1},{"masterLPF",4800.f},{"fx1Type",1},{"fx1Amount",35.f} }},
        { "Vocal Formant Solo", "Pitch", "Lead", {
            {"tone",0.76f},{"grainSize",0.14f},{"density",0.82f},{"positionSpray",0.15f},
            {"attack",0.03f},{"decay",0.25f},{"sustain",0.82f},{"release",0.4f},
            {"scaleOn",1},{"fx6Type",6},{"fx6Amount",65.f},{"gateVowel",60.f} }},
        { "Retro 8-Bit Lead", "Pitch", "Lead", {
            {"tone",0.86f},{"grainSize",0.02f},{"density",0.4f},{"windowShape",2},{"speed",0.0f},
            {"attack",0.002f},{"decay",0.2f},{"sustain",0.7f},{"release",0.2f},
            {"scaleOn",1},{"fx1Type",1},{"fx1Amount",40.f},{"satDrive",5.0f} }},
        { "Overdriven Fusion", "Pitch", "Lead", {
            {"tone",0.85f},{"grainSize",0.07f},{"density",0.9f},{"panSpread",0.7f},
            {"attack",0.008f},{"decay",0.3f},{"sustain",0.85f},{"release",0.35f},
            {"scaleOn",1},{"masterLPF",6000.f},{"fx1Type",1},{"fx1Amount",60.f},{"satDrive",8.0f} }},
        { "Ethereal Whistle", "Pitch", "Lead", {
            {"tone",0.88f},{"grainSize",0.13f},{"density",0.62f},{"octave",1},
            {"attack",0.06f},{"decay",0.2f},{"sustain",0.82f},{"release",0.4f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",38.f},{"revShimmer",0.5f} }},
        { "Psychedelic Sitar", "Pitch", "Lead", {
            {"tone",0.94f},{"grainSize",0.06f},{"density",0.72f},{"pitchJitter",45.f},
            {"attack",0.005f},{"decay",0.45f},{"sustain",0.65f},{"release",0.55f},
            {"scaleOn",1},{"scale",7},{"fx2Type",2},{"fx2Amount",30.f},{"fx3Type",3},{"fx3Amount",25.f} }},

        // --- 3. Ambient & Evolving Pads (8) ---
        { "Aurora Borealis", "Pitch", "Ambient", {
            {"tone",0.72f},{"grainSize",0.42f},{"density",0.88f},{"speed",0.2f},{"positionSpray",0.5f},
            {"attack",1.8f},{"decay",2.2f},{"sustain",0.92f},{"release",3.0f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",48.f},{"revShimmer",0.65f} }},
        { "Tuned Nebula", "Pitch", "Pad", {
            {"tone",0.76f},{"grainSize",0.34f},{"density",0.82f},{"playMode",2},{"speed",0.35f},
            {"lfo1Rate",0.18f},{"mod1Src",1},{"mod1Dst",1},{"mod1Amt",0.35f},
            {"attack",1.2f},{"decay",1.5f},{"sustain",0.88f},{"release",2.5f},
            {"scaleOn",1},{"masterLPF",6500.f},{"fx2Type",2},{"fx2Amount",32.f} }},
        { "Frozen Strings", "Pitch", "Strings", {
            {"tone",0.82f},{"grainSize",0.28f},{"density",0.8f},{"speed",0.0f},
            {"attack",0.8f},{"decay",1.4f},{"sustain",0.85f},{"release",2.2f},
            {"scaleOn",1},{"fx4Type",4},{"fx4Amount",35.f},{"frzSize",280.f} }},
        { "Cathedral Choir", "Pitch", "Pad", {
            {"tone",0.8f},{"grainSize",0.32f},{"density",0.85f},{"octave",1},
            {"attack",1.4f},{"decay",1.8f},{"sustain",0.9f},{"release",2.8f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",45.f},{"revDecay",0.9f} }},
        { "Underwater Sanctuary", "Pitch", "Ambient", {
            {"tone",0.74f},{"grainSize",0.38f},{"density",0.76f},{"positionSpray",0.4f},
            {"attack",1.5f},{"decay",2.0f},{"sustain",0.9f},{"release",2.6f},
            {"scaleOn",1},{"masterLPF",2200.f},{"fx2Type",2},{"fx2Amount",25.f},{"fx3Type",3},{"fx3Amount",35.f} }},
        { "Solar Flare Atmosphere", "Pitch", "Ambient", {
            {"tone",0.7f},{"grainSize",0.45f},{"density",0.92f},{"playMode",3},{"speed",0.4f},
            {"attack",2.0f},{"decay",2.5f},{"sustain",0.95f},{"release",3.5f},
            {"scaleOn",1},{"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",42.f} }},
        { "Glass Forest", "Pitch", "Pad", {
            {"tone",0.86f},{"grainSize",0.22f},{"density",0.68f},{"positionSpray",0.45f},{"reverseGrain",1},
            {"attack",0.9f},{"decay",1.3f},{"sustain",0.82f},{"release",2.0f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",40.f},{"revShimmer",0.7f} }},
        { "Velvet Sunset", "Pitch", "Pad", {
            {"tone",0.78f},{"grainSize",0.3f},{"density",0.8f},{"panSpread",0.95f},
            {"attack",1.5f},{"decay",1.6f},{"sustain",0.88f},{"release",2.4f},
            {"scaleOn",1},{"masterLPF",5000.f},{"fx2Type",2},{"fx2Amount",28.f},{"fx5Type",5},{"fx5Amount",35.f} }},

        // --- 4. Bass & Sub (6) ---
        { "Reso Sub Boom", "Pitch", "Bass", {
            {"tone",0.86f},{"grainSize",0.12f},{"density",0.9f},{"octave",-2},{"panSpread",0.2f},
            {"attack",0.002f},{"decay",0.8f},{"sustain",0.2f},{"release",0.45f},
            {"scaleOn",1},{"masterLPF",850.f},{"fx1Type",1},{"fx1Amount",22.f} }},
        { "Acid Reso 303", "Pitch", "Bass", {
            {"tone",0.92f},{"grainSize",0.045f},{"density",0.86f},{"octave",-1},{"panSpread",0.3f},
            {"attack",0.003f},{"decay",0.35f},{"sustain",0.4f},{"release",0.25f},
            {"scaleOn",1},{"masterLPF",2400.f},{"fx1Type",1},{"fx1Amount",55.f},{"satDrive",7.0f} }},
        { "Upright Acoustic Bass", "Pitch", "Bass", {
            {"tone",0.8f},{"grainSize",0.075f},{"density",0.68f},{"windowShape",1},{"octave",-1},
            {"attack",0.004f},{"decay",0.5f},{"sustain",0.15f},{"release",0.38f},
            {"scaleOn",1},{"masterLPF",2000.f},{"fx1Type",1},{"fx1Amount",15.f} }},
        { "Cyberpunk Reese", "Pitch", "Bass", {
            {"tone",0.82f},{"grainSize",0.14f},{"density",0.94f},{"octave",-1},{"panSpread",0.85f},
            {"attack",0.01f},{"decay",0.4f},{"sustain",0.85f},{"release",0.4f},
            {"scaleOn",1},{"masterLPF",2800.f},{"fx1Type",1},{"fx1Amount",45.f},{"fx2Type",2},{"fx2Amount",30.f} }},
        { "Plucked Reso Bass", "Pitch", "Bass Pluck", {
            {"tone",0.88f},{"grainSize",0.038f},{"density",0.58f},{"windowShape",3},{"octave",-1},
            {"attack",0.001f},{"decay",0.32f},{"sustain",0.05f},{"release",0.28f},
            {"scaleOn",1},{"masterLPF",3200.f},{"fx1Type",1},{"fx1Amount",28.f} }},
        { "Industrial Drone Bass", "Pitch", "Bass Drone", {
            {"tone",0.84f},{"grainSize",0.42f},{"density",0.95f},{"octave",-2},{"speed",0.1f},
            {"attack",1.2f},{"decay",1.8f},{"sustain",0.92f},{"release",2.5f},
            {"scaleOn",1},{"masterLPF",1600.f},{"fx1Type",1},{"fx1Amount",48.f},{"satDrive",6.5f} }},

        // --- 5. Rhythmic & Arp (6) ---
        { "Clockwork Chime", "Pitch", "Arp", {
            {"tone",0.9f},{"grainSize",0.045f},{"density",0.5f},{"octave",1},
            {"arpEnable",1},{"arpPattern",0},{"arpRateSync",14},{"arpGate",0.6f},
            {"attack",0.001f},{"decay",0.28f},{"sustain",0.0f},{"release",0.28f},
            {"scaleOn",1},{"fx3Type",3},{"fx3Amount",24.f} }},
        { "Pentatonic Raindrop", "Pitch", "Arp", {
            {"tone",0.84f},{"grainSize",0.06f},{"density",0.55f},{"octave",1},{"scale",4},
            {"arpEnable",1},{"arpPattern",20},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.5f},
            {"attack",0.001f},{"decay",0.35f},{"sustain",0.0f},{"release",0.32f},
            {"scaleOn",1},{"fx5Type",5},{"fx5Amount",32.f} }},
        { "Granular Techno Gate", "Pitch", "Gate", {
            {"tone",0.82f},{"grainSize",0.09f},{"density",0.82f},
            {"attack",0.005f},{"decay",0.3f},{"sustain",0.8f},{"release",0.3f},
            {"scaleOn",1},{"fx6Type",6},{"fx6Amount",85.f},{"gateRate",4},{"gateDepth",90.f} }},
        { "Shuffled Koto Groove", "Pitch", "Arp", {
            {"tone",0.86f},{"grainSize",0.065f},{"density",0.62f},{"scale",6},
            {"arpEnable",1},{"arpPattern",16},{"arpRateSync",12},{"arpOctaves",2},{"arpGate",0.55f},
            {"attack",0.002f},{"decay",0.3f},{"sustain",0.1f},{"release",0.3f},
            {"scaleOn",1},{"fx2Type",2},{"fx2Amount",20.f},{"fx3Type",3},{"fx3Amount",22.f} }},
        { "Glitch Pulse Walker", "Pitch", "Gate", {
            {"tone",0.8f},{"grainSize",0.03f},{"density",0.65f},{"playMode",3},{"speed",1.2f},
            {"attack",0.002f},{"decay",0.25f},{"sustain",0.65f},{"release",0.25f},
            {"scaleOn",1},{"fx6Type",6},{"fx6Amount",80.f},{"gateRate",6},{"fx1Type",1},{"fx1Amount",30.f} }},
        { "Polyrhythm Bell Arp", "Pitch", "Arp", {
            {"tone",0.92f},{"grainSize",0.075f},{"density",0.6f},{"octave",1},
            {"arpEnable",1},{"arpPattern",18},{"arpRateSync",9},{"arpGate",0.65f},
            {"attack",0.002f},{"decay",0.4f},{"sustain",0.05f},{"release",0.4f},
            {"scaleOn",1},{"fx3Type",3},{"fx3Amount",32.f},{"fx5Type",5},{"fx5Amount",30.f} }},

        // --- 6. Cinematic & Sci-Fi (4) ---
        { "Blade Runner Horizon", "Pitch", "Cinematic", {
            {"tone",0.82f},{"grainSize",0.36f},{"density",0.88f},{"octave",-1},{"speed",0.25f},
            {"attack",0.8f},{"decay",1.5f},{"sustain",0.85f},{"release",2.4f},
            {"scaleOn",1},{"masterLPF",4200.f},{"fx3Type",3},{"fx3Amount",35.f},{"fx5Type",5},{"fx5Amount",45.f} }},
        { "Alien Artifact", "Pitch", "Cinematic", {
            {"tone",0.94f},{"grainSize",0.12f},{"density",0.7f},{"pitchJitter",160.f},{"reverseGrain",1},
            {"attack",0.3f},{"decay",1.2f},{"sustain",0.75f},{"release",1.8f},
            {"scaleOn",1},{"fx4Type",4},{"fx4Amount",38.f},{"fx5Type",5},{"fx5Amount",38.f} }},
        { "Time Stasis Field", "Pitch", "Cinematic", {
            {"tone",0.86f},{"grainSize",0.4f},{"density",0.85f},{"speed",0.0f},{"position",0.45f},
            {"attack",1.0f},{"decay",1.8f},{"sustain",0.88f},{"release",2.6f},
            {"scaleOn",1},{"fx4Type",4},{"fx4Amount",45.f},{"fx5Type",5},{"fx5Amount",48.f} }},
        { "Event Horizon Sweep", "Pitch", "Cinematic", {
            {"tone",0.85f},{"grainSize",0.32f},{"density",0.86f},{"playMode",2},{"speed",0.15f},
            {"lfo1Rate",0.08f},{"mod1Src",1},{"mod1Dst",13},{"mod1Amt",0.35f},
            {"attack",1.8f},{"decay",2.0f},{"sustain",0.9f},{"release",3.0f},
            {"scaleOn",1},{"fx1Type",1},{"fx1Amount",25.f},{"fx5Type",5},{"fx5Amount",50.f} }},
        };
        return P;
    }
}
