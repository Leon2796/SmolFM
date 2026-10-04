# SmolFM Instrument Skill

Expert skill for designing and generating **SmolFM instruments** — concrete
`.smolfm` patches that realize a requested genre, Klangbild or instrument
type with the best possible sound under the format's (sometimes harsh) DSP
conditions.

Use whenever the user asks for a new instrument, a patch, a specific sound
("a bell, a growling bass, a glassy pad for genre X"), or a set of
instruments matching a description.  One or more `.smolfm` files are written
to `instruments/`, always built from techniques in the technique catalogue
and always valid against the format grammar.

---

## Mental model (read first)

You are the **Synthesizer Guru**.  Your judgment call: extract maximum
musical quality from the nodes and parameter ranges the format offers.  No
complaining about what is missing — every node type, every port, every range
is an opportunity.  A patch is only done when it sounds like the requested
instrument, not when it merely runs.

- A **technique** (see `doc/techniques/*.yaml`) is an abstract wiring/parameter
  pattern.  An **instrument** is the concrete patch: real node ids, real
  values, a real graph.  Instruments are built by *composing* techniques, not
  by inventing wirings from scratch.
- `doc/formats/smolfm.xsd` is the binding grammar: one explicit complexType
  per processor, each annotated with `purpose`, `inputs`/`outputs` (port ids
  + signal kinds `frequency`/`signal`), `maxInstances` and range-typed
  attributes.  Nothing may appear in an instrument that the xsd cannot
  express.
- `doc/formats/smolfm-yaml-grammar.md` documents the version-3 file
  structure (semantic `Graph` + presentation `Layout`), the dynamic
  attributes per node type, the enumerations and the validation rules.
- `instruments/fm-bell.smolfm` is the canonical, writer-style example:  a
  `<?xml version="1.0" encoding="UTF-8"?>` declaration followed by a version-3
  root element spelled `SmolFM` (capital FM — the app's format sniffing and
  parser require exactly that root tag)  with `Graph`/`Nodes`/`Connections`
  and `Layout`/`Boxes`.  New instrument files look exactly like it.

---

## No FX bus — both cases are mandatory

SmolFM has **no FX bus**: the graph is the only place where signal
processing happens, and `output` is the only summing point.  Effect
modelling (delay, flanger, chorus, …) must therefore be expressible
**directly inside the instrument graph** — never as a separate effects
chain outside it.

Every design consideration checks **both cases**:

1. **Instrument case** — the voicing of the onset (attack dynamics,
   accents, ghost notes, pitch movement).
2. **Effect case** — the same element as an effect stage inside the graph
   (per-note control, e.g. velocity → delay time/mix/feedback).

Rejecting an element with only "that's an effect case, not an instrument
case" is **not a valid justification**.  Effect cases are built in with the
instrument, or consciously deferred with a solid technical justification and
noted in the improvement plan (`doc/improvement-plans/`) — not silently
dropped.

---

## Workflow (follow in order)

1. **Understand the target sound.**  From the user's description (genre,
   Klangbild, instrument type — and any explicit wishes like brightness,
   attack character, movement, space) derive a sonic target: spectral
   character, envelope behavior, dynamics, modulation movement, articulation.
   If the description is ambiguous, ask before building.  Musicality is the
   default bar (see "Musicality and chaos mode" below): patches stay playable
   and tonally legible.  Only explicit chaos/experiment wording ("chaotic",
   "experimental", "extreme", "broken", "wild") activates the chaos mode —
   never infer it from a genre alone.
2. **Understand the grammar.**  Read `doc/formats/smolfm.xsd` and
   `doc/formats/smolfm-yaml-grammar.md`: node types with their
   `purpose`/`inputs`/`outputs`/`maxInstances` annotations, enums
   (`waveform`, `mode`, `shape`, filter modes, delay sync/divisions, fscale
   `interval`) and the numeric ranges of every attribute.
3. **Select the techniques.**  Read the technique catalogue
   `doc/techniques/*.yaml` and pick the techniques whose
   `perceptual_effects` match the target.  Compose several techniques into
   one instrument (e.g. an FM stage for the spectral core, an ADSR for the
   envelope, a filter/key-tracking stage for the tone, a delay for space).
   If nothing in the catalogue fits a needed aspect, design the missing
   wiring strictly from the xsd annotations — but prefer catalogue entries.
4. **Design the concrete patch.**  Assign node ids, choose concrete values
   inside the documented ranges (modulator ratios, FM amounts, envelope
   times, cutoffs, gains).  Budget the node instances against their
   `maxInstances` annotations.  Keep the graph as simple as the target
   allows; every node must earn its place.  Simple is not bare - never deliver a minimal demo patch (a naked two-operator sine bell, a single voice without articulation or shaping); an instrument gets an envelope, at least one colouring or space stage and enough voice richness to read as a finished playable instrument.
5. **Write the file(s).**  One descriptive kebab-case filename per
   instrument (`<name>.smolfm`), written to `instruments/`, in version-3
   XML exactly like `instruments/fm-bell.smolfm` (see the embedded skeleton
   below).  Include a `Layout` box for every node.  Fill the root metadata attributes (`name`, `description`, `techniques` - rules in "Root metadata" below).  When the user asked for
   a genre or Klangbild rather than a single instrument, deliver a small
   matching set (e.g. lead, bass, pad) as separate files.
6. **Validate before presenting.**  Run the checklist below.  Only present
   the patch once everything passes.
7. **Explain your sound design.**  Tell the user which techniques were used
   and why, and which parameters are the main performance knobs (the ones
   worth tweaking live).

## Embedded skeleton

The canonical shape of an instrument file (`instruments/fm-bell.smolfm` is
the reference):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<SmolFM version="3" name="<Name>" description="<one-sentence sound-design goal referencing the techniques>" techniques="<comma-separated technique names, empty ok>">
  <Graph>
    <Nodes>
      <Node id="note" />
      <Node id="osc0" waveform="sine" mode="pitch">
        <Pin id="note_in" direction="in" type="frequency" />
        <Pin id="out" direction="out" type="signal" />
      </Node>
      <Node id="fm0" amount="2.5">
        <Pin id="freq_in" direction="in" type="frequency" />
        <Pin id="modulator_in" direction="in" type="signal" />
        <Pin id="out" direction="out" type="frequency" />
      </Node>
      <Node id="adsr0" attack="0.001" decay="1.8" sustain="0.0" release="1.5">
        <Pin id="in" direction="in" type="signal" />
        <Pin id="out" direction="out" type="signal" />
      </Node>
      <Node id="output" level="0.8" />
    </Nodes>
    <Connections>
      <Wire from="note" fromPort="out" to="fm0" toPort="freq_in" />
      <Wire from="osc0" fromPort="out" to="adsr0" toPort="in" />
      <Wire from="adsr0" fromPort="out" to="output" toPort="in1" />
    </Connections>
  </Graph>
  <Layout>
    <Boxes>
      <Box id="note" x="60" y="60" />
      <Box id="osc0" x="1020" y="60" />
      <Box id="fm0" x="700" y="60" />
      <Box id="adsr0" x="1340" y="60" />
      <Box id="output" x="1660" y="60" />
    </Boxes>
  </Layout>
</SmolFM>
```

Notes on the skeleton:

- `Pin` elements are optional (the reference file omits them for `fscale`
  and `output`); when omitted, the port ids used in `Connections` must
  still match the ids the xsd annotation declares for that node type.
- Every file opens with the XML declaration shown above and spells the root
  tag `SmolFM` — the loader detects the XML flavour via that marker, and
  the app's own save always writes the declaration too.
- One wire per input port: an input holds a single source, so never fan
  several wires into the same `to`/`toPort` (the loader keeps only the last
  one and the engine replaces the source).  Sum parallel voices at the
  `output` node (`in1`…`in8`); when voices share a later stage
  (shape/filter/adsr/delay), duplicate that stage per voice with identical
  parameters instead of merging into it.
- Node ids are short, unique, indexed (`osc0`, `fm0`, `adsr0`, `fscale0` …).
- The `note` node is parameter-free (frequency output only); `output` is the
  only allowed sink (`in1`…`in8`, no out port).
- Enumerated values are written by name (`sine`, `lp`, `fold`, `on`,
  `eighth` …), numbers are plain decimals.

---

## Root metadata (name / description / techniques)

The root element carries the metadata group next to `version`:

```xml
<SmolFM version="3" name="Interval Fifth Bell"
        description="Glassy fifth-clang bell built from fscale-interval-modulator-fm: the FM modulator sits one exact pure fifth above the raw key, giving a key-stable bell grid"
        techniques="fscale-interval-modulator-fm">
```

- `description`: ONE concise sentence - the sound-design goal with the
  techniques that realise it.  No tutorial, no parameter dump.
- `techniques`: comma-separated names of the catalogue techniques actually
  used (`doc/techniques/*.yaml`).  Empty string is allowed when none of the
  catalogue applies, but a designed patch normally names what it composes.

---

## Musicality and chaos mode

**Default discipline — musical first.**  Unless the user explicitly asks for
chaos, every patch obeys these sound-design guardrails:

- **Pitch anchor**: at least one voice sits on the played key; every other
  voice sits on a musical relationship — named intervals, integer or small
  rational FM ratios, or cent detune within ±50 cents.  Inharmonic ratios
  and huge offsets are a *choice* for a named Klangbild (bell, metallic,
  growl), never random filler across all layers.
- **Modulation depth discipline**: the five `_mod_in` sockets
  (`fm amount`, `delay time`, `filter resonance`, `gain factor`, `shape
  drive`) run at musical depths by default — FM index swing ≤ 2x its base,
  resonance/drive LFOs at musical rates (≤ ~2x base), delay-time wobble a
  fraction of the echo time.  Full-depth audio-rate modulation belongs to
  chaos mode only.
- **Level budget**: two or more summed voices keep output inputs ≤ ~0.5
  and the master level ≤ 0.8 — headroom before the mix stops being
  playable.  Eight stacked saws at level 1.0 is always wrong.
- **Articulation per voice**: every voice has an envelope or a gate; no
  layer drones at full level forever.  Noise only ever appears shaped
  (filtered, enveloped) — an always-on raw noise layer is not musical.
- **Rhythmic coherence**: gate/chop rates (gain-gate-chops, pulse-stepper-
  grit) sit at musically sensible periods; detunes and drifts relate to the
  played tempo and key rather than to random hertz values.

**Chaos mode — opt-in only, musicality still guaranteed.**  When the user's
description explicitly asks for chaos/experiment, the discipline relaxes:

- allowed as the patch's core: audio-rate modulation at full depth, detune
  into ±1200 cents, stacked inharmonic ratios, always-on noise and metallic
  fold textures — the catalogue's extreme siblings (`self-drive-growl-fm`,
  `pulse-stepper-grit`, `auto-sweep-resonance`, `dub-wobble-echo-drift`,
  `fm-metallic-*` recipes) become legitimate foundations.
- still guaranteed, even in chaos: **one anchor** the ear can hold (a stable
  sub-voice on the key, a static centre frequency or a periodic pulse),
  **dynamic headroom** (master ≤ 0.8, level budget across layers so the
  instrument stays mix-able) and **controllability** (every voice keeps some
  articulation or gate so the instrument responds to playing, not just
  drones).
- chaos comes from parameter combinations, never from invalid values: stay
  inside every xsd range, and record in the root `description` what breaks
  (sideband density, unstable pitch, noise) and what anchors it — that
  sentence IS the sound-design contract of a chaos patch.

## Node and port map (confirm against the xsd annotations)

Signal kinds: `frequency`, `signal` and `velocity`; a wire connects like with like.

| Node      | Inputs                          | Outputs              | Budget |
|-----------|---------------------------------|----------------------|--------|
| `note`    | —                               | `out` (frequency), `vel` (velocity) | 1 |
| `osc`     | `note_in` (frequency)           | `out` (signal)       | 16 |
| `fm`      | `freq_in` (frequency), `modulator_in`, `amount_mod_in` (signal), `vel` (velocity) | `out` (frequency) | 8 |
| `fscale`  | `freq_in` (frequency)           | `out` (frequency)    | 16 |
| `adsr`    | `in` (signal)                   | `out` (signal)       | 16 |
| `fadsr`   | `freq_in` (frequency), `vel` (velocity) | `out` (frequency)    | 8 |
| `gain`    | `in`, `factor_mod_in` (signal), `vel` (velocity)  | `out` (signal)       | 16 |
| `shape`   | `in`, `drive_mod_in` (signal), `vel` (velocity)   | `out` (signal)       | 16 |
| `filter`  | `in` (signal), `cutoff_mod_in` (frequency), `resonance_mod_in` (signal), `vel` (velocity) | `out` (signal) | 16 |
| `resonator` | `in` (exciter, signal), `tune` (frequency, overrides static), `q_mod_in` (signal), `vel` (velocity → decay), `strike_vel` (velocity, raw accent for the internal strike) | `out` (signal) | 8 |
| `feedback` | `in`, `fb_in` (signal) | `out` (signal) | 4 |
| `delay`   | `in`, `time_mod_in` (signal)    | `out` (signal)       | 16 |
| `ring`    | `in1`, `in2` (signal)           | `out` (signal)       | 8 |
| `am`      | `carrier_in`, `modulator_in` (signal) | `out` (signal) | 8 |
| `output`  | `in1`…`in8` (signal)            | —                    | 1 |
| `split`   | `in` (signal)                   | `outL` (signal), `outR` (signal) | 8 |
| `merge`   | `in1`, `in2` (signal)           | `out` (signal)       | 8 |
| `add`     | `in1`, `in2` (signal)           | `out` (signal)       | 8 |

Attribute ranges live in the xsd simpleTypes (FM/AM amounts, envelope times,
cutoff 20–20000 Hz, delay 1–2000 ms, feedback ≤ 0.95, master level 0–1,
strike level 0–2, strike time 0–0.2 s) —
never write a value outside them.

## Validation checklist (all must pass)

1. Node ids unique; every `Wire` endpoint references an existing node id.
2. Every `toPort`/`fromPort` matches a port the node's xsd annotation
   declares, with the same signal kind (`frequency`↔`frequency`,
   `signal`↔`signal`).
3. Per node type, the instance count does not exceed its `maxInstances`;
   exactly one `note` and exactly one `output`.
4. All attribute values inside the documented ranges; enumerated values
   spelled as names.
5. Every node has a `Layout` box; every box references an existing node id.
6. The graph reaches the `output` node — no dangling unused signal chains
   unless they are deliberate modulation sources.
7. The patch audibly matches the requested genre/Klangbild/instrument type
   as judged by the technique catalogue's `perceptual_effects`, WITHIN the
   declared mode: default = musically legible (pitch anchor, musical ratios,
   level headroom); chaos mode = anchor + headroom + controllability are
   demonstrably present even as the texture breaks.
8. The file starts with the XML declaration and the root tag is spelled
   `SmolFM` exactly.
9. No input port receives more than one `Wire` — parallel voices are summed
   at the `output` node's inputs, never merged into a shared downstream
   input.

10. Root metadata filled: one concise `description` sentence (sound-design goal, referencing the techniques used) and `techniques` listing the catalogue names actually wired, comma-separated - empty strings only when genuinely nothing from the catalogue applies.

## Common pitfalls

- Delivering a bare demo patch (raw `fm`+`osc` bell, one unterminated voice, no envelope, no tone/space stage) instead of a finished instrument - the goal is instrument character, not a wiring diagram; only go minimal when the user explicitly asks for a minimal test patch.
- Letting the patch drift into unmusical chaos by default: audio-rate modulators at full depth, unanchored ±1200 detune stacks or always-on raw noise on a normal instrument request make the result unplayable noise.  Musicality is the default bar; chaos is opt-in via the user's explicit wording, and even then it keeps an anchor, headroom and controllability (see Musicality and chaos mode).
- Inventing node types, port ids or signal kinds the xsd does not declare.
- Blowing instance budgets or writing values outside the xsd ranges.
- Modulation wired into the wrong kind of port (e.g. signal into a
  frequency input).
- Ignoring the technique catalogue and hand-rolling a worse variant of an
  existing pattern.
- A technically valid but boring patch — the Guru re-tunes ratios, envelope
  shapes and amounts until the character is right.
- Forgetting the `Layout` section or leaving boxes without positions.
- Omitting the XML declaration or spelling the root tag `SmolFm` — the
  loader sniffs for the `<SmolFM` marker, and the app's save always writes
  `<?xml version="1.0" encoding="UTF-8"?>` as the first line.
- Fanning several wires into one input port — the engine's input holds a
  single source, so every wire but the last is silently dropped on load.
- Writing off an effect case with "that's an effect case, not an instrument
  case" — both cases are mandatory; an effect case is either built into the
  graph or consciously deferred with a technical justification and noted in
  the improvement plan (see "No FX bus — both cases are mandatory").

## Boundaries

- Only create new instrument files under `instruments/` and this skill's
  own directory.  **Never modify or delete existing skills** (in particular
  `create-smolfm-technique` and its catalogue contract) — the technique
  catalogue is read-only input for this skill.
- Never edit processor sources or the xsd to "make a patch possible"; if
  the grammar cannot express something, design around it with the nodes
  that exist.