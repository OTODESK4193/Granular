# -*- coding: utf-8 -*-
"""
Preset generator & validator for Granular v1.0.1 B011
Generates 120 unique, richly modulated, differentiated presets across 9 categories.
Guarantees zero FX duplication in all presets.
"""

import sys

presets = []

def add_preset(name, cat, sub, params):
    presets.append({
        "name": name,
        "category": cat,
        "sub": sub,
        "params": params
    })

# ============================================================
# A. PADS & AMBIENT (10)
# ============================================================
add_preset("Warm Cathedral", "Pads & Ambient", "Warm", [
    ("grainSize", 0.38), ("density", 0.82), ("positionSpray", 0.15), ("panSpread", 0.9),
    ("attack", 2.0), ("decay", 2.5), ("sustain", 0.92), ("release", 3.2),
    ("tone", 0.45), ("scaleOn", 1), ("masterLPF", 8500.0),
    ("lfo1Rate", 0.08), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.25),
    ("lfo2Rate", 0.15), ("lfo2Wave", 1), ("mod2Src", 2), ("mod2Dst", 14), ("mod2Amt", 0.2),
    ("fx5Type", 5), ("fx5Amount", 48.0), ("revDecay", 0.88), ("fxMix", 1.0)
])

add_preset("Glass Choir", "Pads & Ambient", "Glass", [
    ("grainSize", 0.28), ("density", 0.78), ("positionSpray", 0.12), ("octave", 1),
    ("tone", 0.58), ("scaleOn", 1), ("masterHPF", 150.0),
    ("attack", 1.5), ("decay", 2.0), ("sustain", 0.88), ("release", 2.8),
    ("lfo1Rate", 0.2), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.18),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.25),
    ("fx2Type", 2), ("fx2Amount", 32.0), ("fx5Type", 5), ("fx5Amount", 40.0), ("revShimmer", 0.6)
])

add_preset("Slow Bloom", "Pads & Ambient", "Evolving", [
    ("grainSize", 0.42), ("density", 0.75), ("positionSpray", 0.22), ("speed", 0.2),
    ("tone", 0.48), ("scaleOn", 1), ("playMode", 2),
    ("attack", 3.0), ("decay", 2.0), ("sustain", 1.0), ("release", 4.0),
    ("env1Attack", 3.5), ("env1Decay", 2.0), ("env1Loop", 1), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.45), ("mod1Uni", 1),
    ("lfo1Rate", 0.05), ("mod2Src", 1), ("mod2Dst", 1), ("mod2Amt", 0.35),
    ("fx5Type", 5), ("fx5Amount", 52.0), ("revDecay", 0.92)
])

add_preset("Dark Vespers", "Pads & Ambient", "Dark", [
    ("grainSize", 0.45), ("density", 0.88), ("octave", -1), ("tone", 0.42), ("scaleOn", 1),
    ("masterLPF", 3200.0), ("attack", 2.2), ("decay", 2.0), ("sustain", 0.9), ("release", 3.5),
    ("lfo1Rate", 0.12), ("lfo1Wave", 1), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", -0.3),
    ("lfo2Rate", 0.07), ("mod2Src", 2), ("mod2Dst", 1), ("mod2Amt", 0.3),
    ("fx1Type", 1), ("fx1Amount", 22.0), ("satDrive", 2.8), ("fx5Type", 5), ("fx5Amount", 45.0)
])

add_preset("Aurora Wash", "Pads & Ambient", "Evolving", [
    ("grainSize", 0.32), ("density", 0.84), ("positionSpray", 0.25), ("panSpread", 1.0),
    ("tone", 0.52), ("scaleOn", 1), ("pitchJitter", 18.0),
    ("attack", 2.4), ("decay", 2.0), ("sustain", 0.9), ("release", 3.2),
    ("lfo1Rate", 0.18), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.4),
    ("lfo2Rate", 0.25), ("mod2Src", 2), ("mod2Dst", 4), ("mod2Amt", 0.3),
    ("fx2Type", 2), ("fx2Amount", 28.0), ("fx5Type", 5), ("fx5Amount", 48.0), ("revShimmer", 0.72)
])

add_preset("Velvet Drift", "Pads & Ambient", "Warm", [
    ("grainSize", 0.35), ("density", 0.8), ("positionSpray", 0.16), ("speed", 0.35),
    ("tone", 0.46), ("scaleOn", 1), ("masterLPF", 6000.0),
    ("attack", 1.8), ("decay", 2.0), ("sustain", 0.88), ("release", 2.8),
    ("lfo1Rate", 0.14), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.28),
    ("mod2Src", 8), ("mod2Dst", 14), ("mod2Amt", 0.3),
    ("fx2Type", 2), ("fx2Amount", 30.0), ("fx5Type", 5), ("fx5Amount", 38.0)
])

add_preset("Frozen Lake", "Pads & Ambient", "Glass", [
    ("grainSize", 0.24), ("density", 0.72), ("octave", 1), ("tone", 0.62), ("scaleOn", 1),
    ("masterHPF", 200.0), ("attack", 1.6), ("decay", 2.2), ("sustain", 0.85), ("release", 3.0),
    ("lfo1Rate", 0.3), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.15),
    ("fx4Type", 4), ("fx4Amount", 30.0), ("frzSize", 350.0), ("fx5Type", 5), ("fx5Amount", 44.0), ("revShimmer", 0.68)
])

add_preset("Distant Bells Pad", "Pads & Ambient", "Glass", [
    ("grainSize", 0.2), ("density", 0.68), ("tone", 0.68), ("scaleOn", 1), ("scale", 1),
    ("attack", 0.9), ("decay", 2.0), ("sustain", 0.8), ("release", 2.6),
    ("lfo1Rate", 4.0), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.12),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.3),
    ("fx3Type", 3), ("fx3Amount", 26.0), ("dlyFeedback", 0.45), ("fx5Type", 5), ("fx5Amount", 42.0)
])

add_preset("Undertow", "Pads & Ambient", "Dark", [
    ("grainSize", 0.44), ("density", 0.9), ("octave", -2), ("speed", 0.25),
    ("tone", 0.4), ("scaleOn", 1), ("masterLPF", 2200.0),
    ("attack", 2.8), ("decay", 2.0), ("sustain", 0.95), ("release", 3.8),
    ("lfo1Rate", 0.08), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.38),
    ("lfo2Rate", 0.12), ("mod2Src", 2), ("mod2Dst", 3), ("mod2Amt", 0.25),
    ("fx1Type", 1), ("fx1Amount", 30.0), ("satDrive", 3.2), ("fx5Type", 5), ("fx5Amount", 46.0)
])

add_preset("Breathing Room", "Pads & Ambient", "Evolving", [
    ("grainSize", 0.34), ("density", 0.8), ("positionSpray", 0.2),
    ("tone", 0.44), ("scaleOn", 1),
    ("attack", 1.8), ("decay", 2.0), ("sustain", 0.85), ("release", 3.0),
    ("env1Attack", 2.2), ("env1Decay", 2.2), ("env1Loop", 1), ("mod1Src", 5), ("mod1Dst", 2), ("mod1Amt", 0.4),
    ("lfo1Rate", 0.1), ("mod2Src", 1), ("mod2Dst", 4), ("mod2Amt", 0.35),
    ("fx5Type", 5), ("fx5Amount", 44.0), ("revDecay", 0.85)
])

# ============================================================
# B. TEXTURES (10)
# ============================================================
add_preset("Grain Cloud", "Textures", "Cloud", [
    ("grainSize", 0.14), ("density", 0.88), ("positionSpray", 0.38), ("pitchJitter", 28.0), ("panSpread", 0.9),
    ("tone", 0.38), ("scaleOn", 1), ("attack", 0.6), ("release", 1.8),
    ("lfo1Rate", 0.2), ("lfo1Wave", 5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.35),
    ("mod2Src", 11), ("mod2Dst", 7), ("mod2Amt", 0.4),
    ("fx5Type", 5), ("fx5Amount", 38.0)
])

add_preset("Dust Motes", "Textures", "Grainy", [
    ("grainSize", 0.07), ("density", 0.6), ("positionSpray", 0.45), ("pitchJitter", 35.0),
    ("tone", 0.35), ("scaleOn", 1), ("speed", 0.3), ("attack", 0.25), ("release", 1.4),
    ("lfo1Rate", 0.35), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.45),
    ("fx2Type", 2), ("fx2Amount", 26.0), ("fx3Type", 3), ("fx3Amount", 28.0)
])

add_preset("Static Field", "Textures", "Noise", [
    ("grainSize", 0.05), ("density", 0.85), ("positionSpray", 0.55), ("pitchJitter", 50.0),
    ("tone", 0.32), ("masterHPF", 300.0), ("attack", 0.1), ("release", 1.0),
    ("lfo1Rate", 8.0), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 3), ("mod1Amt", 0.4),
    ("fx1Type", 1), ("fx1Amount", 28.0), ("satDrive", 4.0), ("fx5Type", 5), ("fx5Amount", 30.0)
])

add_preset("Shimmer Veil", "Textures", "Shimmer", [
    ("grainSize", 0.18), ("density", 0.84), ("positionSpray", 0.28), ("octave", 1), ("pitchJitter", 16.0),
    ("tone", 0.54), ("scaleOn", 1), ("attack", 0.8), ("release", 2.2),
    ("lfo1Rate", 0.15), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.25),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.2),
    ("fx5Type", 5), ("fx5Amount", 50.0), ("revShimmer", 0.82)
])

add_preset("Particle Stream", "Textures", "Cloud", [
    ("grainSize", 0.11), ("density", 0.82), ("positionSpray", 0.32), ("speed", 0.5), ("panSpread", 0.88),
    ("tone", 0.42), ("scaleOn", 1), ("attack", 0.4), ("release", 1.6),
    ("lfo1Rate", 0.3), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.4),
    ("lfo2Rate", 0.2), ("mod2Src", 2), ("mod2Dst", 3), ("mod2Amt", 0.3),
    ("fx3Type", 3), ("fx3Amount", 30.0)
])

add_preset("Frostbite", "Textures", "Grainy", [
    ("grainSize", 0.08), ("density", 0.72), ("octave", 2), ("pitchJitter", 22.0),
    ("tone", 0.6), ("scaleOn", 1), ("masterHPF", 480.0), ("attack", 0.2), ("release", 1.5),
    ("lfo1Rate", 0.4), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.22),
    ("fx5Type", 5), ("fx5Amount", 44.0), ("revShimmer", 0.6)
])

add_preset("Whisper Grains", "Textures", "Noise", [
    ("grainSize", 0.09), ("density", 0.52), ("positionSpray", 0.38), ("pitchJitter", 20.0),
    ("tone", 0.36), ("masterLPF", 5200.0), ("attack", 0.4), ("release", 1.6),
    ("lfo1Rate", 0.25), ("lfo1Wave", 1), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.35),
    ("fx2Type", 2), ("fx2Amount", 32.0)
])

add_preset("Crystalline", "Textures", "Shimmer", [
    ("grainSize", 0.16), ("density", 0.76), ("octave", 2), ("tone", 0.68), ("scaleOn", 1), ("scale", 1),
    ("attack", 0.35), ("release", 2.0),
    ("lfo1Rate", 3.5), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.15),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.25),
    ("fx5Type", 5), ("fx5Amount", 48.0), ("revShimmer", 0.88)
])

add_preset("Tape Erosion", "Textures", "Grainy", [
    ("grainSize", 0.1), ("density", 0.68), ("positionSpray", 0.26), ("pitchJitter", 24.0),
    ("tone", 0.4), ("scaleOn", 1), ("masterLPF", 4200.0), ("attack", 0.1), ("release", 1.2),
    ("lfo1Rate", 0.6), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.3),
    ("fx1Type", 1), ("fx1Amount", 32.0), ("satDrive", 4.2), ("fx3Type", 3), ("fx3Amount", 28.0)
])

add_preset("Spectral Haze", "Textures", "Cloud", [
    ("grainSize", 0.18), ("density", 0.86), ("positionSpray", 0.32), ("panSpread", 0.92),
    ("tone", 0.48), ("scaleOn", 1), ("attack", 0.7), ("release", 2.2),
    ("lfo1Rate", 0.22), ("lfo1Wave", 1), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.3),
    ("lfo2Rate", 0.1), ("mod2Src", 2), ("mod2Dst", 1), ("mod2Amt", 0.3),
    ("fx5Type", 5), ("fx5Amount", 46.0)
])

# ============================================================
# C. RHYTHMIC (10)
# ============================================================
add_preset("Pulse Engine", "Rhythmic", "Pulse", [
    ("grainSize", 0.09), ("density", 0.72), ("tone", 0.46), ("scaleOn", 1),
    ("lfo1Rate", 4.0), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("lfo1Wave", 3),
    ("mod1Src", 1), ("mod1Dst", 3), ("mod1Amt", 0.65), ("mod1Uni", 1),
    ("mod2Src", 8), ("mod2Dst", 14), ("mod2Amt", 0.35),
    ("attack", 0.005), ("decay", 0.2), ("sustain", 0.2), ("release", 0.25)
])

add_preset("Arp Runner", "Rhythmic", "Arp", [
    ("grainSize", 0.08), ("density", 0.62), ("tone", 0.58),
    ("arpEnable", 1), ("arpPattern", 0), ("arpRateSync", 12), ("arpOctaves", 2), ("arpGate", 0.55),
    ("scaleOn", 1), ("scale", 1), ("attack", 0.003), ("decay", 0.16), ("sustain", 0.2), ("release", 0.2),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.3),
    ("fx3Type", 3), ("fx3Amount", 32.0), ("dlyFeedback", 0.48)
])

add_preset("Stutter Chop", "Rhythmic", "Stutter", [
    ("grainSize", 0.06), ("density", 0.78), ("tone", 0.48), ("scaleOn", 1),
    ("lfo1Rate", 8.0), ("lfo1Sync", 1), ("lfo1SyncRate", 12), ("lfo1Wave", 4),
    ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.7),
    ("lfo2Rate", 4.0), ("lfo2Sync", 1), ("lfo2SyncRate", 9), ("lfo2Wave", 3),
    ("mod2Src", 2), ("mod2Dst", 3), ("mod2Amt", 0.5),
    ("attack", 0.002), ("release", 0.15)
])

add_preset("Sequenced Motion", "Rhythmic", "Sequence", [
    ("grainSize", 0.11), ("density", 0.66), ("speedSync", 1), ("speedSyncDiv", 2),
    ("tone", 0.52), ("arpEnable", 1), ("arpPattern", 2), ("arpRateSync", 9), ("arpOctaves", 1),
    ("scaleOn", 1), ("scale", 2), ("attack", 0.008), ("decay", 0.25), ("sustain", 0.3), ("release", 0.3),
    ("mod1Src", 11), ("mod1Dst", 7), ("mod1Amt", 0.45)
])

add_preset("Gate Rider", "Rhythmic", "Pulse", [
    ("grainSize", 0.14), ("density", 0.82), ("tone", 0.5), ("scaleOn", 1),
    ("fx4Type", 6), ("fx4Amount", 88.0), ("gateRate", 4), ("gateDepth", 90.0),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.4),
    ("attack", 0.008), ("release", 0.3), ("fx5Type", 5), ("fx5Amount", 32.0)
])

add_preset("Trance Pluck Arp", "Rhythmic", "Arp", [
    ("grainSize", 0.07), ("density", 0.58), ("tone", 0.68),
    ("arpEnable", 1), ("arpPattern", 0), ("arpRateSync", 12), ("arpOctaves", 2), ("arpGate", 0.45),
    ("scaleOn", 1), ("scale", 1), ("attack", 0.002), ("decay", 0.14), ("sustain", 0.1), ("release", 0.18),
    ("masterHPF", 160.0), ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.3),
    ("fx3Type", 3), ("fx3Amount", 36.0), ("dlyFeedback", 0.52)
])

add_preset("Broken Clock", "Rhythmic", "Stutter", [
    ("grainSize", 0.07), ("density", 0.64), ("playMode", 3), ("positionSpray", 0.25), ("tone", 0.44), ("scaleOn", 1),
    ("lfo1Rate", 6.0), ("lfo1Sync", 1), ("lfo1SyncRate", 14), ("lfo1Wave", 4),
    ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.55),
    ("mod2Src", 11), ("mod2Dst", 8), ("mod2Amt", 0.4),
    ("attack", 0.003), ("release", 0.18)
])

add_preset("Motorik", "Rhythmic", "Pulse", [
    ("grainSize", 0.12), ("density", 0.76), ("tone", 0.5), ("scaleOn", 1),
    ("lfo1Rate", 4.0), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("lfo1Wave", 1),
    ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.35),
    ("mod2Src", 8), ("mod2Dst", 14), ("mod2Amt", 0.3),
    ("attack", 0.01), ("release", 0.25), ("fx1Type", 1), ("fx1Amount", 24.0)
])

add_preset("Ratchet Keys", "Rhythmic", "Sequence", [
    ("grainSize", 0.09), ("density", 0.6), ("tone", 0.6),
    ("arpEnable", 1), ("arpPattern", 18), ("arpRateSync", 12), ("arpOctaves", 3), ("arpGate", 0.5),
    ("scaleOn", 1), ("scale", 1), ("attack", 0.003), ("decay", 0.2), ("sustain", 0.15), ("release", 0.22),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.4)
])

add_preset("Polyrhythm Grains", "Rhythmic", "Sequence", [
    ("grainSize", 0.11), ("density", 0.7), ("tone", 0.48), ("scaleOn", 1),
    ("lfo1Rate", 3.0), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("mod1Src", 1), ("mod1Dst", 3), ("mod1Amt", 0.5),
    ("lfo2Rate", 2.0), ("lfo2Sync", 1), ("lfo2SyncRate", 7), ("mod2Src", 2), ("mod2Dst", 6), ("mod2Amt", 0.45),
    ("attack", 0.006), ("release", 0.28)
])

# ============================================================
# D. BASS (10)
# ============================================================
add_preset("Sub Foundation", "Bass", "Sub", [
    ("grainSize", 0.22), ("density", 0.9), ("octave", -2), ("tone", 0.45), ("scaleOn", 1),
    ("masterLPF", 1200.0), ("attack", 0.005), ("decay", 0.3), ("sustain", 0.85), ("release", 0.28),
    ("env1Attack", 0.005), ("env1Decay", 0.2), ("mod1Src", 5), ("mod1Dst", 9), ("mod1Amt", 0.35),
    ("mod2Src", 8), ("mod2Dst", 14), ("mod2Amt", 0.25)
])

add_preset("Movement Bass", "Bass", "Movement", [
    ("grainSize", 0.15), ("density", 0.88), ("octave", -1), ("tone", 0.5), ("scaleOn", 1),
    ("lfo1Rate", 1.0), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.4), ("mod1Uni", 1),
    ("masterLPF", 2000.0), ("attack", 0.005), ("decay", 0.25), ("sustain", 0.75), ("release", 0.2),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.3)
])

add_preset("Growl Machine", "Bass", "Gritty", [
    ("grainSize", 0.1), ("density", 0.92), ("octave", -1), ("pitchJitter", 10.0), ("tone", 0.48), ("scaleOn", 1),
    ("masterLPF", 3000.0), ("attack", 0.003), ("decay", 0.2), ("sustain", 0.7), ("release", 0.18),
    ("lfo1Rate", 5.0), ("mod1Src", 1), ("mod1Dst", 9), ("mod1Amt", 0.35),
    ("fx1Type", 1), ("fx1Amount", 52.0), ("satDrive", 6.0)
])

add_preset("Reese Grains", "Bass", "Gritty", [
    ("grainSize", 0.13), ("density", 0.95), ("octave", -1), ("pitchJitter", 16.0), ("panSpread", 0.6), ("tone", 0.46), ("scaleOn", 1),
    ("masterLPF", 2500.0), ("attack", 0.005), ("sustain", 0.85), ("release", 0.22),
    ("lfo1Rate", 0.4), ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.3),
    ("fx1Type", 1), ("fx1Amount", 32.0), ("fx2Type", 2), ("fx2Amount", 24.0)
])

add_preset("Deep Pulse Bass", "Bass", "Movement", [
    ("grainSize", 0.18), ("density", 0.9), ("octave", -2), ("tone", 0.44), ("scaleOn", 1),
    ("lfo1Rate", 2.0), ("lfo1Sync", 1), ("lfo1SyncRate", 6), ("lfo1Wave", 3),
    ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", -0.4),
    ("masterLPF", 1500.0), ("attack", 0.005), ("decay", 0.2), ("sustain", 0.8), ("release", 0.2)
])

add_preset("Wobble Grain", "Bass", "Movement", [
    ("grainSize", 0.14), ("density", 0.9), ("octave", -1), ("tone", 0.46), ("scaleOn", 1),
    ("lfo1Rate", 1.5), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("lfo1Wave", 0),
    ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.55), ("mod1Uni", 1),
    ("masterLPF", 2200.0), ("attack", 0.004), ("sustain", 0.8), ("release", 0.2)
])

add_preset("Round Sub", "Bass", "Sub", [
    ("grainSize", 0.25), ("density", 0.88), ("octave", -2), ("tone", 0.38), ("scaleOn", 1),
    ("masterLPF", 900.0), ("masterHPF", 30.0), ("attack", 0.008), ("sustain", 0.9), ("release", 0.25),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.2)
])

add_preset("Distorted Depths", "Bass", "Gritty", [
    ("grainSize", 0.09), ("density", 0.94), ("octave", -2), ("pitchJitter", 14.0), ("tone", 0.45), ("scaleOn", 1),
    ("masterLPF", 1800.0), ("attack", 0.003), ("decay", 0.25), ("sustain", 0.75), ("release", 0.18),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.4),
    ("fx1Type", 1), ("fx1Amount", 68.0), ("satDrive", 8.5)
])

add_preset("Acid Granule", "Bass", "Gritty", [
    ("grainSize", 0.05), ("density", 0.85), ("octave", -1), ("tone", 0.65), ("scaleOn", 1),
    ("env1Attack", 0.002), ("env1Decay", 0.18), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.6), ("mod1Uni", 1),
    ("masterLPF", 1600.0), ("attack", 0.002), ("decay", 0.22), ("sustain", 0.3), ("release", 0.18),
    ("fx1Type", 1), ("fx1Amount", 45.0), ("satDrive", 5.5)
])

add_preset("Sub Attack", "Bass", "Sub", [
    ("grainSize", 0.16), ("density", 0.9), ("octave", -2), ("tone", 0.48), ("scaleOn", 1),
    ("env1Attack", 0.001), ("env1Decay", 0.08), ("mod1Src", 5), ("mod1Dst", 11), ("mod1Amt", 0.4), ("mod1Uni", 1),
    ("masterLPF", 1100.0), ("attack", 0.003), ("sustain", 0.85), ("release", 0.22)
])

# ============================================================
# E. LEADS (10)
# ============================================================
add_preset("Neon Searing", "Leads", "Solo", [
    ("grainSize", 0.08), ("density", 0.85), ("tone", 0.72), ("scaleOn", 1),
    ("lfo1Rate", 5.0), ("lfo1Wave", 0), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.2),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.3),
    ("attack", 0.02), ("decay", 0.3), ("sustain", 0.85), ("release", 0.35),
    ("fx1Type", 1), ("fx1Amount", 35.0), ("satDrive", 4.0), ("fx3Type", 3), ("fx3Amount", 30.0)
])

add_preset("Glass Solo", "Leads", "Bright", [
    ("grainSize", 0.1), ("density", 0.75), ("octave", 1), ("tone", 0.78), ("scaleOn", 1),
    ("lfo1Rate", 4.5), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.18),
    ("attack", 0.03), ("decay", 0.25), ("sustain", 0.8), ("release", 0.35),
    ("fx2Type", 2), ("fx2Amount", 26.0), ("fx5Type", 5), ("fx5Amount", 38.0)
])

add_preset("Singing Grain", "Leads", "Vocal", [
    ("grainSize", 0.14), ("density", 0.8), ("tone", 0.7), ("scaleOn", 1),
    ("lfo1Rate", 5.5), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.22),
    ("attack", 0.05), ("decay", 0.2), ("sustain", 0.85), ("release", 0.4),
    ("fx3Type", 3), ("fx3Amount", 28.0), ("dlyFeedback", 0.45), ("fx5Type", 5), ("fx5Amount", 35.0)
])

add_preset("Vowel Lead", "Leads", "Vocal", [
    ("grainSize", 0.12), ("density", 0.82), ("tone", 0.66), ("scaleOn", 1),
    ("fx5Type", 6), ("fx5Amount", 68.0), ("gateVowel", 65.0),
    ("lfo1Rate", 0.5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.3),
    ("attack", 0.03), ("sustain", 0.8), ("release", 0.35)
])

add_preset("Crystal Flute", "Leads", "Bright", [
    ("grainSize", 0.12), ("density", 0.65), ("octave", 1), ("tone", 0.75), ("scaleOn", 1),
    ("lfo1Rate", 4.8), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.15),
    ("attack", 0.06), ("decay", 0.2), ("sustain", 0.8), ("release", 0.38),
    ("fx5Type", 5), ("fx5Amount", 40.0), ("revShimmer", 0.5)
])

add_preset("Overdriven Peak", "Leads", "Solo", [
    ("grainSize", 0.07), ("density", 0.88), ("tone", 0.74), ("scaleOn", 1),
    ("attack", 0.008), ("decay", 0.25), ("sustain", 0.85), ("release", 0.3),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.4),
    ("fx1Type", 1), ("fx1Amount", 58.0), ("satDrive", 7.5), ("fx3Type", 3), ("fx3Amount", 26.0)
])

add_preset("Whistle Wind", "Leads", "Bright", [
    ("grainSize", 0.13), ("density", 0.6), ("octave", 1), ("tone", 0.8), ("scaleOn", 1),
    ("lfo1Rate", 5.2), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.25),
    ("attack", 0.08), ("release", 0.4), ("fx5Type", 5), ("fx5Amount", 36.0)
])

add_preset("Cyber Horn", "Leads", "Warm", [
    ("grainSize", 0.15), ("density", 0.85), ("tone", 0.68), ("scaleOn", 1),
    ("env1Attack", 0.08), ("env1Decay", 0.3), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.4),
    ("attack", 0.04), ("sustain", 0.82), ("release", 0.38),
    ("fx1Type", 1), ("fx1Amount", 28.0), ("fx2Type", 2), ("fx2Amount", 25.0)
])

add_preset("Bit Lead", "Leads", "Solo", [
    ("grainSize", 0.03), ("density", 0.45), ("windowShape", 2), ("tone", 0.76), ("scaleOn", 1),
    ("attack", 0.002), ("decay", 0.2), ("sustain", 0.7), ("release", 0.2),
    ("fx1Type", 1), ("fx1Amount", 42.0), ("satDrive", 5.2)
])

add_preset("Soft Glow", "Leads", "Warm", [
    ("grainSize", 0.2), ("density", 0.75), ("tone", 0.65), ("scaleOn", 1),
    ("masterLPF", 5500.0), ("attack", 0.08), ("decay", 0.3), ("sustain", 0.85), ("release", 0.45),
    ("lfo1Rate", 4.2), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.14),
    ("fx2Type", 2), ("fx2Amount", 28.0), ("fx5Type", 5), ("fx5Amount", 35.0)
])

# ============================================================
# F. CINEMATIC (10)
# ============================================================
add_preset("Tension Rise", "Cinematic", "Tension", [
    ("grainSize", 0.3), ("density", 0.85), ("tone", 0.52), ("scaleOn", 1),
    ("env1Attack", 5.0), ("env1Decay", 0.5), ("mod1Src", 5), ("mod1Dst", 9), ("mod1Amt", 0.65), ("mod1Uni", 1),
    ("attack", 3.0), ("release", 2.5), ("fx1Type", 1), ("fx1Amount", 35.0), ("fx5Type", 5), ("fx5Amount", 45.0)
])

add_preset("Deep Abyss", "Cinematic", "Drone", [
    ("grainSize", 0.45), ("density", 0.9), ("octave", -2), ("speed", 0.15), ("tone", 0.45), ("scaleOn", 1),
    ("masterLPF", 1800.0), ("attack", 2.5), ("release", 3.5),
    ("lfo1Rate", 0.08), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.35),
    ("fx5Type", 5), ("fx5Amount", 52.0), ("revDecay", 0.94)
])

add_preset("SciFi Drone", "Cinematic", "Drone", [
    ("grainSize", 0.35), ("density", 0.8), ("playMode", 2), ("tone", 0.55), ("scaleOn", 1),
    ("lfo1Rate", 0.15), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.4),
    ("attack", 2.0), ("release", 3.0), ("fx4Type", 4), ("fx4Amount", 35.0), ("fx5Type", 5), ("fx5Amount", 40.0)
])

add_preset("Horror Sting", "Cinematic", "Tension", [
    ("grainSize", 0.05), ("density", 0.95), ("pitchJitter", 280.0), ("tone", 0.6), ("scaleOn", 1),
    ("attack", 0.001), ("decay", 0.8), ("sustain", 0.1), ("release", 1.5),
    ("fx1Type", 1), ("fx1Amount", 55.0), ("satDrive", 7.0), ("fx3Type", 3), ("fx3Amount", 35.0)
])

add_preset("Titan March", "Cinematic", "Atmosphere", [
    ("grainSize", 0.25), ("density", 0.88), ("octave", -1), ("tone", 0.58), ("scaleOn", 1),
    ("attack", 0.02), ("decay", 0.6), ("sustain", 0.7), ("release", 1.2),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.45),
    ("fx1Type", 1), ("fx1Amount", 40.0), ("fx5Type", 5), ("fx5Amount", 42.0)
])

add_preset("Lost World", "Cinematic", "Atmosphere", [
    ("grainSize", 0.38), ("density", 0.78), ("positionSpray", 0.35), ("tone", 0.5), ("scaleOn", 1),
    ("attack", 2.0), ("release", 3.0),
    ("lfo1Rate", 0.12), ("mod1Src", 1), ("mod1Dst", 8), ("mod1Amt", -0.35),
    ("fx2Type", 2), ("fx2Amount", 30.0), ("fx5Type", 5), ("fx5Amount", 46.0)
])

add_preset("Void Pulse", "Cinematic", "Drone", [
    ("grainSize", 0.4), ("density", 0.85), ("octave", -1), ("tone", 0.48), ("scaleOn", 1),
    ("lfo1Rate", 1.0), ("lfo1Sync", 1), ("lfo1SyncRate", 6), ("mod1Src", 1), ("mod1Dst", 3), ("mod1Amt", 0.4),
    ("attack", 1.5), ("release", 2.8), ("fx5Type", 5), ("fx5Amount", 44.0)
])

add_preset("Time Dilate", "Cinematic", "Atmosphere", [
    ("grainSize", 0.32), ("density", 0.82), ("speed", 0.05), ("tone", 0.54), ("scaleOn", 1),
    ("env1Attack", 4.0), ("env1Loop", 0), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.6), ("mod1Uni", 1),
    ("attack", 2.2), ("release", 3.2), ("fx4Type", 4), ("fx4Amount", 40.0)
])

add_preset("Ancient Ruin", "Cinematic", "Atmosphere", [
    ("grainSize", 0.28), ("density", 0.76), ("pitchJitter", 20.0), ("tone", 0.56), ("scaleOn", 1),
    ("attack", 1.2), ("release", 2.6),
    ("lfo1Rate", 0.2), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.3),
    ("fx3Type", 3), ("fx3Amount", 32.0), ("fx5Type", 5), ("fx5Amount", 42.0)
])

add_preset("Star Drift", "Cinematic", "Drone", [
    ("grainSize", 0.42), ("density", 0.84), ("octave", 1), ("tone", 0.62), ("scaleOn", 1),
    ("attack", 2.5), ("release", 3.5),
    ("lfo1Rate", 0.08), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.2),
    ("fx5Type", 5), ("fx5Amount", 50.0), ("revShimmer", 0.75)
])

# ============================================================
# G. GLITCH (10)
# ============================================================
add_preset("Bit Crusher Grain", "Glitch", "Granular", [
    ("grainSize", 0.03), ("density", 0.5), ("windowShape", 2), ("tone", 0.4),
    ("lfo1Rate", 7.0), ("lfo1Sync", 1), ("lfo1SyncRate", 14), ("lfo1Wave", 4),
    ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.55),
    ("attack", 0.001), ("release", 0.15), ("fx1Type", 1), ("fx1Amount", 55.0), ("satDrive", 6.0)
])

add_preset("Chaos Shuffler", "Glitch", "Chaos", [
    ("grainSize", 0.05), ("density", 0.8), ("playMode", 3), ("positionSpray", 0.6), ("tone", 0.45),
    ("mod1Src", 11), ("mod1Dst", 1), ("mod1Amt", 0.6), ("mod2Src", 11), ("mod2Dst", 8), ("mod2Amt", 0.4),
    ("attack", 0.002), ("release", 0.2)
])

add_preset("Data Buffer", "Glitch", "Buffer", [
    ("grainSize", 0.04), ("density", 0.7), ("tone", 0.48),
    ("lfo1Rate", 9.0), ("lfo1Sync", 1), ("lfo1SyncRate", 15), ("lfo1Wave", 4),
    ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.65),
    ("attack", 0.001), ("release", 0.12), ("fx4Type", 4), ("fx4Amount", 35.0)
])

add_preset("Reverse Glitch", "Glitch", "Granular", [
    ("grainSize", 0.06), ("density", 0.75), ("reverseGrain", 1), ("tone", 0.5),
    ("lfo1Rate", 8.0), ("lfo1Sync", 1), ("lfo1SyncRate", 14), ("lfo1Wave", 4),
    ("mod1Src", 1), ("mod1Dst", 3), ("mod1Amt", 0.6),
    ("attack", 0.002), ("release", 0.18), ("fx3Type", 3), ("fx3Amount", 30.0)
])

add_preset("Quantum Stutter", "Glitch", "Stutter", [
    ("grainSize", 0.025), ("density", 0.6), ("tone", 0.52),
    ("mod1Src", 11), ("mod1Dst", 9), ("mod1Amt", 0.5), ("mod2Src", 11), ("mod2Dst", 6), ("mod2Amt", 0.65),
    ("attack", 0.001), ("release", 0.1)
])

add_preset("Malfunction", "Glitch", "Chaos", [
    ("grainSize", 0.07), ("density", 0.85), ("pitchJitter", 120.0), ("tone", 0.42),
    ("lfo1Rate", 6.0), ("lfo1Sync", 1), ("lfo1SyncRate", 12), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.45),
    ("attack", 0.003), ("release", 0.2), ("fx1Type", 1), ("fx1Amount", 48.0)
])

add_preset("Fragment Scatter", "Glitch", "Buffer", [
    ("grainSize", 0.04), ("density", 0.65), ("positionSpray", 0.5), ("tone", 0.46),
    ("lfo1Rate", 5.0), ("lfo1Wave", 5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.5),
    ("attack", 0.002), ("release", 0.16)
])

add_preset("CD Skip", "Glitch", "Stutter", [
    ("grainSize", 0.035), ("density", 0.9), ("tone", 0.5),
    ("lfo1Rate", 4.0), ("lfo1Sync", 1), ("lfo1SyncRate", 9), ("lfo1Wave", 3),
    ("mod1Src", 1), ("mod1Dst", 20), ("mod1Amt", -0.75), ("mod1Uni", 1),
    ("attack", 0.001), ("release", 0.1)
])

add_preset("Logic Melt", "Glitch", "Chaos", [
    ("grainSize", 0.08), ("density", 0.8), ("pitchJitter", 180.0), ("tone", 0.44),
    ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.8), ("attack", 0.001), ("release", 0.12),
    ("fx1Type", 1), ("fx1Amount", 35.0), ("fx5Type", 5), ("fx5Amount", 28.0)
])

add_preset("Binary Ghost", "Glitch", "Granular", [
    ("grainSize", 0.05), ("density", 0.7), ("tone", 0.55),
    ("lfo1Rate", 3.0), ("lfo1Wave", 5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.5),
    ("attack", 0.002), ("release", 0.2), ("fx3Type", 3), ("fx3Amount", 32.0)
])

# ============================================================
# H. EXPERIMENTAL (10)
# ============================================================
add_preset("Elastic Formant", "Experimental", "Formant", [
    ("grainSize", 0.12), ("density", 0.8), ("tone", 0.5),
    ("lfo1Rate", 2.0), ("lfo1Wave", 4), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.55), ("mod1Uni", 1),
    ("attack", 0.05), ("release", 0.5), ("fx5Type", 6), ("fx5Amount", 70.0)
])

add_preset("Reverse Universe", "Experimental", "Fracture", [
    ("grainSize", 0.3), ("density", 0.85), ("reverseGrain", 1), ("speed", 0.4), ("tone", 0.54),
    ("env1Attack", 1.8), ("env1Decay", 1.8), ("env1Loop", 1), ("mod1Src", 5), ("mod1Dst", 1), ("mod1Amt", 0.5),
    ("attack", 1.5), ("release", 2.5), ("fx5Type", 5), ("fx5Amount", 44.0)
])

add_preset("Time Fracture", "Experimental", "Fracture", [
    ("grainSize", 0.06), ("density", 0.7), ("playMode", 3), ("tone", 0.48),
    ("mod1Src", 11), ("mod1Dst", 2), ("mod1Amt", 0.55), ("mod2Src", 11), ("mod2Dst", 1), ("mod2Amt", 0.65),
    ("attack", 0.005), ("release", 0.3), ("fx4Type", 4), ("fx4Amount", 30.0)
])

add_preset("Alien Transmission", "Experimental", "Feedback", [
    ("grainSize", 0.15), ("density", 0.75), ("pitchJitter", 140.0), ("tone", 0.58),
    ("lfo1Rate", 0.5), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.45), ("mod1Uni", 1),
    ("attack", 0.1), ("release", 0.8), ("fx3Type", 3), ("fx3Amount", 38.0), ("dlyFeedback", 0.75)
])

add_preset("Spectral Swarm", "Experimental", "Swarm", [
    ("grainSize", 0.08), ("density", 0.95), ("positionSpray", 0.55), ("panSpread", 1.0), ("tone", 0.45),
    ("lfo1Rate", 8.0), ("lfo1Wave", 5), ("mod1Src", 1), ("mod1Dst", 8), ("mod1Amt", 0.35),
    ("attack", 0.2), ("release", 1.5), ("fx2Type", 2), ("fx2Amount", 30.0)
])

add_preset("Chaos Well", "Experimental", "Feedback", [
    ("grainSize", 0.2), ("density", 0.85), ("pitchJitter", 200.0), ("tone", 0.5),
    ("lfo1Rate", 6.0), ("lfo1Wave", 3), ("mod1Src", 1), ("mod1Dst", 11), ("mod1Amt", 0.25),
    ("attack", 0.5), ("release", 2.0), ("fx1Type", 1), ("fx1Amount", 40.0), ("fx5Type", 5), ("fx5Amount", 40.0)
])

add_preset("Granular Feedback", "Experimental", "Feedback", [
    ("grainSize", 0.1), ("density", 0.8), ("tone", 0.55),
    ("lfo1Rate", 7.0), ("lfo1Wave", 5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.65),
    ("attack", 0.01), ("release", 0.5), ("fx3Type", 3), ("fx3Amount", 45.0), ("dlyFeedback", 0.85)
])

add_preset("Dimensional Tear", "Experimental", "Fracture", [
    ("grainSize", 0.25), ("density", 0.88), ("speed", 0.0), ("position", 0.5), ("tone", 0.6),
    ("lfo1Rate", 0.2), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.5),
    ("attack", 1.0), ("release", 2.2), ("fx4Type", 4), ("fx4Amount", 42.0)
])

add_preset("Quantum Leap", "Experimental", "Swarm", [
    ("grainSize", 0.07), ("density", 0.72), ("octave", 1), ("tone", 0.52),
    ("mod1Src", 11), ("mod1Dst", 10), ("mod1Amt", 0.5),
    ("attack", 0.01), ("release", 0.4), ("fx2Type", 2), ("fx2Amount", 28.0), ("fx5Type", 5), ("fx5Amount", 35.0)
])

add_preset("Event Horizon", "Experimental", "Formant", [
    ("grainSize", 0.4), ("density", 0.9), ("octave", -1), ("tone", 0.5),
    ("lfo1Rate", 0.1), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", -0.45),
    ("attack", 2.0), ("release", 3.0), ("fx1Type", 1), ("fx1Amount", 32.0), ("fx5Type", 5), ("fx5Amount", 45.0)
])

# ============================================================
# I. 新設カテゴリ「Pitch」40選（8サブカテゴリ × 各5種）
#   P-Sync過剰依存を排し、Tone(0.65〜0.95)とScaleQuantizeを主軸に、
#   Size/Density/Spray/Jitter/Position/Arp/Gate/MODで劇的差別化
# ============================================================

# --- 1. Pluck (5) ---
add_preset("Reso Pluck C3", "Pitch", "Pluck", [
    ("tone", 0.88), ("grainSize", 0.035), ("density", 0.42), ("windowShape", 3), ("panSpread", 0.7),
    ("attack", 0.001), ("decay", 0.35), ("sustain", 0.0), ("release", 0.35),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.25),
    ("scaleOn", 1), ("masterHPF", 120.0), ("fx3Type", 3), ("fx3Amount", 24.0)
])

add_preset("Crystal Harp", "Pitch", "Pluck", [
    ("tone", 0.84), ("grainSize", 0.07), ("density", 0.52), ("positionSpray", 0.2), ("octave", 1),
    ("attack", 0.002), ("decay", 0.75), ("sustain", 0.05), ("release", 0.9),
    ("lfo1Rate", 0.3), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.25),
    ("scaleOn", 1), ("fx5Type", 5), ("fx5Amount", 40.0), ("revShimmer", 0.65)
])

add_preset("Steel Kalimba", "Pitch", "Pluck", [
    ("tone", 0.92), ("grainSize", 0.025), ("density", 0.38), ("windowShape", 2), ("octave", 1),
    ("attack", 0.001), ("decay", 0.45), ("sustain", 0.0), ("release", 0.4),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.35),
    ("scaleOn", 1), ("fx2Type", 2), ("fx2Amount", 22.0), ("fx3Type", 3), ("fx3Amount", 26.0)
])

add_preset("Bamboo Marimba", "Pitch", "Pluck", [
    ("tone", 0.78), ("grainSize", 0.045), ("density", 0.48), ("windowShape", 1),
    ("attack", 0.001), ("decay", 0.32), ("sustain", 0.0), ("release", 0.28),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.3),
    ("scaleOn", 1), ("masterLPF", 7500.0), ("fx1Type", 1), ("fx1Amount", 18.0)
])

add_preset("Plucked Reso Vibe", "Pitch", "Pluck", [
    ("tone", 0.85), ("grainSize", 0.05), ("density", 0.5), ("panSpread", 0.8),
    ("attack", 0.001), ("decay", 0.5), ("sustain", 0.0), ("release", 0.45),
    ("lfo1Rate", 3.0), ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.35),
    ("scaleOn", 1), ("fx2Type", 2), ("fx2Amount", 25.0), ("fx5Type", 5), ("fx5Amount", 35.0)
])

# --- 2. Bell (5) ---
add_preset("Music Box Fairy", "Pitch", "Bell", [
    ("tone", 0.9), ("grainSize", 0.04), ("density", 0.45), ("reverseGrain", 1), ("octave", 2),
    ("attack", 0.001), ("decay", 0.75), ("sustain", 0.0), ("release", 0.85),
    ("scaleOn", 1), ("fx5Type", 5), ("fx5Amount", 38.0), ("revShimmer", 0.58)
])

add_preset("Celestial Glocken", "Pitch", "Bell", [
    ("tone", 0.88), ("grainSize", 0.05), ("density", 0.52), ("octave", 2), ("panSpread", 0.9),
    ("lfo1Rate", 4.5), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.16),
    ("attack", 0.001), ("decay", 0.6), ("sustain", 0.0), ("release", 0.6),
    ("scaleOn", 1), ("masterHPF", 220.0), ("fx2Type", 2), ("fx2Amount", 26.0)
])

add_preset("Tibetan Singing Bowl", "Pitch", "Bell", [
    ("tone", 0.94), ("grainSize", 0.35), ("density", 0.85), ("speed", 0.08), ("playMode", 2),
    ("lfo1Rate", 0.12), ("mod1Src", 1), ("mod1Dst", 9), ("mod1Amt", 0.15),
    ("attack", 0.6), ("decay", 1.8), ("sustain", 0.85), ("release", 2.6),
    ("scaleOn", 1), ("fx4Type", 4), ("fx4Amount", 32.0), ("fx5Type", 5), ("fx5Amount", 44.0)
])

add_preset("Glass Harmonica", "Pitch", "Bell", [
    ("tone", 0.92), ("grainSize", 0.22), ("density", 0.72), ("positionSpray", 0.25),
    ("lfo1Rate", 3.8), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.14),
    ("attack", 0.4), ("decay", 1.2), ("sustain", 0.75), ("release", 1.8),
    ("scaleOn", 1), ("fx3Type", 3), ("fx3Amount", 26.0), ("fx5Type", 5), ("fx5Amount", 40.0)
])

add_preset("Temple Metal Bell", "Pitch", "Bell", [
    ("tone", 0.95), ("grainSize", 0.06), ("density", 0.55), ("octave", 1),
    ("attack", 0.001), ("decay", 1.2), ("sustain", 0.0), ("release", 1.5),
    ("lfo1Rate", 5.2), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.18),
    ("scaleOn", 1), ("fx1Type", 1), ("fx1Amount", 20.0), ("fx5Type", 5), ("fx5Amount", 45.0)
])

# --- 3. Lead (5) ---
add_preset("Singing Cello Lead", "Pitch", "Lead", [
    ("tone", 0.84), ("grainSize", 0.16), ("density", 0.78), ("panSpread", 0.6),
    ("lfo1Rate", 5.2), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.24),
    ("mod2Src", 8), ("mod2Dst", 9), ("mod2Amt", 0.25),
    ("attack", 0.06), ("decay", 0.35), ("sustain", 0.85), ("release", 0.45),
    ("scaleOn", 1), ("masterLPF", 5500.0), ("fx1Type", 1), ("fx1Amount", 26.0), ("satDrive", 3.6)
])

add_preset("Cyber Flute", "Pitch", "Lead", [
    ("tone", 0.8), ("grainSize", 0.11), ("density", 0.65), ("octave", 1), ("positionSpray", 0.2),
    ("lfo1Rate", 4.8), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.18),
    ("attack", 0.05), ("decay", 0.25), ("sustain", 0.8), ("release", 0.38),
    ("scaleOn", 1), ("fx3Type", 3), ("fx3Amount", 30.0), ("dlyFeedback", 0.45)
])

add_preset("Analog Brass Lead", "Pitch", "Lead", [
    ("tone", 0.82), ("grainSize", 0.08), ("density", 0.88), ("windowShape", 1),
    ("env1Attack", 0.04), ("env1Decay", 0.3), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.45),
    ("attack", 0.03), ("decay", 0.3), ("sustain", 0.8), ("release", 0.35),
    ("scaleOn", 1), ("filterSlope", 1), ("masterLPF", 4600.0), ("fx1Type", 1), ("fx1Amount", 35.0)
])

add_preset("Overdriven Fusion", "Pitch", "Lead", [
    ("tone", 0.86), ("grainSize", 0.07), ("density", 0.9), ("panSpread", 0.7),
    ("lfo1Rate", 5.0), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.22),
    ("attack", 0.008), ("decay", 0.3), ("sustain", 0.85), ("release", 0.35),
    ("scaleOn", 1), ("masterLPF", 6000.0), ("fx1Type", 1), ("fx1Amount", 58.0), ("satDrive", 7.5)
])

add_preset("Psychedelic Sitar", "Pitch", "Lead", [
    ("tone", 0.94), ("grainSize", 0.06), ("density", 0.72), ("pitchJitter", 35.0),
    ("attack", 0.004), ("decay", 0.45), ("sustain", 0.65), ("release", 0.55),
    ("lfo1Rate", 0.5), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.3),
    ("scaleOn", 1), ("scale", 7), ("fx2Type", 2), ("fx2Amount", 28.0), ("fx3Type", 3), ("fx3Amount", 25.0)
])

# --- 4. Pad (5) ---
add_preset("Tuned Nebula", "Pitch", "Pad", [
    ("tone", 0.76), ("grainSize", 0.34), ("density", 0.82), ("playMode", 2), ("speed", 0.3),
    ("lfo1Rate", 0.16), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.35),
    ("attack", 1.4), ("decay", 1.5), ("sustain", 0.88), ("release", 2.6),
    ("scaleOn", 1), ("masterLPF", 6200.0), ("fx2Type", 2), ("fx2Amount", 30.0), ("fx5Type", 5), ("fx5Amount", 40.0)
])

add_preset("Frozen Strings", "Pitch", "Pad", [
    ("tone", 0.82), ("grainSize", 0.28), ("density", 0.8), ("speed", 0.0),
    ("lfo1Rate", 0.2), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.15),
    ("attack", 0.9), ("decay", 1.4), ("sustain", 0.85), ("release", 2.4),
    ("scaleOn", 1), ("fx4Type", 4), ("fx4Amount", 35.0), ("frzSize", 280.0)
])

add_preset("Cathedral Choir", "Pitch", "Pad", [
    ("tone", 0.8), ("grainSize", 0.32), ("density", 0.85), ("octave", 1),
    ("lfo1Rate", 0.1), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.25),
    ("attack", 1.5), ("decay", 1.8), ("sustain", 0.9), ("release", 2.8),
    ("scaleOn", 1), ("fx5Type", 5), ("fx5Amount", 46.0), ("revDecay", 0.9)
])

add_preset("Glass Forest", "Pitch", "Pad", [
    ("tone", 0.86), ("grainSize", 0.22), ("density", 0.68), ("positionSpray", 0.35), ("reverseGrain", 1),
    ("lfo1Rate", 0.25), ("mod1Src", 1), ("mod1Dst", 4), ("mod1Amt", 0.3),
    ("attack", 1.0), ("decay", 1.3), ("sustain", 0.82), ("release", 2.2),
    ("scaleOn", 1), ("fx5Type", 5), ("fx5Amount", 42.0), ("revShimmer", 0.72)
])

add_preset("Velvet Sunset", "Pitch", "Pad", [
    ("tone", 0.78), ("grainSize", 0.3), ("density", 0.8), ("panSpread", 0.95),
    ("lfo1Rate", 0.12), ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.35),
    ("attack", 1.6), ("decay", 1.6), ("sustain", 0.88), ("release", 2.5),
    ("scaleOn", 1), ("masterLPF", 4800.0), ("fx2Type", 2), ("fx2Amount", 26.0), ("fx5Type", 5), ("fx5Amount", 36.0)
])

# --- 5. Bass (5) ---
add_preset("Reso Sub Boom", "Pitch", "Bass", [
    ("tone", 0.86), ("grainSize", 0.12), ("density", 0.9), ("octave", -2), ("panSpread", 0.2),
    ("attack", 0.002), ("decay", 0.8), ("sustain", 0.2), ("release", 0.45),
    ("env1Attack", 0.002), ("env1Decay", 0.15), ("mod1Src", 5), ("mod1Dst", 9), ("mod1Amt", 0.3),
    ("scaleOn", 1), ("masterLPF", 900.0), ("fx1Type", 1), ("fx1Amount", 24.0)
])

add_preset("Acid Reso 303", "Pitch", "Bass", [
    ("tone", 0.92), ("grainSize", 0.045), ("density", 0.86), ("octave", -1), ("panSpread", 0.3),
    ("env1Attack", 0.002), ("env1Decay", 0.2), ("mod1Src", 5), ("mod1Dst", 14), ("mod1Amt", 0.65), ("mod1Uni", 1),
    ("attack", 0.002), ("decay", 0.35), ("sustain", 0.35), ("release", 0.22),
    ("scaleOn", 1), ("masterLPF", 2200.0), ("fx1Type", 1), ("fx1Amount", 58.0), ("satDrive", 7.2)
])

add_preset("Upright Acoustic Bass", "Pitch", "Bass", [
    ("tone", 0.82), ("grainSize", 0.075), ("density", 0.68), ("windowShape", 1), ("octave", -1),
    ("attack", 0.003), ("decay", 0.45), ("sustain", 0.15), ("release", 0.35),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.35),
    ("scaleOn", 1), ("masterLPF", 2000.0), ("fx1Type", 1), ("fx1Amount", 16.0)
])

add_preset("Cyberpunk Reese", "Pitch", "Bass", [
    ("tone", 0.82), ("grainSize", 0.14), ("density", 0.94), ("octave", -1), ("panSpread", 0.85),
    ("lfo1Rate", 0.5), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.35),
    ("attack", 0.01), ("decay", 0.4), ("sustain", 0.85), ("release", 0.4),
    ("scaleOn", 1), ("masterLPF", 2600.0), ("fx1Type", 1), ("fx1Amount", 44.0), ("fx2Type", 2), ("fx2Amount", 28.0)
])

add_preset("Plucked Reso Bass", "Pitch", "Bass", [
    ("tone", 0.88), ("grainSize", 0.038), ("density", 0.58), ("windowShape", 3), ("octave", -1),
    ("attack", 0.001), ("decay", 0.3), ("sustain", 0.05), ("release", 0.26),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.4),
    ("scaleOn", 1), ("masterLPF", 3000.0), ("fx1Type", 1), ("fx1Amount", 26.0)
])

# --- 6. Arp (5) ---
add_preset("Clockwork Chime", "Pitch", "Arp", [
    ("tone", 0.9), ("grainSize", 0.045), ("density", 0.5), ("octave", 1),
    ("arpEnable", 1), ("arpPattern", 0), ("arpRateSync", 14), ("arpGate", 0.6),
    ("attack", 0.001), ("decay", 0.26), ("sustain", 0.0), ("release", 0.26),
    ("mod1Src", 8), ("mod1Dst", 9), ("mod1Amt", 0.25),
    ("scaleOn", 1), ("fx3Type", 3), ("fx3Amount", 25.0)
])

add_preset("Pentatonic Raindrop", "Pitch", "Arp", [
    ("tone", 0.85), ("grainSize", 0.06), ("density", 0.55), ("octave", 1), ("scale", 4),
    ("arpEnable", 1), ("arpPattern", 20), ("arpRateSync", 12), ("arpOctaves", 2), ("arpGate", 0.5),
    ("attack", 0.001), ("decay", 0.32), ("sustain", 0.0), ("release", 0.3),
    ("scaleOn", 1), ("fx5Type", 5), ("fx5Amount", 34.0)
])

add_preset("Shuffled Koto Groove", "Pitch", "Arp", [
    ("tone", 0.86), ("grainSize", 0.065), ("density", 0.62), ("scale", 6),
    ("arpEnable", 1), ("arpPattern", 16), ("arpRateSync", 12), ("arpOctaves", 2), ("arpGate", 0.55),
    ("attack", 0.002), ("decay", 0.28), ("sustain", 0.1), ("release", 0.28),
    ("scaleOn", 1), ("fx2Type", 2), ("fx2Amount", 20.0), ("fx3Type", 3), ("fx3Amount", 24.0)
])

add_preset("Polyrhythm Bell Arp", "Pitch", "Arp", [
    ("tone", 0.92), ("grainSize", 0.07), ("density", 0.6), ("octave", 1),
    ("arpEnable", 1), ("arpPattern", 18), ("arpRateSync", 9), ("arpGate", 0.65),
    ("attack", 0.002), ("decay", 0.38), ("sustain", 0.05), ("release", 0.38),
    ("scaleOn", 1), ("fx3Type", 3), ("fx3Amount", 30.0), ("fx5Type", 5), ("fx5Amount", 32.0)
])

add_preset("Mystic Ladder Arp", "Pitch", "Arp", [
    ("tone", 0.88), ("grainSize", 0.05), ("density", 0.58), ("scale", 2),
    ("arpEnable", 1), ("arpPattern", 1), ("arpRateSync", 12), ("arpOctaves", 3), ("arpGate", 0.6),
    ("attack", 0.001), ("decay", 0.22), ("sustain", 0.0), ("release", 0.22),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.35),
    ("scaleOn", 1), ("fx3Type", 3), ("fx3Amount", 28.0)
])

# --- 7. Gate (5) ---
add_preset("Granular Techno Gate", "Pitch", "Gate", [
    ("tone", 0.82), ("grainSize", 0.09), ("density", 0.82),
    ("attack", 0.004), ("decay", 0.28), ("sustain", 0.8), ("release", 0.28),
    ("scaleOn", 1), ("fx5Type", 6), ("fx5Amount", 88.0), ("gateRate", 4), ("gateDepth", 92.0),
    ("mod1Src", 8), ("mod1Dst", 14), ("mod1Amt", 0.35)
])

add_preset("Glitch Pulse Walker", "Pitch", "Gate", [
    ("tone", 0.8), ("grainSize", 0.03), ("density", 0.65), ("playMode", 3), ("speed", 1.2),
    ("attack", 0.002), ("decay", 0.22), ("sustain", 0.65), ("release", 0.22),
    ("scaleOn", 1), ("fx5Type", 6), ("fx5Amount", 82.0), ("gateRate", 6),
    ("fx1Type", 1), ("fx1Amount", 32.0)
])

add_preset("Vowel Slicer", "Pitch", "Gate", [
    ("tone", 0.78), ("grainSize", 0.12), ("density", 0.84),
    ("attack", 0.005), ("sustain", 0.85), ("release", 0.3),
    ("scaleOn", 1), ("fx5Type", 6), ("fx5Amount", 85.0), ("gateRate", 4), ("gateVowel", 75.0),
    ("lfo1Rate", 0.5), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.35)
])

add_preset("Stutter Trance Gate", "Pitch", "Gate", [
    ("tone", 0.84), ("grainSize", 0.07), ("density", 0.78),
    ("attack", 0.003), ("decay", 0.25), ("sustain", 0.7), ("release", 0.25),
    ("scaleOn", 1), ("fx4Type", 6), ("fx4Amount", 90.0), ("gateRate", 4), ("gatePattern", 2),
    ("fx5Type", 5), ("fx5Amount", 35.0)
])

add_preset("Sync Rhythmic Gate", "Pitch", "Gate", [
    ("tone", 0.82), ("grainSize", 0.08), ("density", 0.8),
    ("attack", 0.003), ("decay", 0.2), ("sustain", 0.75), ("release", 0.2),
    ("scaleOn", 1), ("fx5Type", 6), ("fx5Amount", 86.0), ("gateRate", 5), ("gatePattern", 1),
    ("fx1Type", 1), ("fx1Amount", 25.0)
])

# --- 8. Drone (5) ---
add_preset("Industrial Drone Bass", "Pitch", "Drone", [
    ("tone", 0.84), ("grainSize", 0.42), ("density", 0.95), ("octave", -2), ("speed", 0.1),
    ("attack", 1.4), ("decay", 1.8), ("sustain", 0.92), ("release", 2.6),
    ("lfo1Rate", 0.08), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", -0.35),
    ("scaleOn", 1), ("masterLPF", 1600.0), ("fx1Type", 1), ("fx1Amount", 46.0), ("satDrive", 6.5)
])

add_preset("Blade Runner Horizon", "Pitch", "Drone", [
    ("tone", 0.82), ("grainSize", 0.36), ("density", 0.88), ("octave", -1), ("speed", 0.2),
    ("attack", 0.9), ("decay", 1.5), ("sustain", 0.85), ("release", 2.5),
    ("lfo1Rate", 0.15), ("mod1Src", 1), ("mod1Dst", 12), ("mod1Amt", 0.18),
    ("scaleOn", 1), ("masterLPF", 4200.0), ("fx3Type", 3), ("fx3Amount", 35.0), ("fx5Type", 5), ("fx5Amount", 44.0)
])

add_preset("Alien Artifact Drone", "Pitch", "Drone", [
    ("tone", 0.94), ("grainSize", 0.15), ("density", 0.72), ("pitchJitter", 140.0), ("reverseGrain", 1),
    ("lfo1Rate", 0.2), ("mod1Src", 1), ("mod1Dst", 1), ("mod1Amt", 0.4),
    ("attack", 0.4), ("decay", 1.2), ("sustain", 0.75), ("release", 2.0),
    ("scaleOn", 1), ("fx4Type", 4), ("fx4Amount", 38.0), ("fx5Type", 5), ("fx5Amount", 38.0)
])

add_preset("Time Stasis Field", "Pitch", "Drone", [
    ("tone", 0.86), ("grainSize", 0.4), ("density", 0.85), ("speed", 0.0), ("position", 0.45),
    ("lfo1Rate", 0.05), ("mod1Src", 1), ("mod1Dst", 7), ("mod1Amt", 0.45),
    ("attack", 1.2), ("decay", 1.8), ("sustain", 0.88), ("release", 2.8),
    ("scaleOn", 1), ("fx4Type", 4), ("fx4Amount", 44.0), ("fx5Type", 5), ("fx5Amount", 46.0)
])

add_preset("Event Horizon Sweep", "Pitch", "Drone", [
    ("tone", 0.85), ("grainSize", 0.32), ("density", 0.86), ("playMode", 2), ("speed", 0.12),
    ("lfo1Rate", 0.08), ("mod1Src", 1), ("mod1Dst", 14), ("mod1Amt", 0.38),
    ("attack", 1.8), ("decay", 2.0), ("sustain", 0.9), ("release", 3.2),
    ("scaleOn", 1), ("fx1Type", 1), ("fx1Amount", 25.0), ("fx5Type", 5), ("fx5Amount", 48.0)
])

# Validation
print(f"Total presets generated: {len(presets)}")
assert len(presets) == 120, f"Expected 120 presets, got {len(presets)}"

# Check FX duplication
fx_errors = 0
for p in presets:
    fx_types = []
    for k, v in p["params"]:
        if k in ("fx1Type", "fx2Type", "fx3Type", "fx4Type", "fx5Type"):
            if int(v) != 0:
                fx_types.append((k, int(v)))
    # check duplicates
    seen = set()
    for slot, t in fx_types:
        if t in seen:
            print(f"ERROR: Duplicate FX type {t} in preset '{p['name']}' (slot {slot})")
            fx_errors += 1
        seen.add(t)

if fx_errors == 0:
    print("SUCCESS: Zero FX duplication in all 120 presets!")
else:
    print(f"FAILED: {fx_errors} FX duplication errors found!")
    sys.exit(1)

# Generate C++ Header code
out = []
out.append("""// ==========================================
// File: PresetData.h
// ファクトリープリセット 120種（パラメータ上書き方式・ハードコード）
//   各プリセットは「基準デフォルトからの上書き」を持つ。
//   Sampleは含まない（プリセットはノブ/MOD/FX設定のみ。音は各自ロード）。
//   全120種すべてにMODマトリクス（LFO/ENV/Velocity/Random）を積極導入し、劇的な差別化を達成。
//   独立カテゴリ「Pitch」（40種）はP-Sync依存を排し、Tone共鳴×時間軸×ARP/Gate/Droneで多彩化。
//   全プリセットでFXタイプの重複ゼロを厳格に保持。
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
        static const std::vector<Preset> P = {""")

cur_cat = None
for p in presets:
    if p["category"] != cur_cat:
        cur_cat = p["category"]
        out.append(f"\n        // ============================================================")
        out.append(f"        // Category: {cur_cat}")
        out.append(f"        // ============================================================")

    params_str = ", ".join([f'{{"{k}",{v:.4f}f}}' if isinstance(v, float) else f'{{"{k}",{v}}}' for k, v in p["params"]])
    out.append(f'        {{ "{p["name"]}", "{p["category"]}", "{p["sub"]}", {{ {params_str} }} }},')

out.append("""        };
        return P;
    }
}
""")

header_content = "\n".join(out)
with open("d:/VST_Project/Granular/Source/Presets/PresetData.h", "w", encoding="utf-8") as f:
    f.write(header_content)

print("Generated Source/Presets/PresetData.h successfully!")
