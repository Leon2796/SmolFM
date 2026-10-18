# SmolFM YAML Format Grammar

## Overview
The `.smolfm` file can be encoded in YAML.  Version 3 splits the document into
a semantic part (`graph:`: processors + wiring) and a presentation part
(`layout:`: canvas boxes; future UI settings extend it).  Legacy version 2
files (flat structure, numeric values) stay readable.

## File Structure (version 3)
```yaml
version: "3"
name: "Instrument Name"
graph:
  nodes:
    - id: "note"
      pins:
        - id: "out"
          direction: out
          type: frequency
        - id: "vel"
          direction: out
          type: velocity
    - id: "osc0"
      waveform: saw
      mode: pitch
      staticfreq: 440
      pins:
        - id: "note_in"
          direction: in
          type: frequency
        - id: "out"
          direction: out
          type: signal
  connections:
    - from: "note"
      fromPort: "out"
      to: "osc0"
      toPort: "note_in"
layout:
  boxes:
    - id: "note"
      x: 60
      y: 100
    - id: "osc0"
      x: 400
      y: 100
```

## Grammar Rules

### Root
- `version`: Required string; "3" = split format (this document), "2" = legacy flat
- `name`: Optional string, instrument name
- `description`: Optional string; one concise sentence stating the sound-design goal (empty string allowed)
- `techniques`: Optional string; comma-separated names of the `doc/techniques` catalogue techniques used (empty string allowed)
- `graph`: Required mapping - `nodes` (required array) and `connections`
  (optional array; an empty list clears the wiring)
- `layout`: Optional mapping - `boxes`: one position per node id; a node
  without a box keeps its current canvas position

### Node Object
- `id`: Required string (unique identifier, e.g., "note", "osc0")
- Type-specific attributes: the allowed keys per node type are listed in
  "Dynamic Attributes by Node Type" below - one section per processor,
  mirroring the per-processor complexTypes in `smolfm.xsd`.  Enumerated
  values are written as their names (see the Enumerations table); the
  numeric legacy spelling is still accepted.
- `pins`: Optional array of pin objects (for multi-pin nodes)

### Pin Object
- `id`: Required string (pin identifier)
- `direction`: Required, `in` or `out`
- `type`: Required, `frequency`, `signal` or `velocity`

### Wire Object
- `from`: Required string (source node id)
- `fromPort`: Required string (source port id)
- `to`: Required string (destination node id)
- `toPort`: Required string (destination port id)

### Box Object (layout)
- `id`: Required string (matches a node id)
- `x`, `y`: Required integers (canvas position)
## Enumerations

| Attribute (processor) | Values |
|-----------------------|--------|
| `waveform` (osc) | sine, saw, square, triangle, noise, perlin, simplex |
| `mode` (osc) | pitch, static, lfo |
| `mode` (filter) | lp, bp, hp, notch, ap |
| `shape` (shape) | soft, hard, fold, quantize |
| `sync` (delay) | off, on |
| `division` (delay) | half, quarter, eighth, sixteenth |
| `sync` (hold) | off, on |
| `division` (hold) | half, quarter, eighth, sixteenth |
| `interval` (fscale) | none, octave_up, octave_down, fifth_up, fifth_down, fourth_up, fourth_down, minor_third_up, major_third_up, semitone_up, semitone_down |

The names match the APVTS choice order declared in
`PluginProcessor::createParameterLayout()`.
## Dynamic Attributes by Node Type

One section per processor - the YAML mirror of the explicit per-processor
complexTypes in `smolfm.xsd`.  The lists are exhaustive: keys not listed
for a node type are ignored by the parsers.

### Oscillator (osc)
- `waveform`: sine | saw | square | triangle | noise | perlin | simplex
- `mode`: pitch | static | lfo
- `staticfreq`: float, 20..20000 Hz (log scale; default 440)
- `lforate`: float, 0.01..50 Hz (default 1)

### FM Modulator (fm)
- `amount`: float, 0..10

### AM (am)
- `amount`: float, 0..1 (0 = dry pass-through, 1 = full modulation)

### Frequency Scale (fscale)
- `factor`: float, 0..10 (default 1 = transparent)
- `interval`: none | octave_up | octave_down | fifth_up | fifth_down | fourth_up | fourth_down | minor_third_up | major_third_up | semitone_up | semitone_down (default none = transparent; each name stands for its exact 12-TET ratio and multiplies with `factor`, so `interval: octave_up` at `factor: 1.0` equals `factor: 2.0`)

### Velocity Scale (vsc)
- `curve`: linear | sqrt | square (default linear = transparent; sqrt fattens soft hits, square reserves the top of the range for hard accents)
- `scale`: float, 0.25..4 (default 1 = transparent; multiplies with the curve, output clamps to 0..1)

### ADSR (adsr)
- `attack`, `decay`: float, 0.001..5 s
- `sustain`: float, 0..1
- `release`: float, 0.001..10 s

### F-ADSR (fadsr)
- `attack`, `decay`: float, 0.001..5 s
- `sustain`: float, 0..1
- `release`: float, 0.001..10 s
- `up`, `down`: float, 0..10 (frequency multiplier at E=1 / E=0; default 1)

### Gain (gain)
- `factor`: float, 0..10 (default 1 = unity)

### Waveshaper (shape)
- `drive`: float, 0..1
- `shape`: soft | hard | fold | quantize

### Filter (filter)
- `cutoff`: float, 20..20000 Hz (log scale; default 1000)
- `resonance`: float, 0..1
- `mode`: lp | bp | hp | notch | ap (ap = allpass, leaves the magnitude flat and shifts the phase)

### Delay (delay)
- `time`: float, 1..2000 ms (free-running mode; default 250)
- `feedback`: float, 0..0.95 (default 0.3)
- `mix`: float, 0..1 (default 0.3)
- `sync`: off | on (on = tempo-synced, `time` is ignored)
- `division`: half | quarter | eighth | sixteenth (delay length; default quarter)

### Sample Hold (hold)
- `rate`: float, 0.5..20000 Hz (hold grid in free-running mode; default 1000; a connected `rate_mod_in` port multiplies it)
- `sync`: off | on (on = tempo-synced, `rate` is ignored)
- `division`: half | quarter | eighth | sixteenth (hold length; default quarter)

### Resonator (resonator)
- `tune`: float, 20..8000 Hz (log scale; default 100; static fallback — a connected `tune` port overrides it entirely)
- `q`: float, 0..1 (selectivity/bandwidth of the coupling stage; 0.5 ≙ Q ≈ 1)
- `decay`: float, 0.01..10 s (t60 of the ring: the pole radius' decay to -60 dB; vel port scales it per `spread`)
- `spread`: float, 0.25..4 (vel-to-decay spread, t_eff = t · (1 + v·(spread − 1)); default 1 = neutral, accented hits ring longer)
- `strikelevel`: float, 0..2 (peak factor of the internal body strike; default 0 = strike off = byte-identical to the old behaviour)
- `striketime`: float, 0..0.2 s (window length of the internal strike; default 0 = off). The strike ignites the body at every note; its peak follows the RAW accent on the `strike_vel` port (no curve inside the node), so the injected energy scales with vel². The strike enters the coupling stage's input (x_eff = in + strike) and never the integrator states, so the retrigger contract (tail kept, energy adds) holds; unwired `strike_vel` defaults to 1.0 (neutral).

### Feedback (feedback)
- `amount`: float, 0..0.95 (per-pass damping of the recirculated loop, out = in + amount·fb_in; 0 = transparent pass-through; the cycle inherently carries 1 sample of delay)

### Ring Modulator (ring)
- No parameters (the node is parameter-free)

### Splitter (split)
- No parameters (the node is parameter-free); two outputs `outL`/`outR`

### Merger (merge)
- No parameters (the node is parameter-free); two inputs `in1`/`in2`

### Add (add)
- No parameters (the node is parameter-free); two inputs `in1`/`in2`;
  out = in1 + in2 channel-wise sum

### Master Output (output)
- `level`: float, 0..1 (default 0.8)

## Special Node Types

### Note Input (note)
- No dynamic attributes
- Provides frequency output

## Legacy (version 2)

Files without `graph:`/`layout:` keep the flat structure - `nodes` and
`connections` at the top level, `x`/`y` on the node itself - and may spell
enumerated values numerically (`waveform: 1`).  The readers accept both;
the writer (`SmolFmFile::save`) always emits version 3.

## Validation Rules
1. All node IDs must be unique
2. Connection `from` and `to` must reference existing node IDs
3. Port IDs must match the node's pin definitions
4. Frequency ports connect to frequency, signal to signal
5. Layout box IDs must reference existing node IDs
