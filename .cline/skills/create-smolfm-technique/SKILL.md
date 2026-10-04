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
| `health-check <technique>` | Feasibility + duplicate audit of existing technique(s) (Feasibility rules below): wrong node/port/param, range violations, catalogue duplicates — findings only; plus the fit check: does the described topology actually do what the technique's idea aims at? |
| `port <reference>` | New technique from an external reference (synth architecture text, effect writeup): the reference comes with the args or is extracted from the prompt — if it is not clear what can be extracted, give the reasons, stop and ask; otherwise port the technique as faithfully as possible as `.yaml` via the `anlegen` proposal workflow (Workflow 4), and XSD feasibility decides |

The workflow below is written from the **`anlegen`/`port`** perspective; an
existing technique is **managed with the same material**: `ändern` adjusts
it instead of forking a parallel variant (Workflow 5), `health-check` runs
the Feasibility rules find-only and checks that the topology does what the
idea aims at, `minen` harvests candidates — and re-checks for described
topologies whether they are meanwhile better expressible given the current
graph state — and never writes.

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
2. **Read the catalogue.**  Read every existing `doc/techniques/*.yaml`.
   Anything with the same topology and the same perceptual purpose is a
   duplicate — do not propose it again.
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

## Common pitfalls

- Proposing a technique the xsd cannot express: unknown node type, invented
  port id, wrong signal kind, or more instances than the budget allows.
- Duplicating an existing technique instead of adapting it.
- Sound-design prose in `purpose`/`topology` — signal processing only;
  taste belongs in `perceptual_effects`.
- Writing the file before the user approved the proposal.