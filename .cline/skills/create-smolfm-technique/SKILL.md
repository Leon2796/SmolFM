---
name: create-smolfm-technique
description: >
  Expert skill for managing the SmolFM synthesis-technique catalogue over
  its full lifecycle: discovering, describing and registering techniques as
  YAML files under doc/techniques/, adjusting existing ones, auditing them
  against the format grammar and the catalogue, and mining candidates from
  research, instruments and real usage. Use when the user asks for new
  synthesis techniques, asks what the format can express, wants the
  technique catalogue grown, or wants existing techniques changed, checked
  or mined. Accepts the invocation options: anlegen | ändern | minen |
  health-check | port.
---

# SmolFM Technique Skill

Expert skill for **the SmolFM technique catalogue** — discovering,
describing and registering **synthesis techniques** for the SmolFM
`.smolfm` format as YAML technique files under `doc/techniques/`, and
managing that catalogue once it exists.

Use whenever the user asks for new synthesis techniques, asks what the
format can express, wants the technique catalogue grown, or wants existing
techniques changed, audited or mined.  Produces one descriptive YAML file
per technique, following the embedded template, and validates feasibility
against the format grammar before anything is written.

---

## Options (args)

When the agent invokes this skill it may pass one option string (`args`).
The five operations come from the agent's routing table
(`Anlegen`/`Ändern`/`Minen`/`Health-Check`/`Port`); everything else in the
args is part of the request. Free prose still triggers the same work — the
option just removes ambiguity. Ambiguous args → ask before writing.

| args | Mode |
|---|---|
| `anlegen <idea>` (default) | Propose new technique(s); no file is written before the user's explicit yes (Workflow 4) |
| `ändern <technique>` | Adjust the existing technique file instead of creating a parallel variant (Workflow 5) |
| `minen [<idea>]` | Read-only: mine research notes, instruments, improvement plans and the catalogue itself for candidate techniques — proposals only; for a described topology, additionally check whether it can meanwhile be expressed better given the current graph state, and which similar techniques have become expressible since |
| `health-check <technique>` | Feasibility + duplicate audit of existing technique(s) (Feasibility rules below): wrong node/port/param, range violations, catalogue duplicates — findings only; plus the fit check: does the described topology actually do what the technique's idea aims at? Explanations and technique comparisons are reported with the actual sub-graphs drawn as ASCII (rule in "Health-Check reporting: ASCII sub-graphs" below) |
| `port <reference>` | New technique from an external reference (synth architecture text, effect writeup): the reference comes with the args or is extracted from the prompt — if it is not clear what can be extracted, give the reasons, stop and ask; otherwise port the technique as faithfully as possible as `.yaml` via the `anlegen` proposal workflow (Workflow 4), and XSD feasibility decides |

The workflow below is written from the **`anlegen`/`port`** perspective; an
existing technique is **managed with the same material**: `ändern` adjusts
it instead of forking a parallel variant (Workflow 5), `health-check` runs
the Feasibility rules find-only and checks that the topology does what the
idea aims at, `minen` harvests candidates — and re-checks for described
topologies whether they are meanwhile better expressible given the current
graph state — and never writes.

---

## Health-Check reporting: ASCII sub-graphs (mandatory)

In `health-check` mode every explanation and every comparison between
techniques is drawn as an ASCII graph. The reader must see the **actual
sub-graph the technique represents** — reconstructed node-for-node and
wire-for-wire from the technique's `involved_nodes`, `involved_ports` and
`topology`, in the same shape the technique yields as a `.smolfm` wiring
(node type + role label per block, wires annotated with their port ids
and direction). Comparisons (duplicate suspicions, family audits, a
fit-check that mismatches the idea) place the candidate graphs side by
side, one column per technique. If a sub-graph cannot be reconstructed
from the technique's YAML — because the topology prose leaves wires
ambiguous, a cited port belongs to no listed node, or a claimed
connection has no source/target — give an explicit statement of that gap
in the findings instead of inventing wires: the non-derivable part is
itself a health-check finding.

---

## Technique docs: graph_ascii (mandatory), technique imports, instances, tutorial

1. **`graph_ascii` is mandatory.**  Every technique file carries an ASCII
   drawing of its real sub-graph, one line per wire where possible: node
   boxes as `(type · role)`, wires annotated with their port ids, `┄` for
   optional / mentioned-only wires and `╳` for a listed node that receives
   no wire in this topology.  The drawing must be derivable 1:1 from
   `involved_nodes`, `involved_ports` and `topology` — an ASCII line without
   a supporting wire or port is a health-check finding.  An importer draws
   the imported part as ONE collapsed `[ import: <name> ]` box plus its own
   external wiring points.
2. **Technique imports instead of re-describing reuse.**  A technique that
   reuses part of another technique's graph gets an `imports:` section
   (list entries with `name` / `reuses` / `delta`).
   - FULL duplicate: the importer omits `involved_nodes`, `involved_ports`,
     `topology` and `parameters` entirely — the import replaces them — and
     keeps only idea, framing and `perceptual_effects`.  The alias documents
     the same klang while the klang's canonical entry remains the importee
     (one entry, one klang).
   - PARTIAL reuse: the composite keeps its self-describing topology AND
     documents the borrowed part via `imports:`; the imported technique
     stays canonical.
   - `used_by` is mandatory in the imported technique: EVERY technique
     cited by another file's `imports:` carries a `used_by:` section
     listing the importer with a one- to two-line `detail` of what it
     takes.  The link is bidirectional — `check-catalog.ps1` validates it
     from both ends (every `imports` has its mirror entry, every `used_by`
     name imports its owner right back) — update both files when reuse
     changes.
   - Imports reference the CONCRETE catalogue graph — also when the target
     is itself an instance exemplar of a pattern registry.  Patterns are
     never imported (they are not executable graphs): applying a pattern
     means registering an `instances:` entry in the matching summarizing registry; reusing
     a specific graph means importing its technique.
3. **The summarizing registries.**
   Three pattern files summarize the catalogue's recurring skeletons —
   each lists in its `instances:` section every catalogue technique whose
   identity is such a pattern, and every technique identity registers in
   exactly ONE registry (single membership, checked catalogue-wide by
   check-catalog.ps1; techniques that only borrow a pattern as an
   ancillary stage stay standalone):
   - `doc/techniques/driver-patterns.yaml` abstracts the recurring driver
     patterns (periodic LFO, aperiodic perlin drift, event
     envelopes/impulses, static ratio/offset, velocity accent, key
     tracking, audio self-feedback; delay as a modulated target and the
     four driver-driven stereo spread techniques live here).
   - `doc/techniques/delay-patterns.yaml` abstracts the delay-line
     families: single cell (comb · slapback · beat tail), serial stages,
     parallel fanout.
   - `doc/techniques/stereo-branch-patterns.yaml` abstracts the split →
     per-side stage → merge routing: stage pairs (filter · gain · delay),
     direct-plus-staged branch, upstream modulator split, merge-free
     fanout.
   Each instance entry names
   the technique and carries its OWN `graph_ascii` — the concrete course of
   the pattern in THAT graph (same notation as point 1, pattern-view:
   driver, target socket, render path; an instance of a technique that
   itself imports draws the collapsed `[ import: <name> ]` box) — plus a
   `verlauf` short description of how the abstract pattern plays out and a
   `tonal` line naming the instance's tonal footprint.  The abstract
   skeleton stays drawn once at the top of the file; instances never re-draw
   it.  New techniques that fit an existing pattern are registered there;
   an existing pattern × target socket × Verlauf is a duplicate and becomes
   an adaptation or an import/alias, never a new parallel file.
4. **`tutorial:` block is mandatory in every technique file — the German
   on-ramp.**  It sits directly after the `idea:` block (the first
   section after the idea, never at the end of the file) and is written
   in German for readers who know neither the format nor synthesis
   jargon yet.  It is ≈ twice the `idea:` block and re-tells the file's
   substance accessibly — it adds no new substance.  As the SmolFM
    sound-design instructor speaking to the apprentice, it always opens
    by naming the technique in its real-world terms (the technique
    exists in studio and synth literature, the opening says so with the
    name the literature gives it: Delay/Echo, Stereo-Widening,
    Modulation/Patching; never file-meta like "Diese Datei ist
    eine Karte").  Then it answers the two questions: what is this wiring
    normally used for on a synthesizer, and what is the goal of using it:
   - **Psychoacoustic terms, not synth slang.**  Speak about what the ear
     does — Schwebungen, Maskierung, Rauheit, Obertöne, Lautheit,
     Transienten, Tonhöhenwahrnehmung — and steer clear of scene jargon
     ("supersaw", "wobble", "acid").
   - **Signal flow as narrative.**  Walk the signal in graph order
     (source → stages → ear); per stage, one clause on what happens to
     the signal there and what the ear hears as a consequence.
   - **Reuse relations in plain words.**  When present, say what
     `imports`/`used_by` share with the other technique and where the
     paths differ; in the summarizing registries, name the shared
     skeleton and point at the listed `instances` as its concrete plays.

---

## Mental model (read first)

- A **technique** is an abstract wiring/parameter pattern (a sub-graph and
  its modulation logic) — NOT a concrete patch.  It describes how nodes are
  connected and which parameter relationships make the pattern work.
- `doc/formats/smolfm.xsd` is the grammar that makes techniques expressible:
  one explicit complexType per processor, each annotated with `purpose`,
  `inputs`/`outputs` (port ids + signal kinds `frequency`/`signal`),
  `maxInstances` and range-typed attributes.  Connection endpoints are
  classed (`FrequencyInPinType` … `SignalOutPinType`).  A technique is only
  valid if the xsd can express its topology.
- Techniques are catalogue entries, not patches: they stay abstract (topology
  and parameter roles), concrete values belong to instruments.

---

## Workflow (follow in order)

1. **Understand the grammar.**  Read `doc/formats/smolfm.xsd` end to end:
   the node types with their `purpose`/`inputs`/`outputs`/`maxInstances`
   annotations, the pin endpoint classes, the named enums and the parameter
   ranges.  A technique may only use nodes, port ids, signal kinds and
   ranges that exist there.
2. **Read the catalogue.**  Read every existing `doc/techniques/*.yaml`;
   consult the summarizing registries' `instances:` sections (`driver-patterns`,
    `delay-patterns`, `stereo-branch-patterns`) first.  Anything
   with the same identity (same pattern into the same target zone,
   same Verlauf) and the same perceptual purpose is a duplicate — propose an
   adaptation of the existing technique or an import/alias instead of a new
   parallel file.
3. **Research.**  Use online sources (synthesizer architecture texts, FM
   cookbooks, subtractive and modular tutorials, effect-design writeups)
   plus your own knowledge to list candidate techniques that are (a) possible
   within the xsd and (b) not described yet.
4. **Ask before creating.**  Present the candidate(s) to the user in the
   template shape (name + idea + involved nodes + topology sketch).  The
   user decides whether a technique is worth having — no file is written
   before an explicit yes.
5. **Handle feedback.**  Work adjustments into the proposal or, when the
   user prefers an existing technique, edit that technique file instead of
   creating a parallel variant.

---

## Embedded template

Each technique file follows exactly this shape (`doc/techniques/template.txt`
is the canonical template):

```yaml
technique:
  name: <unique technique name>

  idea: |
    <Abstract description of the synthesis idea.
    Explain what the technique does, why it is useful,
    and how it generally affects the resulting sound.
    Keep the idea conformative with the possibilies given the .xsd format file.>

  tutorial: |               # MANDATORY — German on-ramp for DSP beginners,
                            # sits directly after the idea block (never at
                            # the file end): the sound-design instructor
                            # speaking to the apprentice
    <German tutorial for THIS technique, ≈ twice the `idea` block.  It
    must open by naming the technique in its real-world terms (the
    technique exists in the literature — never file-meta), then always
    answer the two questions: what is this wiring normally used for
    on a synthesizer, and what is the goal of using it.  Then
    introduce the key terms and ideas in plain psychoacoustic wording
    (Schwebungen, Maskierung, Rauheit, Obertöne, Transienten — not synth
    slang), walk the signal in graph order (source → stages → ear) and
    name each stage's audible effect, and tie the reuse relations in
    plain words — what `imports`/`used_by` share, which `instances` play
    this pattern and how.>
  involved_nodes:
    - type: <NodeType>
      role: <role of this node within the technique>

  involved_ports:
    - node_role: <role of node above>
      port: <port id / semantic port name>
      role: <what this connection does>

  topology:
    description: |
      <Describe the signal/modulation topology in abstract terms regarding the .xsd.
      E.g. can be a certain send or input patterns, but also a full subgraph(s). Just describe
      the implied topology.>

  graph_ascii: |            # MANDATORY in every technique
    <ASCII drawing of the real sub-graph, 1:1 derivable from
    involved_nodes/involved_ports/topology.  Boxes as (type · role),
    one line per wire with its port id; ┄ = optional/mentioned-only,
    ╳ = unwired node; an importer draws [ import: <name> ] as
    one collapsed box plus its own external wiring points.>

  parameters:
    - name: <parameter>
      role: |
        <What this parameter controls within the technique.>

  relationships:
    - name: <relationship>
      description: |
        <Important relationship between parameters,
        frequencies, envelopes, amounts, etc. >

  perceptual_effects:
    - <effect>

  imports:                  # optional — parts reused from another technique
    - name: <technique name>
      reuses: |
        <Which part of the imported technique's graph/parameters is reused>
      delta: |
        <What this technique adds or changes beyond the reuse
        (full duplicate: its framing; partial reuse: its own content)>
                            # full duplicate: omit involved_nodes/involved_ports/
                            # topology/parameters — the import replaces them

  used_by:                  # mandatory whenever another file lists you in its imports
    - name: <importing technique>
      detail: |
        <What that technique takes from this graph — the mirror of its
        import entry, kept in sync both ways (checked by check-catalog.ps1)>

  instances:                # summarizing/pattern techniques only
    - name: <technique name>
      pattern: <which driver pattern>
      graph_ascii: |
        <Concrete ASCII of THIS instance's course — driver, target socket,
        render path (same notation as the top-level graph_ascii)>
      verlauf: |
        <The more specific course of the abstract pattern in this graph>
      tonal: |
        <The instance's tonal footprint: timbre, movement, how it reads>


```

---

## Feasibility rules (validate before writing)

- Every `involved_nodes.type` must map to a processor complexType in the xsd
  and must not exceed its annotated `maxInstances` across the technique.
- Every `involved_ports.port` must be a port the referenced node declares in
  its `inputs`/`outputs` annotation, with the same signal kind
  (`frequency`/`signal`).
- Every `parameters.name` must be an attribute the xsd allows on that node
  and inside its documented range.
- The topology must be derivable from the node set — no hidden nodes, no
  port kinds the format does not carry.
- Every `imports.name` and every `instances.name` must name an existing
  `doc/techniques` catalogue entry, and the `reuses` text must be
  reconstructible from that entry's YAML (import integrity).
- `used_by` mirrors `imports` in both directions: every target of an
  `imports:` carries a matching `used_by` entry naming the importer and its
  `detail`, and every `used_by` name imports its owner right back
  (bi-directional integrity, checked by check-catalog.ps1).
- Every `instances` entry of a summarizing technique carries its own
  non-empty `graph_ascii` (the concrete pattern course, not the abstract
  skeleton), `verlauf` and `tonal`.
- Every technique file carries a non-empty German `tutorial:` block
  directly after its `idea:` block (never at the end of the file), ≈
  twice the length of its `idea:` block — the sound-design instructor
  voice that opens in the technique's real-world name (never file-meta)
  and always answers the two questions (what this wiring is
  normally used for, what the goal is) — plus psychoacoustic wording,
  the signal flow with its audible effect, and its reuse relations
  (`imports`/`used_by`/`instances`) woven in.

---

## Common pitfalls

- Proposing a technique the xsd cannot express: unknown node type, invented
  port id, wrong signal kind, or more instances than the budget allows.
- Duplicating an existing technique instead of adapting it.
- Forgetting the mirrored `used_by` entry in an imported technique, or
  registering an instance without its own `graph_ascii`/`verlauf`/`tonal`.
- A `tutorial` that is missing, not in German, sitting at the file end
  instead of directly after the `idea` block, failing to answer its two
  questions (what the wiring is used for, what the goal is), opening in
  file-meta ("Diese Datei ist eine Karte …") instead of the technique
  itself, written in synth jargon, skipping the signal-flow walk or the
  imports/instances ties, or not ≈ 2× the `idea` block.
- Sound-design prose in `purpose`/`topology` — signal processing only;
  taste belongs in `perceptual_effects`.
- Writing the file before the user approved the proposal.