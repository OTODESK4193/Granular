# GRANULAR — Quick Manual (English)

**OTODESK / 24-voice Scale-Quantized Granular Synth (VST3 / Standalone)**

Granular is a synth that breaks a loaded sound into tiny "grains" and rebuilds it. Even noise or field recordings can be played as a pitched instrument, with modulation, arpeggiator, effects, and 80 factory presets.

---

## 1. Make a sound

1. **Drag & drop** an audio file (WAV / AIFF / MP3 / FLAC / OGG) onto the waveform area, or use the **LOAD** button in the header.
2. On load, the sample is automatically pitch-analyzed and normalized, so you can **play it in tune from your keyboard** (C3 = the sample's original pitch).
3. Play a MIDI keyboard to hear it.

> **PLAY DRY**: auditions the loaded sample as-is (press again to stop).

---

## 2. Layout

The **dot waveform** is always shown at the top. Its colors come from a frequency analysis of the sample (warm = low, cool = high), and grains dance on it in real time while playing.
Below are **5 tabs** that switch the lower section.

| Tab | Contents |
|-----|----------|
| **MAIN** | Grain, Pitch, ADSR envelope, POSITION/START/END, MOTION, Master |
| **ARP** | Arpeggiator + Quantize (Key/Scale) + Texture |
| **MOD** | Modulation (LFO / Env / MACRO → 16-slot matrix) |
| **FX** | 5-slot effects chain |
| **CONFIG** | Analysis mode, Limiter, Polyphony, Filter, Color Theme |

> **Right-click a knob** to type an exact value. Destructive actions (DELETE / RESET, etc.) ask for confirmation.

---

## 3. MAIN tab

- **GRAIN**: SIZE (grain length) / DENSITY / SPRAY (position scatter) / JITTER (pitch wobble) / PAN / SPEED
- **PITCH**: OCTAVE / SEMI / FINE
- **ENVELOPE**: A / D / S / R (amplitude over time)
- **SAMPLE**: POSITION (playback point) / START / END (used region). You can also drag the markers on the waveform
- **MOTION**: how the playhead moves (Forward / Backward / Ping-Pong / Random). **S-SYNC** scans the region in sync with host tempo
- **MASTER**: HPF / LPF / GAIN / DRY-WET (overall FX mix)
- **LIMITER**: CEILING (output ceiling)

---

## 4. ARP tab

Automatically arpeggiates the notes you hold. 13 patterns, tempo sync (dotted/triplet) or free (Hz), Gate (note length), octave range, and Latch (hold without keeping keys down). Output notes snap to the current **Key / Scale**.

---

## 5. MOD tab (Modulation)

The left side is the **sources** (sub-tabs):

- **LFO ×4**: 6 shapes (Sine / Tri / Saw / Sqr / S&H / Chaos). SYNC locks to tempo
- **Env ×3**: ADSR. Turn on LOOP to make it cycle A-D as a complex LFO
- **MACRO ×4**: one knob that drives several destinations at once

The right side is the **16-slot matrix** (Page 1 / Page 2):

- Each row: **Source × Amount → Destination**
- **Amount** resets to 0 on double-click. The **UNI/BI** toggle chooses unipolar or bipolar
- Modulated destination knobs show the modulation range as a **white arc**, and Position / Spray / Start / End are reflected on the waveform

**Using macros**: e.g., add two rows — "Source=Macro 1, Dest=Position, Amt=+0.5" and "Source=Macro 1, Dest=Spray, Amt=-0.5". Now one Macro 1 knob moves Position up and Spray down together. A single macro can be assigned to up to all 16 rows.

---

## 6. FX tab

Place any of these into the 5 slots (reorder by dragging the slot headers):

Saturation / Chorus / Tape Delay / Freeze / Shimmer Reverb. Selecting a slot reveals its detail parameters below. The overall wet/dry balance is the **DRY-WET** knob on MAIN.

---

## 7. CONFIG tab

- **MATERIAL**: analysis mode (Auto / Percussive / Melodic / Vocal). After changing, press **ANALYZE** in the header to re-analyze
- **LIMITER RELEASE**: limiter release time
- **POLY**: maximum voices (1–24)
- **FILTER SLOPE**: master filter steepness (12 / 24 dB/oct)
- **COLOR THEME**: 10 themes (reopen the plugin window for full effect)

---

## 8. Presets

Click **BROWSE** in the header to open the browser in the lower area.

- **80 factory presets** across 8 categories (Pads & Ambient / Textures / Rhythmic / Bass / Leads & Keys / Cinematic / Glitch & IDM / Experimental), from orthodox to wild
- Pick a category on the left, filter with the search box
- **Click a row to load instantly**; use the **▲▼ buttons** or **keyboard ↑↓** to step through
- **Right-click**: add/remove Favorite, or delete a user preset (with confirmation)
- Starred (★) items collect under the Favorites category

**RANDOM** (in the header, left of BROWSE): randomizes knobs, MOD (3–10 slots), and FX — great for happy accidents.

**Saving**: type a name at the top of the browser and press **SAVE**. User presets are stored as `.xml` in
`Documents / OTODESK / Granular / Presets /`.

> **About samples**: presets do not embed audio; they store the **sample's file path**. If the original file still exists, it is reloaded automatically when you load the preset.

---

## 9. Tips

- **Want a clear pitch?** Raise TONE or turn on P-SYNC (Pitch Sync). Even noisy material becomes musical
- **Want movement?** In MOD, route LFO → Position (grains fly across the waveform), or Env(Loop) → Size/Density
- **Want rhythm?** Use the ARP, or LFO(SYNC) → Density/Position
- **Want width?** Raise PAN and add a little Shimmer Reverb
- Even with extreme settings, internal multi-stage safety (soft-clip / limiter) protects the output

Have fun sculpting sound!
