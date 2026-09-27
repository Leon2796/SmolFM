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
- `instruments/fm-bell.smolfm` is the canonical, writer-style example:
  `<?xml version="1.0" encoding="UTF-8"?>` + `<SmolFM version="3">` (capital
  FM — the app's format sniffing and parser require exactly that root tag)
  with `Graph`/`Nodes`/`Connections` and `Layout`/`Boxes`.  New instrument
  files look exactly like it.

---

## Workflow (follow in order)

1. **Understand the target sound.**  From the user's description (genre,
   Klangbild, instrument type — and any explicit wishes like brightness,
   attack character, movement, space) derive a sonic target: spectral
   character, envelope behavior, dynamics, modulation movement, articulation.
   If the description is ambiguous, ask before building.
2. **Understand the grammar.**  Read `doc/formats/smolfm.xsd` and
   `doc/formats/smolfm-yaml-grammar.md`: node types with their
   `purpose`/`inputs`/`outputs`/`maxInstances` annotations, enums
   (`waveform`, `mode`, `shape`, filter modes, delay sync/divisions) and the
   numeric ranges of every attribute.
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
   allows; every node must earn its place.
5. **Write the file(s).**  One descriptive kebab-case filename per
   instrument (`<name>.smolfm`), written to `instruments/`, in version-3
   XML exactly like `instruments/fm-bell.smolfm` (see the embedded skeleton
   below).  Include a `Layout` box for every node.  When the user asked for
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
<SmolFM version="3">
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

## Node and port map (confirm against the xsd annotations)

Signal kinds: `frequency` and `signal` only; a wire connects like with like.

| Node      | Inputs                          | Outputs              | Budget |
|-----------|---------------------------------|----------------------|--------|
| `note`    | —                               | `out` (frequency)    | 1 |
| `osc`     | `note_in` (frequency)           | `out` (signal)       | 8 |
| `fm`      | `freq_in` (frequency), `modulator_in` (signal) | `out` (frequency) | 4 |
| `fscale`  | `freq_in` (frequency)           | `out` (frequency)    | 8 |
| `adsr`    | `in` (signal)                   | `out` (signal)       | 8 |
| `fadsr`   | `freq_in` (frequency)           | `out` (frequency)    | 4 |
| `gain`    | `in` (signal)                   | `out` (signal)       | 8 |
| `shape`   | `in` (signal)                   | `out` (signal)       | 8 |
| `filter`  | `in` (signal), `cutoff_mod_in` (frequency) | `out` (signal) | 8 |
| `delay`   | `in` (signal)                   | `out` (signal)       | 8 |
| `ring`    | `in1`, `in2` (signal)           | `out` (signal)       | 4 |
| `am`      | `carrier_in`, `modulator_in` (signal) | `out` (signal) | 4 |
| `output`  | `in1`…`in8` (signal)            | —                    | 1 |

Attribute ranges live in the xsd simpleTypes (FM/AM amounts, envelope times,
cutoff 20–20000 Hz, delay 1–2000 ms, feedback ≤ 0.95, master level 0–1) —
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
   as judged by the technique catalogue's `perceptual_effects`.
8. The file starts with the XML declaration and the root tag is spelled
   `SmolFM` exactly.
9. No input port receives more than one `Wire` — parallel voices are summed
   at the `output` node's inputs, never merged into a shared downstream
   input.

## Common pitfalls

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

## Boundaries

- Only create new instrument files under `instruments/` and this skill's
  own directory.  **Never modify or delete existing skills** (in particular
  `create-smolfm-technique` and its catalogue contract) — the technique
  catalogue is read-only input for this skill.
- Never edit processor sources or the xsd to "make a patch possible"; if
  the grammar cannot express something, design around it with the nodes
  that exist.