---
name: smolfm-instrument-designer
description: >
  Expert skill for designing and generating SmolFM synthesizer instruments (.smolfm files).
  Use when creating FM synthesis instruments like bells, pianos, basses, pads, drums,
  or any other sounds using the SmolFM processor graph system. Always provide a clear
  description of the DSP structure and instrument concept in the XML description element.
  Accepts instrument descriptions and generates complete .smolfm files with proper wiring.
---

# SmolFM Instrument Designer

This skill helps you create sophisticated FM-based instruments for the SmolFM synthesizer by generating `.smolfm` XML files that define processor graphs, connections, and parameters.

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
| **NoteProcessor** | `note` | 4 | _(none)_ | `out` (frequency) | Converts MIDI note number to frequency in Hz using equal temperament |
| **OscillatorProcessor** | `osc` | 8 | `note_in` (frequency) | `out` (signal) | Generates waveform at frequency from note_in port; supports multiple waveforms |
| **FMModulationProcessor** | `fm` | 4 | `freq_in` (frequency), `modulator_in` (signal) | `out` (frequency) | True FM: scales carrier frequency by modulator signal; chainable in Hertz domain |
| **FrequencyScaleProcessor** | `fscale` | 4 | `freq_in` (frequency) | `out` (frequency) | Multiplies frequency by constant factor; useful for transposition and harmonic series |
| **RingModulatorProcessor** | `ring` | 4 | `in1` (signal), `in2` (signal) | `out` (signal) | Multiplies two signals sample-wise; creates sum/difference sidebands for metallic timbres |
| **AdsrProcessor** | `adsr` | 4 | `in` (signal) | `out` (signal) | Applies ADSR envelope to signal; multiplies input by envelope value and velocity |
| **MasterOutputProcessor** | `output` | 1 | `in1`-`in8` (signal, 8 inputs) | _(none, final output)_ | Sums up to 8 signal inputs with master level control and peak metering |

### Processor Port Details

#### NoteProcessor
- **Purpose**: MIDI note → frequency converter
- **Inputs**: None
- **Outputs**: `out` (frequency) - emits frequency in Hz for played MIDI note
- **Parameters**: None (controlled by MIDI input)

#### OscillatorProcessor
- **Purpose**: Waveform generation
- **Inputs**: `note_in` (frequency) - frequency source (0 Hz if unconnected = silent)
- **Outputs**: `out` (signal) - raw oscillator sample
- **Parameters**:
  - `osc%Waveform` - waveform selector (sine, saw, square, triangle, wavetables)
  - Instance index replaces `%` (e.g., `osc0Waveform`, `osc3Waveform`)

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

#### AdsrProcessor
- **Purpose**: Envelope shaping
- **Inputs**: `in` (signal)
- **Outputs**: `out` (signal) = `in * envelope * velocity`
- **Parameters** (APVTS IDs use `adsr%` prefix + parameter name):
  - `adsr%Attack` - attack time (seconds)
  - `adsr%Decay` - decay time (seconds)
  - `adsr%Sustain` - sustain level (0-1)
  - `adsr%Release` - release time (seconds)

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
  - Examples: `note0`, `osc0`, `osc1`, `fm0`, `fscale0`, `adsr0`, `output`
  - Single-instance nodes omit index: `output` (not `output0`)
- `x`, `y` (required): Canvas position integers (visual layout)
- **Processor-specific parameter attributes** (as many as needed):
  - Oscillator: `waveform` (integer index)
  - FM: `amount` (float)
  - FrequencyScale: `factor` (float)
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
| `note` | `note0`, `note1`, `note2`, `note3` | 0-3 |
| `osc` | `osc0` through `osc7` | 0-7 |
| `fm` | `fm0` through `fm3` | 0-3 |
| `fscale` | `fscale0` through `fscale3` | 0-3 |
| `ring` | `ring0` through `ring3` | 0-3 |
| `adsr` | `adsr0` through `adsr3` | 0-3 |
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

### Signal Flow Patterns

#### Basic Oscillator Chain
```
note0.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Single FM Stage
```
note0.out → fm0.freq_in          (carrier frequency)
osc1.out → fm0.modulator_in      (modulator signal)
fm0.out → osc0.note_in           (modulated frequency to carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Chained FM Stages (Multiple Modulators)
```
note0.out → fm0.freq_in
osc1.out → fm0.modulator_in      (first modulator)
fm0.out → fm1.freq_in            (FM chain in Hertz domain)
osc2.out → fm1.modulator_in      (second modulator)
fm1.out → osc0.note_in           (final FM chain to carrier)
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### FM with Modulator Tracking (Scales with Keyboard)
```
note0.out → fm0.freq_in          (carrier frequency)
note1.out → fscale0.freq_in      (modulator tracks keyboard)
fscale0.factor = 3.5             (modulator/carrier ratio 3.5:1)
fscale0.out → osc1.note_in       (scaled frequency to modulator)
osc1.out → fm0.modulator_in      (modulator signal)
fm0.out → osc0.note_in
osc0.out → adsr0.in
adsr0.out → output.in1
```

#### Ring Modulation
```
note0.out → osc0.note_in         (carrier)
note1.out → fscale0.freq_in      (modulator tuning)
fscale0.out → osc1.note_in
osc0.out → ring0.in1             (carrier to ring mod)
osc1.out → ring0.in2             (modulator to ring mod)
ring0.out → adsr0.in
adsr0.out → output.in1
```

#### Parallel Voices (Multiple Carriers)
```
note0.out → osc0.note_in         (voice 1)
note0.out → osc1.note_in         (voice 2, same pitch)
osc0.out → adsr0.in
osc1.out → adsr1.in
adsr0.out → output.in1           (mix into output)
adsr1.out → output.in2           (separate envelope per voice)
```

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
  <Node id="note0" x="60" y="60">
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
  <Node id="note1" x="60" y="380">
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="note2" x="60" y="700">
    <Pin id="out" direction="out" type="frequency"/>
  </Node>
  <Node id="fscale0" x="380" y="60" factor="3.5545"/>
  <Node id="fscale1" x="380" y="380" factor="1.1886"/>
  <Node id="output" x="1660" y="60" level="0.8"/>
</Nodes>
<Connections>
  <Wire from="note0" fromPort="out" to="fm0" toPort="freq_in"/>
  <Wire from="osc1" fromPort="out" to="fm0" toPort="modulator_in"/>
  <Wire from="fm0" fromPort="out" to="fm1" toPort="freq_in"/>
  <Wire from="osc2" fromPort="out" to="fm1" toPort="modulator_in"/>
  <Wire from="fm1" fromPort="out" to="osc0" toPort="note_in"/>
  <Wire from="osc0" fromPort="out" to="adsr0" toPort="in"/>
  <Wire from="note1" fromPort="out" to="fscale0" toPort="freq_in"/>
  <Wire from="fscale0" fromPort="out" to="osc1" toPort="note_in"/>
  <Wire from="note2" fromPort="out" to="fscale1" toPort="freq_in"/>
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
note0 → fm0.freq_in
note1 → fscale0 → osc1.note_in  (modulator with ratio)
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
note1 → fscale0 → osc1 (modulator) → adsr1 (slow attack) → fm0.modulator_in
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
note0 (fixed pitch ~200Hz) → fm0.freq_in
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
note0 (fixed ~MIDI note 36-40) → osc0.note_in (sine)
osc0.out → adsr0 (attack=0.001, decay=0.3, sustain=0, release=0.1) → output.in1
```

**With click:**
```
note0 → fm0.freq_in
note1 → fscale0 (factor=5.0) → osc1 (modulator)
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
note0 → osc0 (saw) → adsr0 (attack=0.005, decay=0.2, sustain=0.8, release=0.15) → output.in1
```

**Growl bass:**
```
note0 → fm0.freq_in
note1 → fscale0 (factor=2.0) → osc1 (saw modulator)
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
note0 → osc0 (sine) → adsr0 (attack=0.005, decay=0.1, sustain=0.95, release=0.3) → output.in1
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
note0 → osc0 (sine) → adsr0 (attack=0.001, decay=1.0, sustain=0.6, release=0.4) → output.in1
```

**Advanced 808 with FM tail:**
```
note0 → fm0.freq_in
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
  <Node id=\"note0\"/>
  <Node id=\"osc0\" waveform=\"1\"/>  <!-- saw -->
  <Node id=\"osc1\" waveform=\"1\"/>
  <Node id=\"osc2\" waveform=\"2\"/>  <!-- triangle -->
  <Node id=\"fscale0\" factor=\"1.005\"/>  <!-- slight detune -->
  <Node id=\"fscale1\" factor=\"0.995\"/>
  <Node id=\"adsr0\" attack=\"1.0\" decay=\"2.0\" sustain=\"0.7\" release=\"3.0\"/>
  <Node id=\"output\" level=\"0.3\"/>
</Nodes>
<Connections>
  <Wire from=\"note0\" to=\"osc0\"/>
  <Wire from=\"note0\" to=\"fscale0\" to=\"osc1\"/>  <!-- detuned + -->
  <Wire from=\"note0\" to=\"fscale1\" to=\"osc2\"/>  <!-- detuned - -->
  <Wire from=\"osc0\" to=\"adsr0\"/>
  <Wire from=\"osc1.out\" to=\"output.in2\"/>
  <Wire from=\"osc2.out\" to=\"output.in3\"/>
  <Wire from=\"adsr0\" to=\"output.in1\"/>
</Connections>
```

**With FM for movement:**
```
note0 → fm0.freq_in
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
- Bass: note0 → fscale (0.5) → osc (low octave)
- Lead: note1 → fm → osc (melody)
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
Chain 1: note0 → osc0 → output.in1
Chain 2: note1 → fscale0 (1.005) → osc1 → output.in2  
Chain 3: note2 → fscale1 (0.995) → osc2 → output.in3
```
±0.5% detune = subtle chorus; ±2% = wide ensemble.

**Layer balancing**: Reduce `masterLevel` to 0.3-0.5 when mixing 3+ voices to prevent clipping.

**Split-Spectrum (Bass + Lead)**
```
Bass: note0 → fscale0 (0.5) → osc0 (low octave) → output.in1
Lead: note1 → fm0 → osc1 (melody) → output.in2
```

### Hybrid FM + Ring Modulation

Combine FM and ring modulation for metallic textures:

**FM into Ring Mod**:
```
note0 → fm0 → osc0 (FM carrier) → ring0.in1
note1 → fscale0 (3.14:1 inharmonic) → osc1 → ring0.in2
ring0.out → adsr0 → output
```
FM provides base spectrum; ring mod adds sum/difference sidebands.

**Parallel FM + Ring Mod**:
```
FM path:   note0 → fm0 → osc0 → output.in1
Ring path: note1 → osc1 ─┬─ ring0 → output.in2
           note2 → osc2 ─┘
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