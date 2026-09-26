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
- `type`: Required, `frequency` or `signal`

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
| `mode` (filter) | lp, bp, hp, notch |
| `shape` (shape) | soft, hard, fold |
| `sync` (delay) | off, on |
| `division` (delay) | half, quarter, eighth, sixteenth |

The names match the APVTS choice order declared in
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
- `shape`: soft | hard | fold

### Filter (filter)
- `cutoff`: float, 20..20000 Hz (log scale; default 1000)
- `resonance`: float, 0..1
- `mode`: lp | bp | hp | notch

### Delay (delay)
- `time`: float, 1..2000 ms (free-running mode; default 250)
- `feedback`: float, 0..0.95 (default 0.3)
- `mix`: float, 0..1 (default 0.3)
- `sync`: off | on (on = tempo-synced, `time` is ignored)
- `division`: half | quarter | eighth | sixteenth (delay length; default quarter)

### Ring Modulator (ring)
- No parameters (the node is parameter-free)

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
