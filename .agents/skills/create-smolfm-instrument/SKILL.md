---
name: smolfm-instrument-designer
description: >
  Expert skill for designing and generating sophisticated SmolFM synthesizer instruments
  (.smolfm files). Combines foundational and advanced FM sound-design techniques,
  creative modular routing, parameter reasoning, layering, and patch documentation.
  Use when creating, analyzing, refining, or explaining SmolFM instruments. Every generated
  instrument must include a concise sound description, the core synthesis principle,
  a readable patch implementation/wiring explanation, and the most musically interesting
  parameters with rationale.
---

# SmolFM Instrument Designer — Unified Expert Skill

## Mission

Create the **best possible instrument for the requested or can creative sound**, not merely a technically
valid graph. But keep the instrument simple if a the user request to just demonstrate a certain aspect. 
Treat the `.smolfm` patch as both an executable DSP graph and a human-readable
sound-design recipe.

The skill combines:
- the basic SmolFM processor reference and proven instrument recipes,
- advanced multi-stage FM, layering, ADSR placement, and hybrid FM/ring-mod techniques,
- deliberate parameter selection instead of random values,
- concise documentation of **what the instrument is, why it works, and how the patch realizes it**.

The LLM/agent is responsible for musical intent and creative choices. The patch itself is
responsible for deterministic DSP structure. Never invent processors, ports, or file-format
features that are not documented here.

**File placement:** every generated `.smolfm` file goes into the repository under
`instruments/<name>.smolfm`. Use kebab-case (lowercase, hyphens), e.g.
`instruments/fm-acid-bass.smolfm`. Do not propose paths outside the repo — the
patch browser defaults to `instruments/` and every patch must be committable.

---

# Mandatory Output Contract

Whenever generating a `.smolfm` instrument, produce **all four layers**:

## 1. Kurzbeschreibung — What is it?

One or two concise sentences describing the audible identity.

Example:
> Ein dunkler, metallischer Drone mit weichem Grundton, langsam aufblühenden Seitenbändern
> und einem langen, leicht inharmonischen Nachhall.

## 2. Prinzip der Idee — Why does it sound this way?

Explain the synthesis mechanism in musical/DSP terms:
- carrier choice,
- modulator choice,
- frequency ratios,
- FM amount,
- ring modulation if used,
- envelope strategy,
- layering/detuning,
- expected harmonic vs. inharmonic behavior.

## 3. Umsetzung als Patch — How is it built?

Describe the actual signal graph before or alongside the XML.

Use readable notation such as:

```text
Note → Frequency Scale → Modulator Oscillator
                         ↓
Note → FM Carrier Input → FM → Carrier Oscillator → ADSR → Output
```

Then generate the complete `.smolfm` XML.

## 4. Spannende Parameter — What should the user explore?

Always identify **3–7 interesting parameters**. For each, state:
- parameter,
- current value,
- what changing it does,
- useful direction/range.

Prioritize musically meaningful controls such as:
- FM amount,
- FM ratio,
- inharmonic ratio,
- modulator envelope attack/decay,
- carrier/modulator waveform,
- detune amount,
- carrier/modulator layer balance,
- envelope decay/release,
- master level.

Do not merely list every parameter. Highlight the parameters that actually change the character.

---

# Design Philosophy

## A. Start from the desired sound

Translate the request into:
1. **Sound identity** — e.g. bell, glass, rubber, brass, drone, bass, machine, shimmer.
2. **Spectral identity** — pure, harmonic, bright, dense, inharmonic, noisy.
3. **Temporal identity** — pluck, impact, swell, sustain, evolving tail.
4. **Pitch behavior** — tonal, stable, ratio-based, deliberately inharmonic.
5. **Spatial/thickness behavior** — mono, layered, detuned, wide/ensemble-like.
6. **Special character** — metallic, hollow, aggressive, soft, digital, organic.

Then choose the smallest patch that can express those properties. Add complexity only when
it contributes audible character.

## B. Prefer intentional complexity

A sophisticated patch is not one with many nodes. It is one where each stage has a purpose.

Good:
- FM creates the initial attack spectrum.
- A second envelope controls FM intensity.
- A parallel sine supplies a stable body.
- Ring modulation adds controlled metallic sidebands.

Bad:
- arbitrary oscillators,
- arbitrary ratios,
- excessive FM,
- layers without level management.

## C. Parameter values must have a reason

Never choose parameters randomly. Ratios determine partial structure; FM amount determines
sideband density; envelopes determine temporal evolution; detuning determines thickness.

---

# Sound-Design Decision Matrix

| Desired character | First technique | Parameters to explore |
|---|---|---|
| Pure / fundamental | Sine carrier | FM 0–0.3, long sustain |
| Warm / rounded | Triangle or low-FM sine | FM 0.1–1, slower attack |
| Bright / harmonically rich | Saw carrier | FM 0.5–1.5 |
| Bell / chime | Inharmonic FM | ratios ~3–4+, fast attack, decay |
| Metallic | FM + ring mod | irrational ratios, moderate/high FM |
| Thick / wide | Parallel carriers | detune ±0.5–2% |
| Evolving | Envelope on modulator | slow attack / long decay |
| Percussive | High FM + fast envelope | fast attack, short decay |
| Brass-like | Saw + light FM | 0.5–1.5 FM, slow swell |
| Bass growl | Saw/square + FM | 0.5–1.5 FM, ratio 1–2 |
| Sub bass | Sine | little/no FM |
| Tremolo | Audio/LFO-rate AM | ~4–8 Hz, AM 0.2–0.5 |
| Sci-fi / experimental | Ring mod / cascaded ring | inharmonic ratios |

---

# Reference Knowledge

Use these references in this priority order when acoustic intuition is missing:

1. **Patch & Tweak** — modular patching and creative signal routing.
2. **Synth Secrets** — synthesis theory and why a recipe works.
3. **Welsh's Synthesizer Cookbook** — reconstructing recognizable sounds.
4. **The Synthesizer** — broad terminology and synthesis reference.
5. **Designing Sound** — physical/acoustic sound-design approaches.
6. **Electronic Music and Sound Design** — systematic theory and practice.

Known starting points from the source skill:
- Bell FM ratio ≈ 1:3.5.
- Marimba ≈ 1:4.
- E-piano ≈ 1:1 plus 1:14.
- Tremolo ≈ 4–8 Hz, AM depth 0.2–0.5.
- Ring modulation with inharmonic/irrational ratios creates metallic sidebands.
- Kick: high initial pitch dropping rapidly toward the body frequency.
- Snare: inharmonic FM and/or ring-modulated high-frequency content with a short envelope.

Use these as starting points, not immutable presets.

---

## When to Use This Skill

Use this skill whenever:
- The user requests creation of a new instrument or sound
- You need to generate a `.smolfm` file from a description
- The user asks for specific timbres (bells, pianos, basses, pads, drums, etc.)
- You need to understand the available processors and their capabilities

**Always update this skill** when:
- New processor types are added to the codebase
- Processor input/output types change
- New ports are added to existing processors
- The .smolfm file format changes
- Maximum instance counts change in `GraphNodeRegistry`

## Available Types

### Port Types

| Type | Description | Carries | Default Value |
|------|-------------|---------|---------------|
| `signal` | Audio-rate signal | Amplitude samples | 0.0f |
| `frequency` | Frequency in Hertz | Pitch/frequency values | 440.0f |

### Processor Roles

| Role | Purpose |
|------|---------|
| `oscillator` | Waveform generation (sine, saw, square, triangle, wavetables) |
| `fmModulator` | True frequency modulation in Hertz domain |
| `frequencyScale` | Frequency multiplication/transposition |
| `adsr` | Envelope shaping |
| `masterOutput` | Final mixing and level control |
| `generic` | Other processors |

## Available Processors

### Complete Processor Reference

| Processor | Base ID | Max Instances | Inputs | Outputs | Function |
|-----------|---------|---------------|--------|---------|----------|
| **NoteProcessor** | `note` | 1 | _(none)_ | `out` (frequency) | Converts MIDI note number to frequency in Hz using equal temperament; single source — one `note` node can feed many consumers |
| **OscillatorProcessor** | `osc` | 8 | `note_in` (frequency) | `out` (signal) | Generates waveform at frequency from note_in port; supports sine, saw, square, triangle and white noise |
| **FMModulationProcessor** | `fm` | 4 | `freq_in` (frequency), `modulator_in` (signal) | `out` (frequency) | True FM: scales carrier frequency by modulator signal; chainable in Hertz domain |
| **FrequencyScaleProcessor** | `fscale` | 8 | `freq_in` (frequency) | `out` (frequency) | Multiplies frequency by constant factor; useful for transposition and harmonic series |
| **RingModulatorProcessor** | `ring` | 4 | `in1` (signal), `in2` (signal) | `out` (signal) | Multiplies two signals sample-wise; creates sum/difference sidebands for metallic timbres |
| **AmProcessor** | `am` | 4 | `carrier_in` (signal), `modulator_in` (signal) | `out` (signal) | Amplitude modulation with depth control; biased modulator keeps carrier audible |
| **DelayProcessor** | `delay` | 8 | `in` (signal) | `out` (signal) | Digital delay with feedback and mix; free ms or tempo-synced to host BPM by note division |
| **AdsrProcessor** | `adsr` | 8 | `in` (signal) | `out` (signal) | Applies ADSR envelope to signal; multiplies input by envelope value and velocity |
| **GainProcessor** | `gain` | 8 | `in` (signal) | `out` (signal) | Stateless boost/attenuate: multiplies input by constant factor (0-10, 1.0 = transparent, negative inverts phase); use for layer balancing, pre-envelope drive, or per-voice level trims |
| **WaveshaperProcessor** | `shape` | 8 | `in` (signal) | `out` (signal) | Distortion via three transfer functions (Soft/Hard/Fold) with drive control; use for saturation warmth, grit, or metallic foldback textures |
| **FilterProcessor** | `filter` | 8 | `in` (signal), `cutoff_mod_in` (frequency, optional) | `out` (signal) | Multi-mode SVF (LP/BP/HP/Notch) with static cutoff + resonance; optional frequency input replaces the cutoff entirely (envelope sweeps, auto-wah, risers) |
| **FAdsrProcessor** | `fadsr` | 4 | `freq_in` (frequency) | `out` (frequency) | Pitch envelope in the frequency domain: scales input frequency between down/up factors along an ADSR envelope; classic pitch-envelope for kick/808/drops or evolving FM sweeps |
| **MasterOutputProcessor** | `output` | 1 | `in1`-`in8` (signal, 8 inputs) | _(none, final output)_ | Sums up to 8 signal inputs with master level control and peak metering |

### Processor Port Details

#### NoteProcessor
- **Purpose**: MIDI note → frequency converter
- **Inputs**: None
- **Outputs**: `out` (frequency) - emits frequency in Hz for played MIDI note
- **Parameters**: None (controlled by MIDI input)

#### OscillatorProcessor
- **Purpose**: Waveform generation (carrier, modulator, or LFO)
- **Inputs**: `note_in` (frequency) - frequency source (0 Hz if unconnected = silent; ignored in LFO mode)
- **Outputs**: `out` (signal) - raw oscillator sample
- **Parameters**:
  - `osc%Waveform` - waveform selector (sine, saw, square, triangle, noise)
    - noise = white noise, uniform [-1, 1] per sample; frequency-invariant
    - great for snare/cymbal textures and breathy layers
  - `osc%Mode` - frequency mode selector (0 = Pitch, 1 = Static, 2 = LFO)
    - Pitch (default): frequency follows the note_in port
    - Static: note_in is a GATE ONLY — the oscillator fires while a note is
      connected AND playing, always at the fixed `osc%StaticFreq` (20-20000
      Hz); it does NOT follow the keyboard and IGNORES any processed
      frequency arriving on note_in (see Antipattern below)
    - LFO: same gate behaviour at the fixed low rate from `osc%LfoRate`
      (0.01-50 Hz)
  - `osc%StaticFreq` - static frequency in Hz 20-20000 (default 440; Static mode)
  - `osc%LfoRate` - LFO rate in Hz 0.01-50 (default 1.0; LFO mode)
- **Mode behaviour**: Static and LFO ignore note_in entirely — the oscillator
  becomes a fixed-frequency source wired into ANY processor path
  (`fm.modulator_in`, `am.modulator_in`, etc.).  Note-on re-syncs the phase
  so per-note modulation starts consistently.  Noise waveform in Static/LFO
  mode = per-sample random values (a fixed-frequency noise bed).

#### FMModulationProcessor
- **Purpose**: True frequency modulation (not phase modulation!)
- **Inputs**:
  - `freq_in` (frequency) - carrier base frequency
  - `modulator_in` (signal) - modulating signal
- **Outputs**: `out` (frequency) - instantaneous modulated frequency
- **Parameters**:
  - `fmAmount%` - modulation index (0-1 range; 1 = ±100% deviation)
- **Behavior**: `f_out = f_in * (1 + amount * modulator)`
- **Chainable**: Output can feed another FM stage's freq_in or an oscillator's note_in

#### FrequencyScaleProcessor
- **Purpose**: Frequency multiplication
- **Inputs**: `freq_in` (frequency)
- **Outputs**: `out` (frequency) = `freq_in * factor`
- **Parameters**:
  - `fscale%Factor` - multiplication factor (0-10 range; 1.0 = transparent)
- **Use cases**: Octave transposition (2.0), harmonic ratios, mute (0.0)

#### RingModulatorProcessor
- **Purpose**: Signal multiplication (ring modulation)
- **Inputs**: `in1` (signal), `in2` (signal)
- **Outputs**: `out` (signal) = `in1 * in2`
- **Parameters**: None
- **Behavior**: Creates sum and difference sidebands; no carrier residue (classic diode ring mod behavior)

#### AmProcessor
- **Purpose**: Amplitude modulation with depth control
- **Inputs**:
  - `carrier_in` (signal) - the audio to be modulated
  - `modulator_in` (signal) - bipolar modulator ([-1, 1]), typically an oscillator or LFO-style source
- **Outputs**: `out` (signal) - amplitude-modulated audio
- **Parameters**:
  - `am%Amount` - modulation depth (0.0-1.0 range; 0 = dry passthrough, 1 = full modulation)
- **Behavior**: `out = carrier * (1 - amount + amount * (mod + 1))`
  - Maps the bipolar modulator into a positive gain factor [1 - amount, 1 + amount]
  - At `amount = 0`, the modulator has no effect
  - At `amount = 1`, gain spans [0, 2] - carrier fades to silence if modulator hits -1
  - Unwired `modulator_in` reads as 0 → `out = carrier` (clean bypass)
- **Use when**: You want tremolo, periodic level variation, or a "chopper" effect without losing the carrier entirely (unlike ring modulation, which suppresses the carrier)

#### DelayProcessor
- **Purpose**: Digital delay line with feedback and tempo synchronisation
- **Inputs**: `in` (signal) - audio to delay
- **Outputs**: `out` (signal) - dry + wet mix
- **Parameters**:
  - `delay%TimeMs` - free-running delay time in milliseconds (1-2000 ms, default 250)
  - `delay%Feedback` - feedback amount 0.0-0.95 (default 0.3)
  - `delay%Mix` - dry/wet balance 0-1 (default 0.3)
  - `delay%SyncMode` - bool: false = free ms, true = sync to host BPM
  - `delay%Division` - note division 0-3 = 1/2, 1/4, 1/8, 1/16; only used in sync mode
- **Behavior**:
  - Free mode: delay length = `TimeMs * sampleRate / 1000`
  - Sync mode: delay length = `(60000 / bpm) * factor`, factor ∈ {2.0, 1.0, 0.5, 0.25}
  - `out[n] = (1 - mix) * in[n] + mix * delayLine[n]`; delay line is fed back
  - `in[n] + feedback * delayLine[n]` each sample
  - The buffer persists across notes (effect tails ring out)
  - Falls back to 120 BPM if the host reports no tempo
- **Use when**: You want echoed repeats, rhythmic doubling, ping-pong-style
  tails, or tempo-locked delays that follow the host transport
- **Limit**: 8 instances (~380 KB buffer each at 48 kHz; parallel delays for
  stereo spread and rhythmic subdivision patterns are affordable now)

#### AdsrProcessor
- **Purpose**: Envelope shaping
- **Inputs**: `in` (signal)
- **Outputs**: `out` (signal) = `in * envelope * velocity`
- **Parameters** (APVTS IDs use `adsr%` prefix + parameter name):
  - `adsr%Attack` - attack time (seconds)
  - `adsr%Decay` - decay time (seconds)
  - `adsr%Sustain` - sustain level (0-1)
  - `adsr%Release` - release time (seconds)

#### GainProcessor
- **Purpose**: Constant gain stage (boost or attenuate)
- **Inputs**: `in` (signal)
- **Outputs**: `out` (signal) = `in * gain`
- **Parameters**:
  - `gain%Factor` - gain factor 0-10 (default 1.0 = transparent; negative not exposed via UI but valid)
- **Behavior**: stateless, no envelope, no per-note state; unwired input reads silence
- **Use cases**: layer balancing before output stacking, drive boost into FM amount, per-voice level trims, attenuating noisy layers

#### WaveshaperProcessor
- **Purpose**: Distortion via selectable transfer function
- **Inputs**: `in` (signal)
- **Outputs**: `out` (signal) = `f(x * (1 + k*drive))`, k = 9 (Soft/Hard), 19 (Fold)
- **Parameters**:
  - `shape%Drive` - drive amount 0-1 (default 0.0; scales input into the curve)
  - `shape%Shape` - function selector (0 = Soft, 1 = Hard, 2 = Fold)
- **Behavior**: stateless; `drive = 0` with Soft is near-transparent (identity at small amplitudes)
  - **Soft** (`x / (1 + |x|)` after drive): smooth tube-like saturation; keeps
    original dynamics. Best for: warm e-piano/bass saturation, taming bright
    FM patches, adding body without harshness. Low drive = subtle glue.
  - **Hard** (brick-wall clamp after drive): aggressive digital distortion;
    dense harmonic buildup, fast decay of harmonics. Best for: acid bass,
    aggressive leads, lo-fi grit, drum-machine-style snarl.
  - **Fold** (foldback around ±1): reflects the waveform instead of clamping;
    inharmonic, metallic sidebands. Best for: metallic percussion, gongs,
    chaotic textures, dub-siren screech. High drive (0.6+) gets wild —
    attenuate afterwards with a `gain` node.
- **Pairing tip**: put a `gain` node *before* the shaper to push more level
  into the curve, and one *after* to tame the output.

#### FilterProcessor
- **Purpose**: Multi-mode state-variable filter (TPT/trapezoidal, always stable)
- **Inputs**:
  - `in` (signal) - audio to filter
  - `cutoff_mod_in` (frequency, optional) - live cutoff in Hertz.  When
    connected it **replaces** the static cutoff entirely (no offset/blend);
    any frequency producer (fadsr sweep, fm chain, keyboard via fscale) can
    drive it.  Unconnected = the static cutoff parameter is used alone.
- **Outputs**: `out` (signal) - filtered audio
- **Parameters**:
  - `filter%Cutoff` - static cutoff 20-20000 Hz (log scale), default 1000;
    the base/fallback cutoff
  - `filter%Resonance` - resonance 0-1, default 0; sharpens the cutoff;
    near 1 approaches self-oscillation (stays stable)
  - `filter%Mode` - 0 = LP, 1 = BP, 2 = HP, 3 = Notch
- **Behavior**: state-variable filter, coefficients re-derived per sample so
  cutoff sweeps are click-free.  `startNote()` re-zeros the integrators.
- **Use when**: bass growl (cutoff envelope on attack), dark pads brightening
  via LFO (auto-wah), noise risers (LP with rising cutoff), tonal shaping
  that FM cannot fake, removing harsh FM sidebands (LP/Notch after the osc)
- **Antipattern**: a `cutoff_mod_in` wire from a Static/LFO osc ignores the
  pitch chain — the mod input wants a FREQUENCY in Hz, so feed it from
  `note`/`fadsr`/`fm`/`fscale` outputs only (see Antipattern section above)
- **XML attributes**: `cutoff` (Hz), `resonance` (0-1), `mode` (0-3)

#### FAdsrProcessor
- **Purpose**: Pitch envelope in the frequency domain (F-ADSR)
- **Inputs**: `freq_in` (frequency) - incoming frequency in Hz (typically `note.out` or an FM stage output)
- **Outputs**: `out` (frequency) = `freq_in * F(E)`, where E is the ADSR envelope value (0-1)
- **Parameters** (APVTS IDs use `fadsr%` prefix + parameter name):
  - `fadsr%Attack` - attack time (seconds, default 0.01)
  - `fadsr%Decay` - decay time (seconds, default 0.2)
  - `fadsr%Sustain` - sustain level (0-1, default 0.0)
  - `fadsr%Release` - release time (seconds, default 0.5)
  - `fadsr%Up` - scaling factor applied at E = 1 (0-10, default 1.0)
  - `fadsr%Down` - scaling factor applied at E = 0 (0-10, default 1.0)
- **Behavior**: `f_out = f_in * (down + (up - down) * E)`
  - Linear interpolation between the two factors along the envelope
  - `up = down = 1.0` is fully transparent
  - `down = 0.0` + short attack = classic pitch rise (kick/808 style)
  - `up > 1.0` + fast attack/decay = pitch "bling" above played pitch that falls back
  - Release segment moves the pitch during the note-off tail
- **Use when**: You want kick pitch-drops, 808-style pitch glides, plucked pitch
  transients, FM sweeps driven by the note (timbre follows pitch envelope), or
  octave-jump attacks that decay back to playing pitch
- **Chainable**: Output is a frequency, so it can feed `osc.note_in`, another `fadsr.freq_in`, or `fm.freq_in`

#### MasterOutputProcessor
- **Purpose**: Final mixing stage
- **Inputs**: `in1` through `in8` (signal) - 8 parallel inputs
- **Outputs**: None (feeds audio output directly)
- **Parameters**:
  - `masterLevel` - overall volume control

## .smolfm File Format

### XML Structure

A `.smolfm` file is an XML document with this hierarchy:

```xml
<?xml version="1.0" encoding="UTF-8"?>

<!-- Optional comment describing the instrument -->
<SmolFM version="2" name="Instrument Name">
  <description>DSP structure explanation in one sentence</description>
  <Nodes>
    <Node id="nodeId" x="100" y="100" param1="value" param2="value">
      <Pin id="portId" direction="in|out" type="signal|frequency"/>
      <!-- more pins -->
    </Node>
    <!-- more nodes -->
  </Nodes>
  <Connections>
    <Wire from="sourceNodeId" fromPort="sourcePortId" to="destNodeId" toPort="destPortId"/>
    <!-- more wires -->
  </Connections>
</SmolFM>
```

### Required Elements

1. **Root `<SmolFM>` element**
   - `version` attribute: always `"2"`
   - `name` attribute: human-readable instrument name (optional, defaults to filename)

2. **`<description>` element** ⚠️ **REQUIRED in generated files**
   - Contains a one-sentence explanation of the DSP structure and instrument concept
   - Should clarify how the processors interact to create the desired sound
   - Helps users understand the design at a glance

3. **`<Nodes>` container**
   - Contains all processor nodes

4. **`<Connections>` container**
   - Contains all wiring between nodes

### Node Element Structure

Each `<Node>` element defines one processor instance:

**Attributes:**
- `id` (required): Instance identifier following pattern `baseId + index`
  - Examples: `note`, `osc0`, `osc1`, `fm0`, `fscale0`, `adsr0`, `gain0`, `output`
  - Single-instance nodes omit index: `note`, `output` (not `note`)
  - Single-instance nodes omit index: `output` (not `output0`)
- `x`, `y` (required): Canvas position integers (visual layout)
- **Processor-specific parameter attributes** (as many as needed):
  - Oscillator: `waveform` (integer index), `mode` (int: 0=Pitch, 1=Static, 2=LFO), `staticfreq` (float Hz), `lforate` (float Hz)
  - FM: `amount` (float)
  - FrequencyScale: `factor` (float)
  - Gain: `factor` (float)
  - Waveshaper: `drive` (float), `shape` (int: 0=Soft, 1=Hard, 2=Fold)
  - ADSR: `attack`, `decay`, `sustain`, `release` (floats)
  - MasterOutput: `level` (float)

**Child elements:**
- `<Pin>` elements for each port (input and output)
  - `id`: port identifier (e.g., `out`, `note_in`, `freq_in`, `modulator_in`, `in`, `in1`-`in8`)
  - `direction`: `"in"` or `"out"`
  - `type`: `"signal"` or `"frequency"`

### Node ID Convention

| Base ID | Instance IDs | Count |
|---------|--------------|-------|
| `note` | `note` (no index) | 0 only |
| `osc` | `osc0` through `osc7` | 0-7 |
| `fm` | `fm0` through `fm3` | 0-3 |
| `fscale` | `fscale0` through `fscale7` | 0-7 |
| `ring` | `ring0` through `ring3` | 0-3 |
| `am` | `am0` through `am3` | 0-3 |
| `delay` | `delay0` through `delay7` | 0-7 |
| `adsr` | `adsr0` through `adsr7` | 0-7 |
| `gain` | `gain0` through `gain7` | 0-7 |
| `shape` | `shape0` through `shape7` | 0-7 |
| `filter` | `filter0` through `filter7` | 0-7 |
| `fadsr` | `fadsr0` through `fadsr3` | 0-3 |
| `output` | `output` (no index) | 0 only |

### Connection Element Structure

Each `<Wire>` element connects one output port to one input port:

**Attributes:**
- `from`: Source node ID
- `fromPort`: Source output port ID
- `to`: Destination node ID
- `toPort`: Destination input port ID

**Rules:**
- Ports must have matching types (signal ↔ signal, frequency ↔ frequency)
- Each input port can have only **one** source (enforced by design)
- Output ports can feed multiple inputs
- Unconnected inputs use their default value (0.0 for signal, 440.0 for frequency)

## Graph Wiring Principles

### ⚠️ Antipattern: Processed Frequency into a Static/LFO Oscillator

**Static and LFO mode IGNORE the frequency value arriving on `note_in`** —
the port is only a gate (fire while a note plays). Wiring an FM/F-ADSR/
frequency-scale chain into a Static/LFO osc **silently discards the whole
pitch chain**: the envelope and modulation stages compute a frequency that
never reaches the oscillator.

```text
WRONG — the fadsr/fm work is thrown away:
  note → fadsr0 → fm0 → osc0.note_in   (osc0.mode = Static)  ✗

RIGHT — pitch chain needs a Pitch-mode osc:
  note → fadsr0 → fm0 → osc0.note_in   (osc0.mode = Pitch)   ✓

RIGHT — fixed-frequency body needs no pitch chain at all:
  note → osc0.note_in                  (osc0.mode = Static,
                                        osc0.staticfreq = 120)  ✓
```

Rule of thumb: **a Static/LFO osc takes exactly ONE wire — `note.out →
osc.note_in`** (the gate). Any second wire into `note_in` (from `fm.out`,
`fadsr.out`, `fscale.out`) signals the pitch chain was meant for a Pitch
oscillator: either switch the osc to Pitch mode, or delete the dead chain.
Also never wire two sources into one input — an InputPort holds exactly one
source (last wire wins, the other is silently ignored).

### Signal Flow Patterns

#### Basic Oscillator Chain
```
note.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Single FM Stage
```
note.out → fm0.freq_in          (carrier frequency)
osc1.out → fm0.modulator_in      (modulator signal)
fm0.out → osc0.note_in           (modulated frequency to carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Chained FM Stages (Multiple Modulators)
```
note.out → fm0.freq_in
osc1.out → fm0.modulator_in      (first modulator)
fm0.out → fm1.freq_in            (FM chain in Hertz domain)
osc2.out → fm1.modulator_in      (second modulator)
fm1.out → osc0.note_in           (final FM chain to carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### FM with Modulator Tracking (Scales with Keyboard)
```
note.out → fm0.freq_in          (carrier frequency)
note.out → fscale0.freq_in      (modulator tracks keyboard)
fscale0.factor = 3.5             (modulator/carrier ratio 3.5:1)
fscale0.out → osc1.note_in       (scaled frequency to modulator)
osc1.out → fm0.modulator_in      (modulator signal)
fm0.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Ring Modulation
```
note.out → osc0.note_in         (carrier)
note.out → fscale0.freq_in      (modulator tuning)
fscale0.out → osc1.note_in
osc0.out → ring0.in1             (carrier to ring mod)
osc1.out → ring0.in2             (modulator to ring mod)
ring0.out → adsr0.in
adsr0.out → output.in1
```
#### Amplitude Modulation (Tremolo)
Classic AM with an LFO-style slow modulator. The amount knob sets how much the volume wobbles - the carrier always stays audible, unlike ring modulation which can suppress it entirely.

Use for: Rhythmic tremolo, vintage vibrato, slow swells.

```
note.out  -> osc0.note_in         (carrier at played pitch)
note.out  -> fscale0.freq_in      (LFO rate; tracks keyboard down several octaves)
fscale0.out -> osc1.note_in
osc0.out    -> am0.carrier_in
osc1.out    -> am0.modulator_in
am0.out     -> adsr0.in
adsr0.out   -> output.in1
```
Example settings:
- `fscale0.factor = 0.05` (modulator ~4-5 octaves below the root, 5-8 Hz)
- `am0.amount = 0.3` (gentle tremolo; raise to 0.8 for a chopper effect)

#### Amplitude Modulation (Audio Rate, Harmonic Sidebands)
Modulate at a pitch derived from the played note so the sidebands stay glued to the carrier - the carrier pitch remains identifiable instead of dissolving into inharmonic ring-mod noise.

Use for: Thicker pads, animated leads, bell tones that keep their fundamental.

```
note.out  -> osc0.note_in         (carrier)
note.out  -> fscale0.freq_in      (modulator tuned to a ratio like 1.5 or 2.03)
fscale0.out -> osc1.note_in
osc0.out    -> am0.carrier_in
osc1.out    -> am0.modulator_in
am0.out     -> adsr0.in
adsr0.out   -> output.in1
```
Example settings:
- `fscale0.factor = 1.5` (modulator at a perfect fifth above)
- `am0.amount = 0.6` (clear sidebands, carrier still dominant)

#### AM as Sidechain-Style Ducker
Use a slow oscillator as a periodic ducking source for a carrier. This is the synth equivalent of a sidechain compressor keyed by an LFO.

```
note.out  -> osc0.note_in         (melody carrier)
note.out  -> fscale0.freq_in      (slow LFO: fscale factor 0.02-0.1)
fscale0.out -> osc1.note_in
osc1.out    -> am0.modulator_in
osc0.out    -> am0.carrier_in
am0.out     -> adsr0.in
adsr0.out   -> output.in1
```
Set `am0.amount` near 1.0 and the LFO swings between 0 and 2x gain - the carrier rhythmically fades out. Add an `adsr0` on the modulator if you want the ducking to ease in rather than start abruptly.

#### Simple Delay (Free-Running)
Any voice routed through a delay creates classic echo repeats. Good default for pads, leads, ambient plucks.

```
note.out  -> osc0.note_in
osc0.out    -> adsr0.in
adsr0.out   -> delay0.in
delay0.out  -> output.in1
adsr0.out   -> output.in2   (send dry copy to output too for level)
```
Example settings:
- `delay0.time = 350` (long echo)
- `delay0.feedback = 0.35`
- `delay0.mix = 0.25` (subtle tail)

#### Tempo-Synced Delay (Follows Host BPM)
Sync mode locks the delay to note divisions of the host tempo, so repeats land on the beat. Use for rhythmic patterns tied to the track.

```
note.out  -> osc0.note_in
osc0.out    -> adsr0.in
adsr0.out   -> delay0.in
delay0.out  -> output.in1
```
Example settings:
- `delay0.sync = 1`
- `delay0.division = 2` (1/8 note)
- `delay0.feedback = 0.4`
- `delay0.mix = 0.3`

#### Ping-Pong via Two Parallel Delays
Route the same source into two delays with different times/divisions and send them to two different output inputs. The result is a wide, staggered tail.

```
note.out  -> osc0.note_in
osc0.out    -> adsr0.in
adsr0.out   -> delay0.in
adsr0.out   -> delay1.in
delay0.out  -> output.in1   (e.g. 1/4)
delay1.out  -> output.in2   (e.g. 1/8)
```
Example settings:
- `delay0: sync=1, division=1 (1/4), feedback=0.5, mix=0.5`
- `delay1: sync=1, division=2 (1/8), feedback=0.3, mix=0.5`

#### Parallel Voices (Multiple Carriers)
```
note.out → osc0.note_in         (voice 1)
note.out → osc1.note_in         (voice 2, same pitch)
osc0.out → adsr0.in
osc1.out → adsr1.in
adsr0.out → output.in1           (mix into output)
adsr1.out → output.in2           (separate envelope per voice)
```

#### Pitch Drop (Kick / 808 Body)
The classic pitch envelope: tone starts low (factor 0) and glides up to the
played pitch over the attack time. Sustain 0 keeps the pitch stable at the
envelope's decay end.

```
note.out → fadsr0.freq_in       (pitch source into the envelope)
fadsr0.out → osc0.note_in        (pitch-enveloped frequency to carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `fadsr0.down = 0.0` (start at 0 Hz)
- `fadsr0.up = 1.0` (settle at played pitch)
- `fadsr0.attack = 0.03`, `fadsr0.decay = 0.1`, `fadsr0.sustain = 0.0`
- `fadsr0.release = 0.1`

#### Pitch Bling / Pluck Transient
Tone strikes above (or below) the played pitch and decays back to it. The
played pitch stays the musical reference; the transient is a timbral accent.

```
note.out → fadsr0.freq_in
fadsr0.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `fadsr0.up = 2.0` (strike one octave above)
- `fadsr0.down = 1.0` (settle at played pitch)
- `fadsr0.attack = 0.001`, `fadsr0.decay = 0.15`, `fadsr0.sustain = 0.0`

#### FM Sweep via Pitch Envelope
The pitch envelope sits **after** the FM stage and scales the already-modulated
carrier pitch, so the timbre sweeps with the envelope: bright/dense at the
attack peak, settling into a stable timbre. Never wire two sources into one
input — an InputPort holds exactly one source (last wire wins, the other is
silently ignored). Modulator still tracks the keyboard via fscale.

```
note.out → fm0.freq_in          (carrier frequency)
note.out → fscale0.freq_in      (modulator tracks keyboard)
fscale0.out → osc1.note_in
osc1.out → fm0.modulator_in
fm0.out → fadsr0.freq_in         (envelope AFTER the FM stage, not parallel)
fadsr0.out → osc0.note_in        (enveloped pitch lands on the carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `fadsr0.up = 2.5`, `fadsr0.down = 1.0`
- `fadsr0.attack = 0.4`, `fadsr0.decay = 1.5`, `fadsr0.sustain = 0.0`
- `fscale0.factor = 2.0`

#### Waveshaper as Warm Saturation (E-Piano / Bass Body)
Soft shape with low drive adds tube-style warmth and glues a patch together
without changing its identity. Great as the last stage before the output.

Use for: E-piano, bass body, taming bright FM, analogue glue.

```
note.out → fm0.freq_in
note.out → fscale0.freq_in      (modulator tracks keyboard)
fscale0.out → osc1.note_in
osc1.out → fm0.modulator_in
fm0.out → osc0.note_in
osc0.out → shape0.in            (saturation after the oscillator)
shape0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `shape0.shape = 0` (Soft)
- `shape0.drive = 0.15` (subtle glue; raise to 0.35 for more bite)

#### Waveshaper as Acid Grit (Bass / Lead)
Hard shape with high drive turns a plain saw into an aggressive acid/digital
snarl. Put the shaper *before* the amplitude envelope so the envelope still
shapes the distorted timbre.

Use for: acid bass, aggressive leads, lo-fi grit.

```
note.out → fm0.freq_in
note.out → fscale0.freq_in
fscale0.out → osc1.note_in
osc1.out → fm0.modulator_in
fm0.out → osc0.note_in
osc0.out → shape0.in            (distort first)
shape0.out → adsr0.in           (envelope shapes the grit)
adsr0.out → output.in1
```
Example settings:
- `shape0.shape = 1` (Hard)
- `shape0.drive = 0.7` (dense harmonics)
- `fm0.amount = 0.9` (growl under the distortion)

#### Waveshaper Foldback Percussion (Gong / Metallic)
Fold shape at extreme drive creates inharmonic sidebands — the raw material
for metallic percussion. A gain node after the shaper tames the level.

Use for: gongs, metallic hits, chaotic dub textures.

```
note.out → osc0.note_in
osc0.out → shape0.in
shape0.out → gain0.in           (tame the wild foldback output)
gain0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `shape0.shape = 2` (Fold)
- `shape0.drive = 0.8` (extreme folding)
- `gain0.factor = 0.4` (compensate the hot output)
- `adsr0.attack = 0.001`, `adsr0.decay = 2.5` (long metallic tail)

#### LFO Oscillator: Vibrato on FM Pitch
An LFO-mode oscillator sweeps the carrier frequency slowly. Note-triggered:
every note restarts the vibrato cycle, so the wobble lands identically on
each key. Wire the LFO output into an fscale stage to scale the deviation
with the keyboard.

Use for: vibrato leads, singing pads, siren effects.

```
note.out → fm0.freq_in          (carrier frequency)
note.out → fscale0.freq_in      (scales LFO deviation with the keyboard)
fscale0.out → osc1.note_in      (LFO source; osc1 in LFO mode)
osc1.out → fm0.modulator_in     (LFO signal bends the FM pitch)
fm0.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `osc1.mode = 2` (LFO), `osc1.lforate = 5.5` (singing vibrato ~5 Hz)
- `osc1.waveform = 0` (sine — smooth pitch wobble)
- `fm0.amount = 0.4` (vibrato depth)
- `fscale0.factor = 1.0` (constant deviation; lower = less wobble on low notes)

#### LFO Oscillator: Rhythmic Tremolo Gate
An LFO-mode oscillator drives an AM modulator input at audio-below rate —
periodic volume pulsing that restarts on every note. Noise waveform gives a
scratchy texture instead of smooth pulsing.

Use for: trance gates, dub chops, pulsing pads.

```
note.out → osc0.note_in         (melody carrier)
osc0.out → am0.carrier_in
osc1 (LFO mode, square, ~8 Hz).out → am0.modulator_in
am0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `osc1.mode = 2` (LFO), `osc1.lforate = 8.0` (gate rate)
- `osc1.waveform = 2` (square — hard on/off gating)
- `am0.amount = 1.0` (full depth; drops to silence at LFO troughs)
- `adsr0.sustain = 1.0` (let the LFO do the dynamics)

#### LFO Oscillator: Noise-Triggered Texture Bursts
Noise waveform in LFO mode emits a fresh noise burst on every note — breathy
texture layers, percussion fills, or chaotic modulation sources.

Use for: breath layers on pads, percussion fills, riser noise.

```
note.out → osc2.note_in         (wired only to trigger the note re-sync;
                                 note_in ignored in LFO mode)
osc2 (LFO mode, noise).out → fm0.modulator_in   (noise modulates FM timbre)
...or: osc2.out → am0.modulator_in              (noise texture on amplitude)
osc0.out → adsr0.in
adsr0.out → output.in1
```
Example settings:
- `osc2.mode = 2` (LFO), `osc2.lforate = 7.0` (burst rate)
- `osc2.waveform = 4` (noise)
- `fm0.amount = 1.5` (chaotic timbre modulation from noise)
- Pair with a slow attack adsr for "breath" pads

### Type Safety

Connections are only valid when port types match:

| Connection | Valid? | Reason |
|------------|--------|--------|
| `note.out` → `osc.note_in` | ✓ | frequency → frequency |
| `osc.out` → `fm.modulator_in` | ✓ | signal → signal |
| `fm.out` → `osc.note_in` | ✓ | frequency → frequency |
| `fm.out` → `fm.freq_in` | ✓ | frequency → frequency (chaining) |
| `osc.out` → `osc.note_in` | ✗ | signal → frequency (type mismatch) |
| `note.out` → `osc.out` | ✗ | output → output (direction wrong) |

#### Filter: Static Cutoff (Simple Tone Shaping)
A filter without a modulation source works purely from its static cutoff —
darken a bright FM patch, notch out a nasal resonance, or tame hats.

Use for: tone shaping on any signal path, gentle analog-style warmth.

```
note.out -> osc0.note_in
osc0.out   -> filter0.in             (cutoff_mod_in left unconnected)
filter0.out -> adsr0.in
adsr0.out   -> output.in1
```
Example settings:
- `filter0.mode = 0` (LP), `filter0.cutoff = 1200` (rounds harsh FM)
- `filter0.resonance = 0.2` (gentle character; 0.6+ for a wah peak)
- Notch (mode 3) with cutoff 800 removes boxy/nasal tones

#### Filter: Envelope Cutoff Sweep (Bass Growl / Riser)
A pitch envelope (F-ADSR) drives the cutoff directly — the filter opens on
the attack and settles back.  The classic bass growl: filter closed low,
sweeping open over ~50 ms on each note.

Use for: bass growl, acid squelch, riser noise, percussive opens.

```
note.out -> fadsr0.freq_in          (envelope in the frequency domain)
fadsr0.out -> filter0.cutoff_mod_in (envelope output IS the cutoff in Hz)
osc0.out -> filter0.in
filter0.out -> adsr0.in
adsr0.out -> output.in1
```
Example settings:
- `filter0.mode = 0` (LP), `filter0.resonance = 0.4`
- `fadsr0.up = 4.0`, `fadsr0.down = 1.0` — cutoff sweeps 4x the base
- Base cutoff via `fadsr0` input from a fixed fscale, e.g.
  `fscale0.factor = 0.018` (440 Hz -> ~200 Hz base cutoff)
- `fadsr0.attack = 0.05`, `fadsr0.decay = 0.3`, `fadsr0.sustain = 0.0`

#### Filter: LFO Auto-Wah (Breathing Pads)
A slow LFO sweeps the cutoff so the sound breathes.  Derive the LFO rate
from the note via fscale so the rate stays pitch-relative.

Use for: evolving pads, dub chops, moving noise textures.

```
note.out -> fscale0.freq_in         (scales LFO rate with the keyboard)
fscale0.out -> osc1.note_in         (osc1 = LFO mode, sine)
osc1.out -> filter0.cutoff_mod_in   (LFO output IS the cutoff)
osc0.out -> filter0.in
filter0.out -> adsr0.in -> output.in1
```
Example settings:
- `osc1.mode = 2` (LFO), `osc1.lforate = 2.0`, `osc1.waveform = 0` (sine)
- `fscale0.factor = 0.1` (keeps the sweep slow on low notes)
- `filter0.mode = 0` (LP), `filter0.resonance = 0.5` (audible wah)
- Pair with a slow-attack adsr for classic house pad movement

## Instrument Design Recipes

### Bells / Glocken

**DSP Concept**: Inharmonic FM with non-integer sideband ratios creates bell-like partial structure.

**Key ingredients:**
- Multiple FM stages with irrational frequency ratios (e.g., 3.55:1, 1.19:1)
- Fast attack, long exponential decay
- Moderate FM amounts to avoid excessive sidebands

**Example:**
```xml
<!-- FM Bell: cascaded FM with irrational ratios -->
<description>Double FM chain (fm0→fm1) with inharmonic modulator ratios (3.55:1 and 1.19:1) creates bell-like partials; modulators track keyboard via fscale for pitch stability.</description>
<Nodes>
  <Node id="note" x="60" y="60">
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="osc0" x="1020" y="60" waveform="0">
    <Pin id="note_in" direction="in" type="frequency"/>
    <Pin id="out" direction="out" type="signal"/>
  </Node>
  <Node id="osc1" x="1020" y="380" waveform="0">
    <Pin id="note_in" direction="in" type="frequency"/>
    <Pin id="out" direction="out" type="signal"/>
  </Node>
  <Node id="osc2" x="1020" y="700" waveform="0">
    <Pin id="note_in" direction="in" type="frequency"/>
    <Pin id="out" direction="out" type="signal"/>
  </Node>
  <Node id="fm0" x="700" y="60" amount="2.5">
    <Pin id="freq_in" direction="in" type="frequency"/>
    <Pin id="modulator_in" direction="in" type="signal"/>
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="fm1" x="700" y="380" amount="1.2">
    <Pin id="freq_in" direction="in" type="frequency"/>
    <Pin id="modulator_in" direction="in" type="signal"/>
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="adsr0" x="1340" y="60" attack="0.001" decay="1.8" sustain="0.0" release="1.5">
    <Pin id="in" direction="in" type="signal"/>
    <Pin id="out" direction="out" type="signal"/>
  </Node>
  <Node id="note" x="60" y="380">
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="note" x="60" y="700">
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="fscale0" x="380" y="60" factor="3.5545"/>
  <Node id="fscale1" x="380" y="380" factor="1.1886"/>
  <Node id="output" x="1660" y="60" level="0.8"/>
</Nodes>
<Connections>
  <Wire from="note" fromPort="out" to="fm0" toPort="freq_in"/>
  <Wire from="osc1" fromPort="out" to="fm0" toPort="modulator_in"/>
  <Wire from="fm0" fromPort="out" to="fm1" toPort="freq_in"/>
  <Wire from="osc2" fromPort="out" to="fm1" toPort="modulator_in"/>
  <Wire from="fm1" fromPort="out" to="osc0" toPort="note_in"/>
  <Wire from="osc0" fromPort="out" to="adsr0" toPort="in"/>
  <Wire from="note" fromPort="out" to="fscale0" toPort="freq_in"/>
  <Wire from="fscale0" fromPort="out" to="osc1" toPort="note_in"/>
  <Wire from="note" fromPort="out" to="fscale1" toPort="freq_in"/>
  <Wire from="fscale1" fromPort="out" to="osc2" toPort="note_in"/>
  <Wire from="adsr0" fromPort="out" to="output" toPort="in1"/>
</Connections>

### Piano / E-Piano

**DSP Concept**: Single FM stage with harmonic ratios (1:1, 2:1, 3:1); moderate modulation index creates realistic attack transients; fast attack with natural decay.

**Key ingredients:**
- FM ratio close to integers (1:1 up to 4:1)
- Moderate FM amount (1.0-3.0)
- Fast attack (0.001-0.01s), medium decay (0.3-1.5s), low sustain, short release
- Modulator tracks keyboard for consistent timbre

**Example parameters:**
- `fm0.amount = 2.0`
- `fscale0.factor = 1.0` to `3.0` (harmonic ratios)
- `adsr0`: attack=0.001, decay=0.8, sustain=0.2, release=0.3

**Wiring pattern:**
```
note → fm0.freq_in
note → fscale0 → osc1.note_in  (modulator with ratio)
osc1.out → fm0.modulator_in
fm0.out → osc0.note_in  (carrier)
osc0.out → adsr0 → output.in1
```

### Brass / Bläser

**DSP Concept**: Saw wave with light FM for brightness; slow attack simulates breath build-up; moderate FM adds characteristic \"bite\".

**Key ingredients:**
- Saw waveform for carrier AND modulator
- Light to moderate FM (0.5-1.5)
- Slower attack (0.05-0.15s), short decay, high sustain (0.7-0.9)
- Integer ratios (1:1 or 2:1) for harmonic spectrum

**Advanced variant**: Add ADSR envelope on the modulator for \"swell\" effect:
```
note → fscale0 → osc1 (modulator) → adsr1 (slow attack) → fm0.modulator_in
```
This creates an evolving brass sound where FM intensity builds up gradually.

**Example parameters:**
- `osc0.waveform = 1` (saw)
- `osc1.waveform = 1` (saw)
- `fm0.amount = 0.8`
- `fscale0.factor = 1.0` or `2.0`
- `adsr0`: attack=0.08, decay=0.2, sustain=0.8, release=0.2
- `adsr1` (optional, on modulator): attack=0.3, decay=0.2, sustain=0.7, release=0.2

**Multi-layer option**: Detune 2-3 of these patches by ±0.5% using `fscale` for ensemble thickness.

### Snare Drum

**DSP Concept**: Inharmonic FM or ring modulation for noise-like character; very short envelope; high frequency content.

**Key ingredients:**
- High FM amount (3-6) with inharmonic ratios
- OR: Ring modulation between two oscillators
- Very fast attack (0.001s), very short decay (0.05-0.15s), no sustain
- Optional: mix with sine \"body\" oscillator

**Wiring pattern (FM-based):**
```
note (fixed pitch ~200Hz) → fm0.freq_in
fm0.amount = 5.0 (high)
osc1 (not tracking, or ratio like 3.7:1) → fm0.modulator_in
fm0.out → osc0.note_in
osc0.out → adsr0 (attack=0.001, decay=0.1, sustain=0, release=0.05)
adsr0 → output.in1
```

**Wiring pattern (Ring mod-based):**
```
osc0 (body, ~180Hz) → ring0.in1
osc1 (noise-like, ratio 4.2:1) → ring0.in2
ring0.out → adsr0 (very short envelope) → output.in1
```

### Kick Drum

**DSP Concept**: Sine wave with pitch envelope (frequency drop); very fast attack; sub-bass frequencies.

**Key ingredients:**
- Sine wave carrier at low frequency (~50-60 Hz)
- Very fast pitch drop (using fscale with low factor on note, or just fixed low note)
- Very fast attack (0.001s), short-medium decay (0.2-0.4s), no sustain
- Optional: light FM for click transient

**Simple version:**
```
note (fixed ~MIDI note 36-40) → osc0.note_in (sine)
osc0.out → adsr0 (attack=0.001, decay=0.3, sustain=0, release=0.1) → output.in1
```

**With click:**
```
note → fm0.freq_in
note → fscale0 (factor=5.0) → osc1 (modulator)
osc1.out → fm0.modulator_in
fm0.amount = 0.3 (subtle)
fm0.out → osc0.note_in
osc0.out → adsr0 → output.in1
```

### Bass (Electric/Synth Bass)

**DSP Concept**: Saw or square wave with optional FM for growl; fast attack; moderate sustain for sustained notes.

**Key ingredients:**
- Saw or square waveform
- Optional FM for \"growl\" character
- Fast attack (0.001-0.01s), medium decay, high sustain (0.7-0.9)
- FM ratio 1:1 or 2:1 for harmonics

**Clean bass:**
```
note → osc0 (saw) → adsr0 (attack=0.005, decay=0.2, sustain=0.8, release=0.15) → output.in1
```

**Growl bass:**
```
note → fm0.freq_in
note → fscale0 (factor=2.0) → osc1 (saw modulator)
osc1.out → fm0.modulator_in
fm0.amount = 0.7
fm0.out → osc0 (saw carrier)
osc0.out → adsr0 → output.in1
```

### Subbass / Sub-Bass

**DSP Concept**: Pure sine wave at very low frequencies; minimal harmonic content; smooth envelope.

**Key ingredients:**
- Sine wave only
- No FM or very subtle FM
- Attack 0.001-0.01s, sustain high (0.9-1.0), long release
- Fixed low tuning or normal keyboard tracking

**Wiring:**
```
note → osc0 (sine) → adsr0 (attack=0.005, decay=0.1, sustain=0.95, release=0.3) → output.in1
```

### 808 Bass

**DSP Concept**: Sine wave with pitch envelope dropping one octave; long decay; characteristic \"boom\" sound.

**Key ingredients:**
- Sine wave carrier
- Moderate sustain for held notes
- Long decay (0.5-1.5s) for the famous \"boom\"
- Optional subtle FM for tonal variation

**Wiring:**
```
note → osc0 (sine) → adsr0 (attack=0.001, decay=1.0, sustain=0.6, release=0.4) → output.in1
```

**Advanced 808 with FM tail:**
```
note → fm0.freq_in
osc1 (subtle modulator, ratio 1:1, sine) → fm0.modulator_in
fm0.amount = 0.15 (very subtle)
fm0.out → osc0.note_in
osc0.out → adsr0 (attack=0.001, decay=1.2, sustain=0.5, release=0.5) → output.in1
```

### Ambient Pads

**DSP Concept**: Multiple detuned oscillators mixed together; slow attack; long release; complex FM for evolving timbre.

**Key ingredients:**
- 2-4 oscillators in parallel
- Slight detuning (using fscale with factors like 1.0, 1.005, 0.995)
- Very slow attack (0.5-2.0s), long decay, high sustain, long release (2-5s)
- Saw or triangle waves
- Optional FM for movement

**Example (3-osc pad):**
```xml
<Nodes>
  <Node id=\"note\"/>
  <Node id=\"osc0\" waveform=\"1\"/>  <!-- saw -->
  <Node id=\"osc1\" waveform=\"1\"/>
  <Node id=\"osc2\" waveform=\"2\"/>  <!-- triangle -->
  <Node id=\"fscale0\" factor=\"1.005\"/>  <!-- slight detune -->
  <Node id=\"fscale1\" factor=\"0.995\"/>
  <Node id=\"adsr0\" attack=\"1.0\" decay=\"2.0\" sustain=\"0.7\" release=\"3.0\"/>
  <Node id=\"output\" level=\"0.3\"/>
</Nodes>
<Connections>
  <Wire from=\"note\" to=\"osc0\"/>
  <Wire from=\"note\" to=\"fscale0\" to=\"osc1\"/>  <!-- detuned + -->
  <Wire from=\"note\" to=\"fscale1\" to=\"osc2\"/>  <!-- detuned - -->
  <Wire from=\"osc0\" to=\"adsr0\"/>
  <Wire from=\"osc1.out\" to=\"output.in2\"/>
  <Wire from=\"osc2.out\" to=\"output.in3\"/>
  <Wire from=\"adsr0\" to=\"output.in1\"/>
</Connections>
```

**With FM for movement:**
```
note → fm0.freq_in
fm0.amount = 0.2 (subtle)
osc3 (LFO-like, ratio 0.5:1 using fscale) → fm0.modulator_in
fm0.out → osc0.note_in (main pad osc)
+ parallel detuned oscillators
```

### Advanced Instrument Concepts

Für komplexere Instrumente siehe [SKILL-ADVANCED.md](SKILL-ADVANCED.md):

**Multi-Layer E-Piano** (DX7-style):
- Layer 1: FM percussive attack (fast decay)
- Layer 2: Sustained sine body
- Layer 3: Optional detuned shimmer

**Hybrid Bass** (Clean + Growl):
- Path A: Saw bass (clean) → output.in1
- Path B: FM growl (high amount, fast decay) → output.in2
- Mix ratio: 70% clean, 30% growl

**Evolving Pad** (3-stage FM + Swell):
- FM modulator mit eigener ADSR (slow attack)
- Erzeugt \"blooming\" Effekt über Zeit

**Split Keyboard** (Bass + Lead):
- Bass: note → fscale (0.5) → osc (low octave)
- Lead: note → fm → osc (melody)
- Verschiedene MIDI-Noten → verschiedene Rollen

### Additional Instrument Ideas

**Metallic Pads / FX:**
- Ring modulation between two oscillators with inharmonic ratios
- Long envelopes
- Multiple ring mod stages
- **NEW**: Combine FM + Ring Mod for dense metallic textures:
  ```
  note → fm → osc0 (FM carrier) → ring0.in1
  note → fscale (3.14:1) → osc1 (ring mod) → ring0.in2
  ring0.out → adsr → output
  ```
  FM creates harmonic sidebands, ring mod adds sum/difference inharmonics.

**Mallets (Marimba, Vibraphone):**
- Similar to bells but with harmonic ratios (4:1, 10:1 for marimba)
- Faster decay than bells
- Less FM amount

**Plucked Strings:**
- Fast attack, medium decay
- Moderate FM with integer ratios (2:1, 3:1)
- Saw or triangle carrier

## Multi-Stage Processing and Sound Layering

### ADSR Placement Strategies

ADSR envelopes can be placed at **multiple points** in the signal chain:

**1. Post-Carrier (Standard)**
```
osc0.out → adsr0.in → output.in1
```
Classic amplitude shaping of the final sound.

**2. Pre-FM Modulator (Timbre Morphing)**
```
osc1.out (modulator) → adsr1.in
adsr1.out → fm0.modulator_in
```
**Effect**: FM intensity varies over time. Fast attack on adsr1 creates percussive FM bursts; slow attack makes FM fade in gradually.

**Use case**: Brass swells, evolving pads, plucks with FM "thunk".

**3. Dual Envelopes (Independent Carrier/Modulator)**
```
Carrier:   fm0 → osc0 → adsr0 → output.in1
Modulator: osc1 → adsr1 → fm0.modulator_in
```
Different decay times create complex timbral evolution.

### Sound Layering: Parallel Carrier Chains

The MasterOutputProcessor has **8 inputs** for mixing multiple independent synthesis chains:

**Detuned Unison (Chorus/Ensemble)**
```
Chain 1: note → osc0 → output.in1
Chain 2: note → fscale0 (1.005) → osc1 → output.in2  
Chain 3: note → fscale1 (0.995) → osc2 → output.in3
```
±0.5% detune = subtle chorus; ±2% = wide ensemble.

**Layer balancing**: Reduce `masterLevel` to 0.3-0.5 when mixing 3+ voices to prevent clipping.

**Split-Spectrum (Bass + Lead)**
```
Bass: note → fscale0 (0.5) → osc0 (low octave) → output.in1
Lead: note → fm0 → osc1 (melody) → output.in2
```

### Hybrid FM + Ring Modulation

Combine FM and ring modulation for metallic textures:

**FM into Ring Mod**:
```
note → fm0 → osc0 (FM carrier) → ring0.in1
note → fscale0 (3.14:1 inharmonic) → osc1 → ring0.in2
ring0.out → adsr0 → output
```
FM provides base spectrum; ring mod adds sum/difference sidebands.

**Parallel FM + Ring Mod**:
```
FM path:   note → fm0 → osc0 → output.in1
Ring path: note → osc1 ─┬─ ring0 → output.in2
           note → osc2 ─┘
```
Mix tonal FM with metallic ring mod for hybrid acoustic/electronic sounds.

---

## Advanced Techniques Reference

For more complex patterns including multi-operator FM, feedback loops, and creative abuse techniques, see:
**[SKILL-ADVANCED.md](SKILL-ADVANCED.md)** in the same directory.

This includes:
- Asymmetric FM (different waveforms for carrier/modulator)
- Sub-octave reinforcement
- Filter emulation via FM amount automation
- Cascaded ring modulators
- Experimental feedback patches

---

## Generation Guidelines

Wenn ein `.smolfm` File aus einer Beschreibung generiert wird:

1. **IMMER ein `<description>` Element einfügen**, das das DSP-Konzept in einem Satz erklärt
2. **Einen aussagekräftigen Instrument-Namen wählen** und im `name` Attribut setzen
3. **Mit dem einfachsten Graph beginnen**, der den gewünschten Sound erzeugt
4. **Die obigen Rezepte als Startpunkt verwenden**
5. **Alle Verbindungen prüfen**: Port-Typen müssen übereinstimmen
6. **Instanz-Limits beachten**: max 4 notes, 8 oscs, 4 fm, 4 fscale, 4 ring, 4 adsr, 1 output
7. **Realistische Parameter-Ranges setzen**:
   - FM amount:
     * Sine carrier: 0.1-5.0 (sweet spot 1-3)
     * Saw carrier: 0.1-2.0 (sweet spot 0.5-1.5, höher = harsch)
     * Square carrier: 0.1-1.0 (sehr sensitiv)
   - Frequency scale: 0.1-10.0 (1.0 = unity; 0.5 = Oktave tiefer; 2.0 = Oktave höher)
   - ADSR attack: 0.001-2.0 Sekunden
   - ADSR decay: 0.05-3.0 Sekunden
   - ADSR sustain: 0.0-1.0
   - ADSR release: 0.01-5.0 Sekunden
   - Waveform: 0=sine, 1=saw, 2=triangle, 3=square, 4+=wavetables
   
8. **Multi-Stage Processing erwägen**:
   - Bei \"evolving\", \"swell\", \"morphing\" → ADSR auf Modulator
   - Bei \"thick\", \"detuned\", \"ensemble\" → mehrere Carrier mit fscale ±0.5-2%
   - Bei \"metallic\", \"inharmonic\" → Ring Mod zusätzlich zu FM
   - Bei \"bass\" + \"lead\" gleichzeitig → Split-Spectrum Layering
   
9. **Layer-Balancing beachten**:
   - 2 parallele Stimmen: masterLevel ≈ 0.5
   - 3 parallele Stimmen: masterLevel ≈ 0.33
   - 4+ parallele Stimmen: masterLevel ≈ 0.25
   - Verhindert Clipping am Output

10. **Nodes räumlich anordnen**: 
   - X: 60 (notes) → 380 (fscale) → 700 (fm) → 1020 (osc) → 1340 (adsr) → 1660 (output)
   - Y: vertikal verteilen für parallele Pfade (60, 380, 700, 1020, ...)
   - Bei Multi-Layer-Patches: Jede Schicht auf eigener Y-Ebene

11. **Logik testen**: Signalfluss von Note-Input zu Output durchgehen
    - Prüfen: Sind alle Inputs verbunden?
    - Bei parallelen Chains: Gehen alle zu verschiedenen output.inN?
    
12. **Mehrere Instrumente in Betracht ziehen**: Bei mehrdeutigen Beschreibungen 2-3 Varianten generieren
    - z.B. \"Bass\" → clean saw bass + FM growl bass + sub-reinforced bass

## Wichtige Hinweise zu Processor-Capabilities

### Flexible Processor-Verwendung

Die SmolFM-Prozessoren sind **generisch** und können mehrstufig verwendet werden:

**ADSR nicht nur am Ende**:
- Standard: `oscillator → adsr → output` (Amplituden-Hüllkurve)
- Pre-FM: `modulator → adsr → fm.modulator_in` (FM-Intensität über Zeit)
- Getrennt: Separate ADSRs für Carrier und Modulator (unabhängige Dynamik)

**Oscillatoren als Modulatoren UND Carrier**:
- Jeder Oscillator kann beides sein, abhängig von der Verdrahtung
- `osc.out` zu `fm.modulator_in` → Modulator
- `fm.out` zu `osc.note_in` → Carrier
- **Wichtig**: Es gibt keine dedizierten \"LFOs\" – tiefe Frequenzen via fscale < 0.1

**FrequencyScale als Multiplikator**:
- Pitch-Transposition: factor=2.0 (Oktave hoch), 0.5 (Oktave runter)
- Detuning: factor=1.005 (±0.5%), 1.02 (±2%)
- Harmonische Verhältnisse: factor=3.0 (Quinte + Oktave), 1.5 (Quinte)
- Inharmonische: factor=3.14159 (π:1), 2.718 (e:1) für metallische Klänge

**Ring Modulator als VCA**:
- `ring` multipliziert zwei Signale sample-weise
- Nutzbar als VCA: `signal → ring.in1`, `envelope → ring.in2` → amplitude modulation
- Kaskadierbar für extreme Inharmonizität

**MasterOutput als Mixer**:
- 8 Inputs für parallele Synthese-Ketten
- NICHT nur \"final output\" – auch Subgruppen möglich (z.B. mehrere adsr → verschiedene in-N)
- Level-Balancing kritisch bei 3+ Layern

---

## Wichtige Hinweise zur Skill-Pflege

### Update-Trigger

Der Skill MUSS aktualisiert werden bei:

1. **Neuen Prozessor-Typen** im Code
2. **Änderungen an Input/Output-Typen** bestehender Prozessoren
3. **Neuen Ports** bei bestehenden Prozessoren
4. **Änderungen am .smolfm Dateiformat**
5. **Änderungen der max. Instanzen** in `GraphNodeRegistry`

### Verantwortlichkeit

- Wer Prozessoren ändert oder hinzufügt, muss auch den Skill aktualisieren
- Der Skill ist Teil der \"Single Source of Truth\" für die .smolfm Datei-Generierung
- Änderungen am Code ohne Skill-Update führen zu Dokumentationsdrift

### Validierung

Der Skill sollte regelmäßig gegen den aktuellen Code validiert werden:
- Prozessor-Definitionen in `src/processors/*.h`
- Node-Specs in `src/graph/GraphNodes.cpp`
- Dateiformat in `src/graph/SmolFmFile.cpp`

## Implementierung

Die Inhalte dieser Datei manuell in `.agents/skills/smolfm/SKILL.md` nach dem Bell-Beispiel einfügen. Alternativ die komplette SKILL.md mit diesen Inhalten neu schreiben.

---

## Multi-Stage Processing: ADSR Placement Strategies

The ADSR processor can be inserted at **multiple points** in the signal chain,
not just at the end. This creates different timbral effects:

### Pattern 1: Post-Carrier Envelope (Standard)
\`\`\`
note → fm0 → osc0 (carrier) → adsr0 → output.in1
\`\`\`
Classic approach. Envelope shapes the final amplitude.

### Pattern 2: Pre-FM Modulator Envelope (Timbre Morphing)
\`\`\`
note → osc1 (modulator) → adsr1 → fm0.modulator_in
note → fm0 → osc0 (carrier) → adsr0 → output.in1
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
Modulator chain: note → osc1 → adsr1 ─┐
                                       ├→ fm0.modulator_in
Carrier chain:   note → fm0 → osc0 → adsr0 → output.in1
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
Chain 1: note → fm0 → osc0 → adsr0 → output.in1
Chain 2: note → fscale0 (1.005) → fm1 → osc1 → adsr1 → output.in2
Chain 3: note → fscale1 (0.995) → fm2 → osc2 → adsr2 → output.in3
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
<Wire from=\"note\" to=\"osc0\"/>  <!-- center -->
<Wire from=\"note\" to=\"fscale0\" to=\"osc1\"/>  <!-- sharp -->
<Wire from=\"note\" to=\"fscale1\" to=\"osc2\"/>  <!-- flat -->
<Wire from=\"osc0\" to=\"output.in1\"/>
<Wire from=\"osc1\" to=\"output.in2\"/>
<Wire from=\"osc2\" to=\"output.in3\"/>
<Node id=\"output\" level=\"0.3\"/>  <!-- reduce level to avoid clipping -->
\`\`\`

### Pattern 6: Split-Spectrum Layering (Bass + Lead)
\`\`\`
Bass:   note → fscale0 (0.5) → osc0 (saw, low octave) → adsr0 → output.in1
Lead:   note → fm0 → osc1 (sine + FM) → adsr1 → output.in2
\`\`\`
**Effect**: Independent bass and melody lines from one keyboard.
- Different fscale ratios on same note = octave splits
- **Use case**: Live performance patches, one-hand accompaniment

### Pattern 7: Transient/Sustain Split
\`\`\`
Attack:  note → fm0 (high amount) → osc0 → adsr0 (fast decay) → output.in1
Sustain: note → osc1 (pure sine) → adsr1 (slow attack, long sustain) → output.in2
\`\`\`
**Effect**: Percussive FM \"thunk\" + sustained tonal body.
- **Use case**: E-pianos, plucked basses, mallet instruments

---

## Hybrid FM + Ring Modulation

Ring modulation (`ring` processor) creates different sidebands than FM.
Combining both expands the timbral palette.

### Pattern 8: FM into Ring Mod
\`\`\`
note → fm0 → osc0 (FM carrier) → ring0.in1
note → fscale0 (3.14) → osc1 (ring modulator) → ring0.in2
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
<Wire from=\"note\" to=\"fm0\" to=\"osc0\"/>
<Wire from=\"osc0\" to=\"ring0.in1\"/>
<Wire from=\"note\" to=\"fscale0\" to=\"osc1\"/>
<Wire from=\"osc1\" to=\"ring0.in2\"/>
<Wire from=\"ring0\" to=\"adsr0\" to=\"output.in1\"/>
\`\`\`

### Pattern 9: Parallel FM + Ring Mod
\`\`\`
FM path:   note → fm0 → osc0 → adsr0 → output.in1
Ring path: note → osc1 ─┐
                         ├→ ring0 → adsr1 → output.in2
           note → osc2 ─┘
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
Main: note → fm0 → osc0 → output.in1
Sub:  note → fscale0 (0.5) → osc1 (sine) → adsr1 → output.in2
\`\`\`
**Effect**: Adds sub-bass foundation.
- fscale=0.5 = one octave down
- Sine sub keeps low end clean (no FM artifacts)
- **Use case**: Bass patches, kick drums

### Pattern 13: Filter Emulation via FM
\`\`\`
note → fm0 (variable amount) → osc0 (saw) → output
\`\`\`
**Effect**: Sweep fm0.amount over time (via DAW automation) = \"filter sweep\".
- Low FM amount ≈ \"closed filter\" (few sidebands)
- High FM amount ≈ \"open filter\" (many sidebands)
- **Use case**: Acid basslines, dubstep wobbles

---

## Instrument-Specific Advanced Recipes

### E-Bass with Distortion (via Ring Mod)
\`\`\`
Clean path:  note → osc0 (saw) → adsr0 → output.in1
Growl path:  note → fscale0 (2.0) → osc1 (square) → adsr1 (fast decay) → output.in2
\`\`\`
Mix ratio: 70% clean, 30% growl → adds harmonic complexity without losing definition.

### Evolving Pad (3-Stage FM + Swell Envelope)
\`\`\`
note → fm0 → fm1 → fm2 → osc0 → output.in1
         ↑     ↑     ↑
        osc1  osc2  osc3
         ↑
       adsr1 (slow attack on modulator)
\`\`\`
Envelope on modulator creates \"blooming\" effect.

### Drum Kit (Kick + Snare from one patch)
\`\`\`
Kick:  note (low MIDI note) → osc0 (sine) → adsr0 (fast decay) → output.in1
Snare: note (high MIDI note) → ring0 (osc1 × osc2) → adsr1 (very fast) → output.in2
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
note → fm0.freq_in
fm0.out → osc0.note_in
\`\`\`
**WARNING**: Creates feedback loop! May result in silence or noise depending on phase.
- Not officially supported, but can create interesting glitches
- Use adsr to tame the output

### Pattern 15: Audio-Rate Frequency Scaling
\`\`\`
note → osc1 (LFO, very low freq) → fscale0.freq_in
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

---

# Unified Generation Workflow

For every instrument request, execute this sequence mentally before writing XML.

## Step 1 — Define the sound target

Create an internal target profile:

```text
Identity:
Spectral character:
Temporal character:
Pitch/ratio behavior:
Layering:
Primary synthesis mechanism:
Secondary character mechanism:
```

## Step 2 — Pick a core architecture

Choose one:
- subtractive-like oscillator/envelope,
- single FM,
- chained FM,
- parallel FM layers,
- FM + ring modulation,
- parallel FM + ring,
- transient/sustain split,
- detuned unison,
- hybrid architecture.

## Step 3 — Establish the frequency relationships

For tonal sounds, prefer deliberate integer or musically meaningful ratios.

For inharmonic sounds, deliberately use non-integer or irrational ratios.

Ask:
- What is the carrier frequency?
- What is each modulator frequency relative to the played note?
- Should sidebands remain pitch-glued or become inharmonic?
- Is the modulator tracking the keyboard?
- Does the sound need a stable fundamental?

## Step 4 — Establish time behavior

Use ADSR not only for final amplitude but also for **timbral evolution**.

Typical logic:
- fast attack + high FM → impact / bell / pluck,
- slow attack + low FM → pad / swell,
- long modulator decay → evolving or metallic tail,
- short modulator envelope + sustained carrier → transient/sustain contrast.

## Step 5 — Add one or two character stages

Examples:
- detune for thickness,
- ring mod for metallic sidebands,
- second FM stage for denser spectra,
- sub oscillator for low-end authority,
- separate modulator ADSR for evolving brightness.

Avoid adding complexity without a sonic reason.

## Step 6 — Balance the layers

When multiple outputs are mixed:
- 2 voices: around 0.5 each,
- 3 voices: around 0.33 each,
- 4+ voices: around 0.25 each.

Use the single `masterLevel` conservatively when several signals enter the output.

## Step 7 — Validate the graph

Check:
- every connection uses an existing node,
- source is an output port,
- destination is an input port,
- signal connects only to signal,
- frequency connects only to frequency,
- every required signal path eventually reaches `output`,
- no input receives multiple sources,
- processor instance limits are respected,
- parameter ranges are sensible,
- output level is safe.

## Step 8 — Explain the interesting parameters

Do not stop at “here is the XML”.

Always tell the user which knobs are worth touching first and what sonic result to expect.

---

# Patch Explanation Standard

For each generated instrument, use this structure immediately before the XML:

### Kurzbeschreibung
<1–2 sentences>

### Prinzip
<2–5 sentences explaining the DSP/synthesis idea>

### Patch
```text
<readable signal-flow graph>
```

### Spannende Parameter
| Parameter | Value | Sonic effect |
|---|---:|---|
| ... | ... | ... |

### Erwarteter Klang
<brief description of attack, body, spectrum, movement, decay>

Then provide the complete `.smolfm`.

---

# Creative Parameter Search Strategy

When the request is open-ended (“mach etwas Spannendes”), optimize in this order:

1. **Architecture** — choose a distinctive topology.
2. **Frequency ratios** — determine the spectral fingerprint.
3. **FM amount** — determine spectral density.
4. **Envelope placement** — determine evolution.
5. **Waveform asymmetry** — introduce controlled complexity.
6. **Layering/detuning** — add width and body.
7. **Ring modulation** — add inharmonic character where appropriate.
8. **Output balance** — preserve clarity.

A useful exploration set is:

### Ratio axis
- 0.5, 1, 1.5, 2, 3, 3.5, 4
- selected inharmonic values such as 1.19, 2.718, 3.14, 3.55

### FM axis
- subtle: 0.1–0.5
- moderate: 0.5–1.5
- strong: 1.5–3
- extreme: 3–6, mainly for sine carriers and controlled percussion/FX

### Detune axis
- ±0.5% = subtle chorus
- ±2% = obvious ensemble

### Envelope axis
- transient: attack ~0.001–0.01 s
- pluck: decay ~0.1–1 s
- pad: attack ~0.5–2 s
- long tail: release ~2–5 s

These are design regions, not guarantees.

---

# Parameter Interaction Rules

## FM amount × carrier waveform

| Carrier | Typical usable region | Character |
|---|---|---|
| Sine | 0–5 | clean to complex sidebands |
| Saw | 0–2 | quickly becomes dense/harsh |
| Square | 0.1–1 | highly sensitive |

## Ratio × FM amount

Low FM amount with a strange ratio can create a subtle metallic coloration.
High FM amount with an inharmonic ratio can create a dense, bell/noise-like spectrum.

## Envelope × FM amount

The same FM amount can sound completely different depending on when it is active.
Therefore, for evolving sounds, prefer an envelope on the **modulator path** instead of
only changing the carrier amplitude.

## Detune × layer count

More layers require smaller per-layer levels. Detuning should remain deliberate:
small values preserve pitch identity; larger values create obvious ensemble beating.

---

# Advanced Pattern Selection

Use these patterns when a simple recipe is insufficient:

1. **Pre-FM modulator envelope** — timbral morphing.
2. **Dual envelopes** — independent carrier and modulator dynamics.
3. **Layered detuned FM** — width and richness.
4. **Transient/sustain split** — attack detail plus stable body.
5. **FM into ring mod** — metallic hybrid.
6. **Parallel FM + ring** — tonal body plus metallic layer.
7. **Cascaded ring modulation** — extreme inharmonicity.
8. **Asymmetric FM** — different carrier/modulator waveforms.
9. **Sub-octave reinforcement** — bass foundation.
10. **FM amount automation** — filter-like spectral movement where host automation exists.

---

# Safety / Validity Rule for Experimental Patches

Experimental patterns must never be presented as guaranteed-safe production patches.

The supplied advanced source contains an experimental feedback pattern:

```text
osc0.out → fm0.modulator_in
note → fm0.freq_in
fm0.out → osc0.note_in
```

It explicitly warns that this is an unsupported feedback loop and may result in silence or
noise. If using it, label it **EXPERIMENTAL** and prefer a conventional non-feedback
alternative first.

Likewise, the supplied advanced document contains an “audio-rate frequency scaling”
idea that conflicts with the documented port types if an oscillator `signal` output is
connected directly to a `frequency` input. The mandatory type-safety rules take precedence:
do not emit an invalid `signal → frequency` wire in a normal generated `.smolfm` patch.

---

# Quality Checklist Before Returning a Patch

### Sound design
- [ ] The requested sound identity is explicit.
- [ ] The spectral mechanism is intentional.
- [ ] Frequency ratios have a reason.
- [ ] FM amount matches the carrier waveform.
- [ ] Envelope timing supports the intended articulation.
- [ ] Complexity is audible/useful rather than decorative.

### Patch engineering
- [ ] XML is structurally valid according to the documented format.
- [ ] `version="2"` is used.
- [ ] `<description>` is present and explains the DSP concept.
- [ ] Node IDs follow the documented conventions.
- [ ] Ports are declared correctly.
- [ ] All wires are type-compatible.
- [ ] Input fan-in rules are respected.
- [ ] Instance limits are respected.
- [ ] Output level is appropriate for the number of layers.

### Communication
- [ ] Kurzbeschreibung included.
- [ ] Prinzip included.
- [ ] Patch/wiring explanation included.
- [ ] 3–7 spannende parameters explained.
- [ ] Expected sound behavior explained.
- [ ] If useful, 1–2 intentional variation ideas are mentioned.

---

# Skill Maintenance

Update this skill whenever:
- processor types are added/removed,
- processor ports change,
- parameter names/ranges change,
- `.smolfm` format changes,
- instance limits change,
- new reliable sound-design patterns are discovered.

The processor definitions, graph registry, and file-format implementation remain the ultimate
technical source of truth. This skill should be kept synchronized to prevent documentation drift.


---

# Advanced Sound Design Extension — Routing, Instrument Families & Genre Adaptation

This section extends the existing skill with a systematic method for designing high-quality
instruments across synthesizer families and musical contexts. It does not replace the
processor reference above. When there is a conflict, the documented processor types,
ports, instance limits, and file-format rules above remain authoritative.

## 1. Two Signal Domains

Think about every patch as a graph containing two related domains:

### Frequency domain

These nodes produce or transform Hertz values:

- `note`
- `fscale`
- `fm`
- `fadsr`

Typical flow:

```text
note → fscale/fm/fadsr → osc.note_in
```

### Audio domain

These nodes produce or process audio-rate signals:

- `osc`
- `gain`
- `shape`
- `filter`
- `adsr`
- `ring`
- `am`
- `delay`
- `output`

Typical flow:

```text
osc → gain/shape/filter → adsr → delay → output
```

**Never cross these domains with an incompatible wire.** In particular, an oscillator's
`signal` output cannot directly drive `fm.freq_in`, `fscale.freq_in`, `fadsr.freq_in`,
or `filter.cutoff_mod_in`. A frequency-producing node is required there.

## 2. General Patch Construction Hierarchy

For every requested sound, make decisions in this order:

1. **Musical role** — bass, lead, pad, pluck, bell, percussion, texture, etc.
2. **Spectral mechanism** — subtractive, FM, AM, ring modulation, waveshaping, layering,
   or a deliberate hybrid.
3. **Signal topology** — decide the minimum graph that can create the mechanism.
4. **Parameter relationships** — ratios, modulation depth, envelope timing, filter movement,
   layer balance.
5. **Fine values** — only after the structure is correct.

Do not start by randomly selecting processor parameters. First decide what physical/DSP
mechanism should create the requested character.

## 3. Minimum Viable Topology

Prefer the smallest graph that produces the desired audible behavior.

### Basic subtractive voice

```text
note → osc → filter → adsr → output
```

### Layered voice

```text
                 ┌→ gain → filter ─┐
note → osc0 ─────┤                  ├→ adsr → output
                 └→ gain → filter ─┘
note → osc1 ────────────────────────┘
```

### FM voice

```text
note → fm.freq_in
note → fscale → modulator osc → fm.modulator_in
fm.out → carrier osc.note_in → adsr → output
```

The exact routing matters: `fm.out` is a **frequency**, so it must feed the carrier
oscillator's `note_in`, not an audio input.

## 4. Oscillator Selection

Choose waveforms according to the desired spectral starting point:

| Waveform | Typical role | Design tendency |
|---|---|---|
| Sine | sub, FM carrier, bell, clean body | minimal harmonic content |
| Triangle | soft body, mellow lead | fewer upper harmonics than saw |
| Saw | bass, lead, brass, pad | dense harmonic foundation |
| Square | hollow bass/lead, digital tones | strong odd-harmonic character |
| Noise | snare, hats, breath, texture | no pitched harmonic series |

For FM, a sine carrier gives the most predictable sideband structure. A saw or square
carrier can become dense very quickly because its own harmonics are also modulated.

## 5. Layering Strategy

Layer only when a single oscillator cannot provide all required roles.

Useful layer roles:

- **Sub layer:** sine, usually one octave or more below the main voice.
- **Body layer:** sine/triangle with little modulation.
- **Character layer:** FM, ring mod, or waveshaped oscillator.
- **Transient layer:** short envelope and optionally a pitch envelope.
- **Noise layer:** noise oscillator for breath, attack, snare, hats, or texture.

Balance layers with `gain` before they reach the final output. Do not assume that eight
oscillators automatically sound better than two deliberately chosen layers.

## 6. Frequency Scaling and Harmonic Relationships

`fscale` is the main tool for defining frequency ratios.

Examples:

```text
note → fscale(2.0) → osc       = octave above
note → fscale(0.5) → osc       = octave below
note → fscale(3.0) → osc       = third harmonic
note → fscale(3.5) → osc       = inharmonic FM starting point
```

Use simple integer ratios for harmonic relationships and non-integer ratios when an
inharmonic or metallic spectrum is desired.

For a tonal instrument, prefer ratios that preserve a recognizable pitch. For bells,
gongs, metallic percussion, and experimental sounds, deliberately non-integer ratios can
be more appropriate.

## 7. FM Design Rules

FM is primarily a **timbre-generation mechanism**, not merely an effect.

Canonical routing:

```text
note → fm.freq_in
note → fscale → modulator osc → fm.modulator_in
fm.out → carrier osc.note_in
```

### FM ratio

The modulator/carrier frequency ratio determines the spacing and organization of
sidebands.

- ~1:1 → electric-piano-like, vocal, rounded FM colors
- ~2:1 → brighter, more harmonic character
- ~3–4:1 → bell/chime territory
- non-integer ratios → increasingly inharmonic/metallic behavior

These are starting regions, not fixed presets.

### FM amount

Increase FM amount when more sidebands and brightness are needed. Reduce it when the
fundamental becomes obscured or the spectrum becomes unnecessarily dense.

For expressive sounds, change FM intensity over time instead of relying only on a static
high amount.

## 8. Dynamic FM: Modulator Envelope

The existing processor set does not provide a dedicated signal-amplitude modulation
input for `fm` other than the modulator signal itself. Therefore, shape the modulator
with an `adsr` when a time-varying FM spectrum is required.

```text
note → fscale → modulator osc → adsr → fm.modulator_in
note → fm.freq_in
fm.out → carrier osc.note_in
carrier osc → main adsr → output
```

This is useful for:

- bright attack followed by a mellow sustain,
- piano/e-piano transients,
- bells whose high partials disappear during the decay,
- evolving digital textures.

The modulator envelope and the carrier amplitude envelope should be treated as two
independent musical controls.

## 9. Ring Modulation

Ring modulation multiplies two audio signals and creates sum/difference components.
It is therefore a **spectral generator**, not simply an effect placed at the end.

```text
osc A ─┐
       ├→ ring → shape/filter → adsr → output
osc B ─┘
```

Use harmonic ratios for controlled coloration and inharmonic ratios for metallic or
experimental spectra.

For a tonal hybrid, keep a dry sine/body layer in parallel:

```text
osc body → gain ─────────────────────┐
                                     ├→ output
osc A + osc B → ring → filter → adsr ─┘
```

This preserves a stable fundamental while the ring-mod layer supplies character.

## 10. Amplitude Modulation

AM preserves a carrier component because the documented `AmProcessor` uses a biased
modulator. This makes it suitable for tremolo and animated timbres where the carrier
should remain identifiable.

### Tremolo

Use an LFO-mode oscillator as the modulator:

```text
note → carrier osc → am.carrier_in
LFO osc → am.modulator_in
am → adsr → output
```

For a periodic gate/chop, use a square LFO and a high AM amount. For smooth tremolo, use
a sine LFO and a moderate amount.

### Audio-rate AM

Use a pitch-derived oscillator as the modulator when the sidebands should track the
played note:

```text
note → fscale(ratio) → modulator osc
note → carrier osc
carrier + modulator → am → adsr → output
```

## 11. Waveshaping

Waveshaping is most useful when a clean oscillator or FM signal needs additional
harmonic density.

### Soft

Use low-to-moderate drive for warmth, body, and saturation.

```text
osc/FM → gain → shape(Soft) → filter → adsr → output
```

### Hard

Use for aggressive digital/acid-like sounds and dense harmonic buildup.

### Fold

Use for metallic, unstable, or deliberately inharmonic textures. High drive can become
very dense, so follow it with `gain` and/or `filter` when necessary.

`gain` before a shaper is structurally important: it controls how hard the waveform enters
the nonlinear transfer function.

## 12. Filter Design

The filter has two fundamentally different operating modes.

### Static filter

Leave `cutoff_mod_in` unconnected and use `filter%Cutoff`.

```text
source → filter → adsr → output
```

### Dynamic cutoff

`cutoff_mod_in` expects a **frequency** and replaces the static cutoff. Therefore use
`fadsr`, `fscale`, or another frequency-producing chain — never a normal `adsr` output.

Correct filter-envelope pattern:

```text
note → fadsr → filter.cutoff_mod_in
source → filter.in
filter → adsr → output
```

For a fixed base cutoff with a moving range, derive the frequency first, then use the
frequency envelope:

```text
note → fscale → fadsr.freq_in → filter.cutoff_mod_in
```

The filter's static cutoff is ignored once `cutoff_mod_in` is connected.

## 13. F-ADSR: Pitch and Frequency Motion

`fadsr` operates in the frequency domain. Use it for pitch transients and any parameter
that explicitly expects Hertz.

### Kick / 808 pitch drop

```text
note → fadsr → sine osc → adsr → output
```

A short attack/decay with a lower starting factor produces the characteristic falling
pitch transient.

### Pitch bling / upward transient

```text
note → fadsr → osc → adsr → output
```

Use `up` above 1.0 for a transient that begins above the played pitch and settles toward
the normal register.

### FM sweep

Keep the FM stage in the frequency domain, then place the pitch envelope after it when
the intention is to move the already-modulated carrier frequency:

```text
note → fm.freq_in
note → fscale → modulator osc → fm.modulator_in
fm.out → fadsr.freq_in
fadsr.out → carrier osc.note_in
```

## 14. Gain as a Structural Processor

Do not treat `gain` as only a final volume knob. It has at least four important design
roles:

1. **Layer balance** — normalize multiple voices before mixing.
2. **Pre-shaper drive** — increase the level entering `shape`.
3. **Modulator level management** — control how strongly a modulation source affects a
   downstream nonlinear stage.
4. **Post-processing trim** — tame a hot waveshaper or ring-mod output.

When several layers are summed, reduce individual gains rather than relying on the master
level to hide internal overload.

## 15. Delay as Part of the Instrument Identity

Use delay when the repeat structure is musically meaningful.

Good uses:

- short rhythmic lead repeats,
- pluck echoes,
- ambient tails,
- tempo-synchronized rhythmic patterns,
- two parallel delay times for staggered echoes.

A delay after the amplitude envelope allows the envelope to define the dry note while the
delay creates the tail:

```text
osc → adsr → delay → output
```

For a dry/wet balance that remains explicit, a dry copy can also feed another output input.

## 16. Instrument Archetype Recipes

These are construction strategies, not fixed presets.

### Sub Bass

```text
note → sine osc → adsr → output
```

Optionally add a very low-drive Soft shaper for gentle harmonics.

### Bass / Growl

```text
note → saw/square osc → shape/filter → adsr → output
note → fscale → FM modulator → FM → optional second layer
```

Use dynamic filtering when the growl should open on the attack.

### Lead

Use a saw or square carrier, moderate filtering, expressive ADSR, and optional LFO-mode
vibrato or light delay.

### Pad

Use multiple carriers with slightly different waveforms/ratios, slow attacks, long
releases, moderate filtering, and optional AM or delay movement.

### Bell / Chime

Use a sine carrier, inharmonic FM ratio, short attack, and a longer decay. Keep FM
intensity moderate enough that the pitch remains readable unless an intentionally
metallic result is requested.

### E-Piano

Use 1:1 FM for the body, a higher-ratio transient component for attack brightness, and
Soft waveshaping for warmth.

### Organ

Use parallel oscillators at integer frequency ratios. Balance harmonic layers with
gain rather than relying on distortion.

### Metallic Percussion

Combine FM, ring modulation, or Fold waveshaping with short envelopes. Keep a parallel
pitched body layer when the hit needs a recognizable root.

### Kick

Use a sine carrier with `fadsr` pitch motion and a short `adsr` amplitude envelope.
The pitch transient should be substantially faster than the amplitude decay.

### Snare / Noise Percussion

Use a noise oscillator for the noisy body, optionally combined with a short tonal/FM or
ring-modulated layer. Shape each layer with its own envelope when their decay times differ.

## 17. Genre Adaptation

Genre should modify the design priorities, not force a fixed preset.

### House / Disco / Funk

Prioritize clear fundamentals, rhythmic modulation, moderate filtering, tasteful delay,
and controlled resonance. Tremolo/AM can be used as a rhythmic movement source.

### Techno

Prioritize strong transient definition, controlled repetition, FM/ring modulation for
metallic percussion, and tempo-synchronized delay where appropriate.

### Ambient / Experimental

Prioritize long envelopes, evolving FM depth, inharmonic ratios, slow LFO movement,
layered textures, and long delay tails.

### Synthwave / Retro

Prioritize saw/square carriers, octave layering, restrained detuning, simple harmonic
ratios, moderate filtering, and audible but controlled delay.

### Bass Music / Drum & Bass

Prioritize a stable sub layer plus a separate character layer. Use FM, hard/fold shaping,
and dynamic filtering for the character layer while protecting the sub from unnecessary
spectral complexity.

### Cinematic

Use layered tonal and inharmonic components, long envelopes, carefully controlled pitch
motion, and broad spectral evolution. Keep the fundamental/body layer stable when the
sound must remain musically legible.

## 18. Complexity Budget

Every processor should answer a concrete question:

- Does this oscillator provide a missing spectral role?
- Does this FM stage create a specific sideband structure?
- Does this ring modulator create a required inharmonic component?
- Does this shaper add the requested nonlinear character?
- Does this filter solve a spectral or dynamic problem?
- Does this envelope define a meaningful temporal change?
- Does this delay create an intentional spatial/rhythmic behavior?

If the answer is no, remove the processor.

Prefer:

```text
simple → evaluate → add one purposeful stage → evaluate again
```

over:

```text
many processors → hope the result is interesting
```

## 19. Quality-Control Procedure

Before returning a generated instrument, inspect it in this order:

### A. Graph validity

- Every node uses a documented processor ID.
- Every instance is within its maximum count.
- Node IDs follow the naming convention.
- Every wire connects an output to an input.
- Signal and frequency types match.
- No input has multiple competing sources.
- `output` is reachable from the intended audio path.

### B. Musical validity

- The fundamental is identifiable when the sound is intended to be tonal.
- The chosen ratios support the intended harmonic or inharmonic character.
- Attack, decay, sustain, and release fit the instrument role.
- Modulation depth is proportional to the desired complexity.
- Layers have intentional level relationships.

### C. Register validity

Check that the sound remains useful across the intended keyboard range. A ratio that is
convincing in one register can become excessively bright or dense in another.

### D. Repeated-note validity

Consider how the patch behaves on repeated notes. Note-triggered LFO/static oscillators
re-sync, while delay buffers persist. The resulting interaction should be intentional.

## 20. Patch Documentation Standard

Every generated patch should be explainable without opening the XML. Document:

- **Sound name**
- **Instrument family / role**
- **Genre or context** when relevant
- **Core synthesis principle**
- **Signal topology**
- **Important node instances**
- **Important connections**
- **Key parameter values**
- **Reason for each unusual parameter**
- **Expected audible result**
- **Useful variation directions**

Use concise wiring notation so a human can reconstruct the architecture:

```text
note
 ├─→ carrier frequency path
 └─→ ratio path → modulator oscillator → FM
                         ↓
carrier oscillator → shaping/filtering → ADSR → output
```

## 21. Final Design Principle

The goal is not maximum processor count. The goal is a **deliberate relationship between
frequency, spectrum, time, and level**.

A high-quality SmolFM instrument should therefore be designed as:

```text
musical role
    ↓
spectral mechanism
    ↓
frequency relationships
    ↓
signal topology
    ↓
temporal envelopes
    ↓
level / nonlinear processing
    ↓
final output behavior
```

Start simple. Add complexity only when the requested sound requires it. Prefer explicit,
reproducible relationships over arbitrary parameter values. The existing processor and
file-format documentation remains the source of truth for what can actually be wired.
