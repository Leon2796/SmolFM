"---
name: smolfm-advanced-techniques
description: >
  Advanced sound design techniques for SmolFM beyond basic FM recipes.
  Covers multi-stage processing, carrier layering, ADSR placement strategies,
  and hybrid FM/ring modulation approaches.  Use when the basic SKILL.md
  recipes don't achieve the desired complexity or character.
---

# SmolFM Advanced Sound Design Techniques

This document extends the basic SKILL.md with advanced patching strategies
discovered from classic FM synthesis (DX7, Synclavier) and modern modular
synthesis workflows.

## Multi-Stage Processing: ADSR Placement Strategies

The ADSR processor can be inserted at **multiple points** in the signal chain,
not just at the end. This creates different timbral effects:

### Pattern 1: Post-Carrier Envelope (Standard)
\`\`\`
note0 → fm0 → osc0 (carrier) → adsr0 → output.in1
\`\`\`
Classic approach. Envelope shapes the final amplitude.

### Pattern 2: Pre-FM Modulator Envelope (Timbre Morphing)
\`\`\`
note1 → osc1 (modulator) → adsr1 → fm0.modulator_in
note0 → fm0 → osc0 (carrier) → adsr0 → output.in1
\`\`\`
**Effect**: FM intensity varies over time.
- Fast attack on adsr1 → percussive FM \"burst\" at note start
- Slow attack on adsr1 → FM fades in gradually (evolving pad)
- **Use case**: Brass swells, evolving pads, plucked strings with FM \"thunk\"

**Example** (Brass with FM swell):
\`\`\`xml
<Node id=\"osc1\" waveform=\"0\"/>  <!-- sine modulator -->
<Node id=\"adsr1\" attack=\"0.3\" decay=\"0.2\" sustain=\"0.7\" release=\"0.2\"/>
<Wire from=\"osc1\" fromPort=\"out\" to=\"adsr1\" toPort=\"in\"/>
<Wire from=\"adsr1\" fromPort=\"out\" to=\"fm0\" toPort=\"modulator_in\"/>
\`\`\`

### Pattern 3: Dual Envelope (Independent Carrier/Modulator Envelopes)
\`\`\`
Modulator chain: note1 → osc1 → adsr1 ─┐
                                       ├→ fm0.modulator_in
Carrier chain:   note0 → fm0 → osc0 → adsr0 → output.in1
\`\`\`
**Effect**: Carrier and modulator have independent dynamics.
- Long modulator decay + short carrier decay = FM tail after note ends
- **Use case**: Bells with evolving partials, metallic percussion

### Pattern 4: Feedback Envelope (Not directly supported, but emulated)
\`\`\`
fm0.out → fscale0 (factor=0.25) → osc1 (as envelope follower)
osc1.out → ring0.in2
carrier.out → ring0.in1 → output
\`\`\`
**Workaround for**: Amplitude modulation of carrier by its own FM output
(Requires ring modulator as VCA)

---

## Sound Layering: Multi-Carrier Parallel Chains

The MasterOutputProcessor has **8 inputs**, allowing complex layered sounds
from independent synthesis chains.

### Pattern 5: Layered FM Voices (Detuned Unison)
\`\`\`
Chain 1: note0 → fm0 → osc0 → adsr0 → output.in1
Chain 2: note1 → fscale0 (1.005) → fm1 → osc1 → adsr1 → output.in2
Chain 3: note2 → fscale1 (0.995) → fm2 → osc2 → adsr2 → output.in3
\`\`\`
**Effect**: Chorus-like thickness from slight detuning.
- fscale factors: 1.0, 1.005, 0.995 (±0.5% detune) for subtle chorus
- fscale factors: 1.0, 1.02, 0.98 (±2% detune) for wide ensemble
- **Use case**: Supersaw leads, thick pads, orchestral strings

**Example** (3-voice detuned supersaw):
\`\`\`xml
<!-- Three parallel saw chains, ±0.5% detune -->
<Node id=\"fscale0\" factor=\"1.005\"/>
<Node id=\"fscale1\" factor=\"0.995\"/>
<Wire from=\"note0\" to=\"osc0\"/>  <!-- center -->
<Wire from=\"note1\" to=\"fscale0\" to=\"osc1\"/>  <!-- sharp -->
<Wire from=\"note2\" to=\"fscale1\" to=\"osc2\"/>  <!-- flat -->
<Wire from=\"osc0\" to=\"output.in1\"/>
<Wire from=\"osc1\" to=\"output.in2\"/>
<Wire from=\"osc2\" to=\"output.in3\"/>
<Node id=\"output\" level=\"0.3\"/>  <!-- reduce level to avoid clipping -->
\`\`\`

### Pattern 6: Split-Spectrum Layering (Bass + Lead)
\`\`\`
Bass:   note0 → fscale0 (0.5) → osc0 (saw, low octave) → adsr0 → output.in1
Lead:   note1 → fm0 → osc1 (sine + FM) → adsr1 → output.in2
\`\`\`
**Effect**: Independent bass and melody lines from one keyboard.
- Different fscale ratios on same note = octave splits
- **Use case**: Live performance patches, one-hand accompaniment

### Pattern 7: Transient/Sustain Split
\`\`\`
Attack:  note0 → fm0 (high amount) → osc0 → adsr0 (fast decay) → output.in1
Sustain: note1 → osc1 (pure sine) → adsr1 (slow attack, long sustain) → output.in2
\`\`\`
**Effect**: Percussive FM \"thunk\" + sustained tonal body.
- **Use case**: E-pianos, plucked basses, mallet instruments

---

## Hybrid FM + Ring Modulation

Ring modulation (`ring` processor) creates different sidebands than FM.
Combining both expands the timbral palette.

### Pattern 8: FM into Ring Mod
\`\`\`
note0 → fm0 → osc0 (FM carrier) → ring0.in1
note1 → fscale0 (3.14) → osc1 (ring modulator) → ring0.in2
ring0.out → adsr0 → output.in1
\`\`\`
**Effect**: FM creates complex base spectrum, ring mod adds metallic sidebands.
- Inharmonic ring mod frequency (e.g., 3.14:1) + harmonic FM = dense metallic texture
- **Use case**: Bells, gongs, sci-fi FX

**Example** (Metallic pad):
\`\`\`xml
<description>FM carrier ring-modulated by inharmonic ratio creates dense metallic texture</description>
<Node id=\"fm0\" amount=\"1.5\"/>
<Node id=\"osc0\" waveform=\"0\"/>  <!-- sine FM carrier -->
<Node id=\"osc1\" waveform=\"1\"/>  <!-- saw ring modulator -->
<Node id=\"fscale0\" factor=\"2.718\"/>  <!-- e:1 ratio (irrational) -->
<Node id=\"ring0\"/>
<Wire from=\"note0\" to=\"fm0\" to=\"osc0\"/>
<Wire from=\"osc0\" to=\"ring0.in1\"/>
<Wire from=\"note1\" to=\"fscale0\" to=\"osc1\"/>
<Wire from=\"osc1\" to=\"ring0.in2\"/>
<Wire from=\"ring0\" to=\"adsr0\" to=\"output.in1\"/>
\`\`\`

### Pattern 9: Parallel FM + Ring Mod
\`\`\`
FM path:   note0 → fm0 → osc0 → adsr0 → output.in1
Ring path: note1 → osc1 ─┐
                         ├→ ring0 → adsr1 → output.in2
           note2 → osc2 ─┘
\`\`\`
**Effect**: Two independent timbres mixed at output.
- FM provides tonal body, ring mod adds metallic shimmer
- **Use case**: Hybrid acoustic/electronic sounds (e.g., prepared piano)

### Pattern 10: Cascaded Ring Mods (Ring Mod of Ring Mod)
\`\`\`
osc0.out → ring0.in1
osc1.out → ring0.in2
ring0.out → ring1.in1
osc2.out → ring1.in2
ring1.out → output.in1
\`\`\`
**Effect**: Extreme inharmonicity (sum/difference of sum/difference).
- **Use case**: Noise-like FX, glitch percussion, industrial sounds

---

## Advanced FM Techniques

### Pattern 11: Asymmetric FM (Different Waveforms for Carrier/Modulator)
\`\`\`
Carrier:   osc0 waveform=\"0\" (sine)
Modulator: osc1 waveform=\"1\" (saw)
\`\`\`
**Effect**: Saw modulator creates richer sideband structure than sine.
- Saw modulator = harmonics in modulation signal = more complex FM spectrum
- Triangle modulator = softer than saw, richer than sine
- **Use case**: Leads, basses with extra \"bite\"

**Rule of thumb**:
| Carrier | Modulator | Result |
|---------|-----------|--------|
| Sine    | Sine      | Classic FM, clean sidebands |
| Sine    | Saw       | Harsher, more obertones |
| Saw     | Sine      | FM-colored saw, gritty |
| Saw     | Saw       | Very dense, can be noisy (use low FM amount) |

### Pattern 12: Sub-Octave Reinforcement
\`\`\`
Main: note0 → fm0 → osc0 → output.in1
Sub:  note1 → fscale0 (0.5) → osc1 (sine) → adsr1 → output.in2
\`\`\`
**Effect**: Adds sub-bass foundation.
- fscale=0.5 = one octave down
- Sine sub keeps low end clean (no FM artifacts)
- **Use case**: Bass patches, kick drums

### Pattern 13: Filter Emulation via FM
\`\`\`
note0 → fm0 (variable amount) → osc0 (saw) → output
\`\`\`
**Effect**: Sweep fm0.amount over time (via DAW automation) = \"filter sweep\".
- Low FM amount ≈ \"closed filter\" (few sidebands)
- High FM amount ≈ \"open filter\" (many sidebands)
- **Use case**: Acid basslines, dubstep wobbles

---

## Instrument-Specific Advanced Recipes

### E-Bass with Distortion (via Ring Mod)
\`\`\`
Clean path:  note0 → osc0 (saw) → adsr0 → output.in1
Growl path:  note1 → fscale0 (2.0) → osc1 (square) → adsr1 (fast decay) → output.in2
\`\`\`
Mix ratio: 70% clean, 30% growl → adds harmonic complexity without losing definition.

### Evolving Pad (3-Stage FM + Swell Envelope)
\`\`\`
note0 → fm0 → fm1 → fm2 → osc0 → output.in1
         ↑     ↑     ↑
        osc1  osc2  osc3
         ↑
       adsr1 (slow attack on modulator)
\`\`\`
Envelope on modulator creates \"blooming\" effect.

### Drum Kit (Kick + Snare from one patch)
\`\`\`
Kick:  note0 (low MIDI note) → osc0 (sine) → adsr0 (fast decay) → output.in1
Snare: note1 (high MIDI note) → ring0 (osc1 × osc2) → adsr1 (very fast) → output.in2
\`\`\`
Use MIDI note range to select drum type.

---

## Parameter Interaction Guidelines

### FM Amount vs. Carrier Waveform
- **Sine carrier**: FM amount 0-5 usable, sweet spot 1-3
- **Saw carrier**: FM amount 0-2 usable (higher = noise), sweet spot 0.5-1.5
- **Square carrier**: Very sensitive, sweet spot 0.3-1.0

### ADSR Timing vs. FM Complexity
- **Fast attack + high FM**: Percussive (bells, plucks)
- **Slow attack + low FM**: Smooth (pads, strings)
- **Long release + inharmonic FM**: Metallic decay (gongs, chimes)

### Layering Level Balance
When mixing multiple carriers to output:
- 2 voices: level=0.5 each
- 3 voices: level=0.33 each
- 4+ voices: level=0.25 each
Prevents clipping at MasterOutput.

---

## Debugging Harsh/Noisy Sounds

If a patch sounds harsh or \"broken\":

1. **Check FM amount on saw/square carriers**: Reduce to < 1.5
2. **Check modulator frequency**: Very high ratios (>8:1) can alias
3. **Check for clipping**: Multiple layers summed at output can exceed ±1.0
4. **Check fscale ratios**: Extreme values (>10 or <0.1) can cause issues
5. **Use sine carriers for testing**: If sine version sounds good but saw doesn't, it's a waveshaping issue, not a routing issue

---

## Creative \"Abuse\" Techniques

### Pattern 14: Feedback Oscillator (Experimental)
\`\`\`
osc0.out → fm0.modulator_in
note0 → fm0.freq_in
fm0.out → osc0.note_in
\`\`\`
**WARNING**: Creates feedback loop! May result in silence or noise depending on phase.
- Not officially supported, but can create interesting glitches
- Use adsr to tame the output

### Pattern 15: Audio-Rate Frequency Scaling
\`\`\`
note0 → osc1 (LFO, very low freq) → fscale0.freq_in
fscale0.out → osc0.note_in
\`\`\`
**Effect**: Vibrato via frequency domain instead of FM.
- fscale0.factor = 1.0 + osc1 output
- Slower than FM, more like traditional vibrato

---

## When to Update This Document

Add new patterns when:
- Users discover novel combinations
- Common requests can't be solved with basic SKILL.md recipes
- Community forums (Reddit, Gearspace) share SmolFM techniques
"