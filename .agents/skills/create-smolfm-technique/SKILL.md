# SmolFM Technique Skill

Expert skill for discovering, describing and registering **synthesis
techniques** for the SmolFM `.smolfm` format as YAML technique files under
`doc/techniques/`.

Use whenever the user asks for new synthesis techniques, asks what the
format can express, or wants the technique catalogue grown.  Produces one
descriptive YAML file per technique, following the embedded template, and
validates feasibility against the format grammar before anything is written.

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