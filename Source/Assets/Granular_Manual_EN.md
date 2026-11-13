# GRANULAR Quick Manual (English)

**English** | [日本語](Granular_Manual_JP.md)

---

## Contents

1. [Make a sound](#1-make-a-sound)
2. [Layout & Operations](#2-layout--operations)
3. [MAIN tab](#3-main-tab)
4. [ENGINE tab (Phase 4 Engine & Modal Resonator)](#4-engine-tab-phase-4-engine--modal-resonator)
5. [ARP tab (Arpeggiator & Euclidean Rhythm)](#5-arp-tab-arpeggiator--euclidean-rhythm)
6. [MOD tab (Polyphonic Modulation Matrix)](#6-mod-tab-polyphonic-modulation-matrix)
7. [FX tab (Effect Rack)](#7-fx-tab-effect-rack)
8. [CONFIG tab](#8-config-tab)
9. [Preset Browser](#9-preset-browser)
10. [Tips & Troubleshooting](#10-tips--troubleshooting)

---

## 1. Make a sound

**OTODESK / 24-voice Scale-Quantized Granular Synth (VST3 / Standalone)**  
**Version:** v1.1.0 (Build 015)

GRANULAR breaks incoming audio into tiny "grains" and reconstructs them into new musical forms. Even noise or raw field recordings become fully playable instruments in perfect key, powered by a post-modulation scale quantizer, a 16-slot modulation matrix, an arpeggiator, a 5-slot FX rack, and 120 factory presets.

### Installation

1. Copy `Granular.vst3` to your system VST3 folder:
   ```
   C:\Program Files\Common Files\VST3\
   ```
2. Rescan plugins in your DAW.

### First steps

1. **Drag & drop** an audio file (WAV / AIFF / MP3 / FLAC / OGG) onto the waveform area, or click **LOAD** in the header.
2. The sample is automatically pitch-analyzed and normalized, allowing you to **play it in tune from your keyboard** (C3 = original pitch).
3. Play your MIDI keyboard to hear the sound.

> **Header Controls**
> - **PLAY DRY**: Auditions the raw sample directly (click again to stop).
> - **PANIC**: Instantly silences all active voices, reverb tails, and delay buffers if notes ever hang.
> - **RANDOM**: Musically randomizes knobs, MOD matrix (3–10 rows), and FX chain while protecting output gain, polyphony, and color theme.
> - **BROWSE**: Opens and closes the 3-column preset browser in the lower area.

---

## 2. Layout & Operations

The **dot waveform display** is permanently visible at the top (frequencies are mapped to color: warm = bass, cool = treble; grain firings dance on it in real time).  
Below are **6 tabs** that switch the lower panel:

| Tab | Contents |
|-----|----------|
| **MAIN** | Grain generator, Pitch, ADSR envelope, SAMPLE (Position/Start/End), MOTION, Master, Look-Ahead Limiter |
| **ENGINE** | Engine Mode (Granular/WSOLA/TD-PSOLA), Concatenative Atlas, Diatonic Swarm, Per-Grain SVF, Modal Resonator |
| **ARP** | Arpeggiator + Euclidean Rhythm ($E(k,n,r)$) + 82-scale Quantizer |
| **MOD** | Polyphonic Modulation sources (LFO ×4, Per-Voice Env ×3) and 16-slot matrix (77 destinations) |
| **FX** | 5-slot drag-and-drop effect rack (exclusive selection) |
| **CONFIG** | Material analysis mode, Look-ahead Limiter, Polyphony, Filter slope, Color themes, GUI scaling |

### Handy Operations
- **Window Resizing**: Drag the resize handle at the bottom-right corner to scale the plugin between **75% and 200%** with a locked aspect ratio. Window size is preserved in your DAW project.
- **Direct Value Entry**: **Right-click** any knob to type an exact numeric value.
- **Quick Reset**: **Double-click** any matrix Amount slider to quickly reset it to `0.0`.
- **Destructive Confirmation**: Actions like INIT or preset deletion prompt a confirmation box to prevent accidental loss.

---

## 3. MAIN tab

- **GRAIN**:
  - **SIZE**: Grain length (0.02s to 0.5s).
  - **DENSITY**: Grain overlap factor (0.25× to 64×, automatically normalized by 1/√overlap).
  - **SPRAY**: Random position scatter around the playhead (0.0 to 1.0).
  - **JITTER**: Random grain pitch offset (0 to 2400 cents, applied **before** scale quantization so it always stays in key).
  - **PAN**: Stereo spread (0.0 to 1.0).
  - **SPEED**: Playhead movement speed (0.0 to 2.0).
  - **WINDOW**: Envelope window shape (Hanning / Triangle / Square / Asymmetric).
  - **REV**: Toggles reverse grain playback.
- **PITCH**: OCTAVE (-3 to +3) / SEMI (-24 to +24 st) / FINE (-100 to +100 cents).
- **TONE**: Tuned Karplus-Strong comb resonator (0.0 to 1.0). Injects note-tracked acoustic resonance with soft-clipped feedback to prevent runaway bursts.
- **P-SYNC**: Pitch-synchronous grain firing. Locks grain emission intervals strictly to the played note frequency, imparting clean pitch to noise and percussion.
- **ENVELOPE**: ADSR envelope with power-curve attack (0.5ms to 10s) and exponential decay/release.
- **SAMPLE**: POSITION (playback start point) / START / END (playback boundary). Markers can also be dragged directly on the waveform. ~3ms edge fades prevent boundary clicks.
- **MOTION**: Playhead movement mode (Forward / Backward / Ping-Pong / Random).
  - **S-SYNC**: Scans the Start–End region locked to host tempo (1 beat to 8 bars).
  - **Random mode**: Rather than moving continuously, the playhead jumps randomly across the region at an interval set by SPEED (frozen at 0).
- **MASTER**: HPF / LPF (TPT state-variable filters with per-sample smoothing) / GAIN / DRY-WET (overall FX rack blend).
- **LIMITER**: CEILING (brick-wall limiter threshold at the output stage).

---

## 4. ENGINE tab (Phase 4 Engine & Modal Resonator)

Advanced synthesis and physical modeling modules that push beyond conventional granular synths:

- **ENGINE & SWARM**:
  - **ENGINE MODE**: Switch between `Granular`, `WSOLA` (normalized cross-correlation phase alignment), and `TD-PSOLA` (decoupled pitch & formants).
  - **FORMANT**: Shifts spectral formants (-12 to +12 st) independently of note pitch in TD-PSOLA mode.
  - **SWARM MIX**: Generates scale-locked harmonic grain layers with cross-voice collision avoidance.
  - **ATLAS PITCH**: Reads per-frame local F0 from the Grain Atlas so melodic samples play chromatically in tune.
- **CONCATENATIVE ATLAS**:
  - **TIMBRE / ATLAS AMT**: Uses O(log N) binary search over pre-analyzed grain timbre features (plus zero-crossing snap and adaptive onset skew).
- **GRAIN SHAPE & PER-GRAIN SVF**:
  - **SKEW / CHIRP**: Skews grain windows continuously from percussive snap to reverse swell, and applies intra-grain pitch glides (±12 st).
  - **FILTER MODE / CUTOFF / RESONANCE / FLT JITTER**: Per-grain Zavalishin TPT SVF (Off/LP/BP/HP/Notch) with random cutoff jitter.
- **SCALE-TUNED MODAL RESONATOR BANK**:
  - **RATIO MODE / MODAL MIX / DECAY / DAMPING / INHARM**: 8 stereo damped TPT modal resonators tuned to `Scale`, `Harmonic`, `Marimba`, `Bell`, or `Plate` modes with T60 damping and stiffness dispersion.

---

## 5. ARP tab (Arpeggiator & Euclidean Rhythm)

### Arpeggiator
- **Pattern**: 13 patterns (Up, Down, Up-Down, Down-Up, Up-Down (Incl), Converge, Diverge, Pedal Low, Pedal High, As Played, Random, Random Walk, Chord).
- **Rate**: Tempo sync (1/1 to 1/32, dotted & triplets) or free rate (0.5 to 30 Hz).
- **Octaves**: Range across 1 to 4 octaves.
- **Duration / Gate**: Note length mode (Gate % or fixed 1/32 to 1/4 note length).
- **Latch**: Holds the arpeggiated sequence after keys are released.
- **Euclidean Rhythm (EUCLID / STEPS / FILLS / ROT)**: Orthogonal Bjorklund step gating (2–16 steps, 1–16 fills, 0–15 rotation).

### Scale Quantizer
Controls the unique post-modulation scale snapping engine:
- **SCALE QUANTIZE (Toggle)**: Enables or bypasses scale snapping.
- **Key**: Root key (Auto from audio analysis, or fixed C to B).
- **Scale**: **82 musical scales** across curated categories.
- **ScaleNote Display**: Displays active scale notes in real time (e.g. `C, D, E, F, G, A, B`) based on selected Key and Scale.

---

## 6. MOD tab (Polyphonic Modulation Matrix)

Configure sources on the left sub-tabs and route them via the **16-slot matrix** on the right (Page 1 / Page 2).

### Sources (13 total)
- **LFO ×4**: 6 shapes (Sine / Triangle / Saw / Square / S&H / RK4 Lorenz Chaos). Free (0.02–20 Hz) or tempo-synced (1/1–1/32).
- **Env ×3 (Polyphonic)**: Each of the 24 voices advances its own ADSR state independently. Enable **LOOP** to cycle Attack-Decay as a complex custom LFO.
- **MIDI Sources**: Velocity / Note / Mod Wheel / Random / Aftertouch (Poly Pressure & MPE CC74).

### 16-Slot Matrix (77 destinations)
- Each row sets: **Source × Amount → Destination**.
- **True Polyphonic Evaluation**: Per-voice Velocity, Note, Env, Random, and Aftertouch independently modulate voice parameters per note!
- **Quantization Safety**: Pitch destinations (Octave, Semi, Fine, Jitter) are summed *before* the scale quantizer, guaranteeing that even extreme modulation remains consonant.

---

## 7. FX tab (Effect Rack)

5-slot serial effect chain. Drag slot headers to reorder the signal chain.

### Included Effects (6 types)
1. **Saturation (ADAA)**: Anti-derivative anti-aliased saturation with **10 algorithms** (Tape, Tube, Diode, Transformer, etc.), pre-HPF, and output trim.
2. **Ensemble Chorus**: Rich stereo modulation (Rate, Depth, Width).
3. **Tape Delay**: Tempo-synced delay with input ducking (Duck) and damping filter.
4. **Freeze**: Granular buffer hold up to 1000ms with feedback and damping.
5. **Shimmer Reverb**: 16-channel FDN reverb with an octave pitch shifter.
6. **FormantGate**: Formant vowel filter with rhythmic stutter slicing (Rate, Pattern, Depth, Decay, Vowel).

> **Exclusive Slot Selection**  
> An effect active in one slot is automatically disabled (greyed out) in all other slots to prevent redundant processing. The RANDOM button also uses a collision-free shuffle.

---

## 8. CONFIG tab

- **MATERIAL**: Audio analysis profile (Auto / Percussive / Melodic / Vocal). Click **ANALYZE** in the header after changing.
- **LIMITER RELEASE**: Look-ahead brick-wall limiter (~1.5 ms, reported via host PDC) release time (1 to 500 ms).
- **POLY**: Maximum polyphony (1 to 24 voices).
- **FILTER SLOPE**: Master filter steepness (12 dB/oct or 24 dB/oct).
- **COLOR THEME**: 10 color themes (Midnight, Sakura, Ocean, Forest, Sunset, Mono, Neon, Vaporwave, Amber, Arctic).

---

## 9. Preset Browser

Click **BROWSE** in the header to open the **3-column hierarchical browser**:

### 3-Column Structure
1. **Left Column "CATEGORY"**:
   - `All` (all presets)
   - `Favorites ★` (starred favorites only)
   - 9 Factory Categories (`Pads & Ambient`, `Textures`, `Rhythmic`, `Bass`, `Leads`, `Cinematic`, `Glitch`, `Experimental`, `Pitch`)
   - `User` (user-created presets)
2. **Center Column "SUBCATEGORY"**:
   - Dynamically lists subcategories belonging to the active category (starts with "All"). Click to instantly filter the preset list.
3. **Right Column "PRESET"**:
   - Filtered preset list. Click to load instantly. Step through with ▲▼ buttons or keyboard ↑↓ keys.

### Star Favorites (★)
- Click the **★ / ☆ icon** at the left edge of any preset row to immediately toggle favorite status with one click.
- Also accessible via the **right-click context menu** ("Mark Favorite ★ / Remove from Favorites").

### Saving User Presets
1. Click **SAVE** in the browser header to open the modal dialog.
2. Select or enter a **Category**, type a **Subcategory**, enter the **Preset Name**, and click Save.
3. User presets are saved as `.xml` files in `Documents / OTODESK / Granular / Presets /` and automatically appear under their respective categories.
4. User presets can be deleted via right-click with a confirmation prompt.

> **Sample References**: Presets do not embed heavy audio files; they record the **sample's file path**. If the file exists on disk, it is automatically reloaded in the background.

### 120 Factory Presets
Completely revised in B011 with extensive MOD matrix routing (LFO/ENV/Velocity/Random) for deep sonic differentiation.
- **Existing 80 presets**: Across Pads, Textures, Rhythmic, Bass, Leads, Cinematic, Glitch, Experimental.
- **New "Pitch" Category (40 presets)**:
  Organized into 8 dedicated subcategories (5 presets each) emphasizing Tone comb resonance and granular timing:
  - **Pluck (5)**: Crisp acoustic pick attacks and percussive plucks.
  - **Bell (5)**: Metallic chime hardness, fine modulation, shimmer reverb.
  - **Lead (5)**: Smooth vibrato solos with expressive dynamics.
  - **Pad (5)**: Lush, evolving harmonic textures with slow LFO sweeps.
  - **Bass (5)**: Solid low-end fundamentals with bite.
  - **Arp (5)**: Integrated arpeggiator sequences.
  - **Gate (5)**: FormantGate and rhythmic stutter slices.
  - **Drone (5)**: Infinite harmonic sustain and dark reverberant beds.

---

## 10. Tips & Troubleshooting

- **Turn noise into lush pads**: Load ambient noise, set TONE to ~0.5, Size to 0.2s, Density to 0.7, enable SCALE QUANTIZE, and pick a scale.
- **Wild pitch modulation in tune**: Route LFO 1 to `Semi` or `Fine` at full Amount. Extreme pitch sweeps will remain locked to the selected scale.
- **Breathing tempo textures**: Set Motion to Ping-Pong, turn S-SYNC on with Scan = 2 Bars, and modulate Filter or Density with a synced LFO.
- **Stuck notes?**: Press the **PANIC** button in the header to instantly flush all voice buffers and reset sound.

### Troubleshooting
- **High CPU or audio dropouts**: Lower **POLY** in CONFIG (e.g. 24 → 12) or lower **DENSITY** in MAIN.
- **Color theme only partially applied**: Reopen the plugin window to ensure all cached graphics refresh completely.

---

## License

This software is released under the **AGPLv3** license.

Time-stretching is powered by **Signalsmith Stretch** (MIT License):
- Signalsmith Stretch — Copyright © 2022 Geraint Luff / Signalsmith Audio Ltd.
- Signalsmith Linear — Copyright © 2025 Signalsmith Audio

---

**Developer:** @kijyoumusic (OTODESK)  
[X (Twitter)](https://x.com/kijyoumusic) | [Official Website](https://otodesk4193.github.io/OTODESK_SITE/)
