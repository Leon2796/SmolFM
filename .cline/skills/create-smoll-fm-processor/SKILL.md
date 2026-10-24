---
name: create-smoll-fm-processor
description: >
  Expert skill for managing processors in SmolFM over their full lifecycle:
  adding a complete new processor (vertical slice: DSP core, voice graph,
  GraphNodeRegistry, palette, editor factory, GUI component, .smolfm
  (de)serialisation, documentation), changing an existing one under the
  compatibility guard, auditing it against its doc/processors/*.md and
  harvesting processor candidates. Use when the user asks for a new
  node/processor type, asks how to add one, or wants an existing processor
  changed, checked or mined. Accepts the invocation options:
  anlegen | ändern | minen | health-check | port.
---

# SmolFM Processor Skill

Expert skill for **processors in SmolFM**. The
core is the **vertical slice** for a complete new processor: DSP core → voice
graph → GraphNodeRegistry → palette button → editor factory → GUI component
→ `.smolfm` (de)serialisation → documentation. The same layer map also drives
managing an existing processor: changing it (Compatibility guard below),
auditing it against its `doc/processors/*.md`, mining processor candidates
from real usage.

Use whenever the user asks for a new node/processor type, asks how to add
one, or wants an existing processor changed, health-checked or mined.
Produces a checklist-driven, minimal-diff plan that keeps every layer
consistent — for new processors and changes to existing ones alike.

---

## Options (args)

When the agent invokes this skill it may pass one option string (`args`).
The five operations come from the agent's routing table
(`Anlegen`/`Ändern`/`Minen`/`Health-Check`/`Port`); everything else in the
args is part of the request. Free prose still triggers the same work — the
option just removes ambiguity. Ambiguous args → ask before implementing.

| args | Mode |
|---|---|
| `anlegen <node>` (default) | Full vertical slice for a new processor (layered checklist below) |
| `ändern <processor>` | Change to an existing processor — the **Compatibility guard** (section below) applies: check breakage, ask before signal-processing changes |
| `minen` | Read-only harvest: scan instruments, improvement plans and teaching docs for repeated wirings and missing nodes → processor candidates (Reinforcement-Ziel); report, don't build |
| `health-check <processor>` | Audit an existing processor against its `doc/processors/*.md`, the template rules and the Common pitfalls (doc drift, ID/attribute spelling in every required place) — findings only |
| `port <algorithm>` | Vertical slice derived from a known DSP algorithm or external reference; translate honestly into existing Bausteine, name deviations |

The layered checklist below is written for the vertical slice (`anlegen` /
`port`); an existing processor can be **managed with the same skill**:
`ändern` walks those layers on the existing processor (consistency check
instead of building), `health-check` audits them step by step.

---

## Mental model (read first)

SmolFM is a **per-voice graph**:

- Each of the 8 polyphonic `SynthVoice`s owns a **pool** of every processor
  type (`std::array<Thing*, maxThings>`), pre-allocated.
- A node on the UI canvas (`DraggableComponent` with a `boxId` like `ring2`)
  is just a *view* onto instance `N` of that pool.
- Wiring lives in `ConnectionPatch` (`from = {nodeId, portId}` → `to = {...}`)
  and is pushed to every voice's `applyConnectionPatch`, which re-points
  atomic `InputPort`/`OutputPort` connections.
- A processor exists even when invisible/unwired; unconnected inputs read
  their per-port **default value** (0 for signal, 440 for frequency) — so
  unwired nodes are inert, not absent.

**Port domain** is part of the type: `PortType::signal` (audio) or
`PortType::frequency` (Hertz). Connections between mismatched types are
rejected at `tryConnect` time.

**Parameter model**: APVTS owns all mutable state. Raw `std::atomic<float>*`
pointers are cached in `SynthVoiceParameters` at construction; UI attachments
bind sliders/combos directly. No locks on the audio thread.

---

## Compatibility guard (signal processing changes)

Adding a brand-new processor is additive by design: nothing in a saved patch
depends on it. But when work changes the **signal processing** of an existing
processor — its algorithm/behaviour, what a parameter does or its default,
internal routing, port layout, or render order relative to other processors —
do not apply it silently:

1. Briefly check whether existing `.smolfm` instruments could break:
   structurally (dangling connection ids, parameters silently dropped by
   the read whitelist, xsd/grammar mismatch on reload) or audibly (the same
   patch suddenly behaves or sounds differently, audio rerouted or silent, layout changed).
2. If breakage is plausible, briefly ask the user (name the concrete risk)
   before applying the change.

---

## The layered checklist(Vertical Slice)

Written from the **`anlegen`** perspective: adding processor `Foo` (baseId
`foo`) touches **every** layer below, in this order. The same layer map is
the backbone for every mode — `ändern` walks the layers on an existing
processor, `health-check` audits each one against its docs. Skipping one
leaves a half-wired node: palette tile appears but sound never reaches the
output, or the `.smolfm` save/load round-trips nothing.

### 1. DSP — `src/processors/FooProcessor.{h,cpp}`

Inherits `Processor`. Contract:

- `prepare(double sampleRate)` — allocate zero; called once per voice.
- `startNote()` — reset any per-note state (phases, envelopes). Called from
  `SynthVoice::startNote`.
- `processSample()` — **must be real-time safe**: no allocations, no locks,
  no atomics beyond the port reads. Read inputs via `inputN.getSample()`
  (returns a `StereoFrame` L/R pair), write via `output.setSample(frame)`,
  return `frame`. Frequency ports keep their scalar semantics and travel as
  centered pairs (read the `.l` channel).
- Ports are plain members: `InputPort {PortType::signal}` /
  `OutputPort {PortType::..., *this}`.
- Constructor: pass the role to `Processor(role)` and set per-port default
  values (`setDefaultValue(0.0f)` for signals, `440.0f` for frequency in
  most cases) so an unwired node produces silence / neutral values, not
  garbage.

If the processor has mutable parameters, it stores `std::atomic<float>*`
pointers handed in at construction (see `FMModulationProcessor` for the
pattern). Never read the APVTS directly from `processSample`.

**Effective-value telemetry (UI ghost needle).** The needle is
slider × mod-port.  Whenever the processor computes a per-sample *effective*
parameter value that a mod port bends — and then throws it away with the
return sample — it also feeds that value into a
`NeedleWindowTelemetry uiEffective<Control>` member (see
`src/EffectiveRangeTelemetry.h`): a `.sample (v)` call at the exact site the
effective value is final — before the velocity scaling, so the needle pins
the mod position, never the accent — and a thread-safe getter
(`getEffective<Control>Range()`; memory precedent:
`MasterOutputProcessor::peakLevel`).  The struct keeps a min/max band over
a 256-sample window (~5 ms) and folds it into relaxed atomics: a single
per-sample overwrite aliased against the overlay's 30 Hz poll and made the
needle jump to arbitrary slider points even for a pure sine.  The UI's
`ModIndicatorOverlay` polls that band over `SynthVoice::getEffectiveControlRange`
→ `AudioPluginAudioProcessor::getEffectiveControlRange` → provider lambda
and animates: one needle chasing the band's centre with meter ballistics (a
slow mover — e.g. an LFO sine — pendulates between its true extremes; an
audio-rate mover blurs toward the band centre), plus the band arc as an
instant-attack / ~0.5 s-release envelope of the observed bands.  Skip any
part and the control stays a static lie while its mod port is wired.
Velocity deliberately never feeds the needle — it stays sound-only, so a
vel-only mover (resonator decay) gets no needle at all.

### 2. Voice wiring — `src/SynthVoice.{h,cpp}`

Three places, all inside `namespace smolfm`:

1. **`SynthVoice.h`** — add to `SynthVoiceParameters` **only if** the
   processor has APVTS params; always add the pool pointer array:
   ```cpp
   std::array<FooProcessor*, GraphNodeRegistry::maxFoos> fooProcessors {};
   ```
2. **`SynthVoice.cpp` constructor `buildGraph()`** — allocate the pool and
   register with the `SignalGraph`:
   ```cpp
   for (int i = 0; i < GraphNodeRegistry::maxFoos; ++i)
   {
       auto foo = std::make_unique<FooProcessor>( /*params*/ );
       fooProcessors[(size_t)i] = foo.get();
       graph.addProcessor (std::move (foo));
   }
   ```
   Order within `buildGraph` is the topological render order, so put the new
   block after its frequency-domain producers (after `note`, `fm`, `fscale`)
   and before signal consumers (`ring`, `adsr`, `output`).
3. **`resolveInput` / `resolveOutput` helpers** (file-local statics near the
   top of `SynthVoice.cpp`) — add a branch that maps
   `(nodeType == NodeType::foo && portId == "inN"/"out")` to
   `&fooProcessors[index]->getInputN()/getOutput()`. The function signatures
   already thread the pool array through; just extend the argument list.
   Both `disconnectInput`-style functions and `applyConnectionPatch` call
   these resolvers, so a missing branch silently breaks that port.
4. **`SynthVoice::getEffectiveControlRange`** — if the processor publishes an
   effective-value atomic (see §1), add a `case` for its `NodeType` (and the
   `controlIndex`, filter: 0 = cutoff, 1 = resonance) returning a `(v, v)`
   range.  The plugin-level aggregator in `AudioPluginAudioProcessor` is
   generic and needs no change.

### 3. Registry — `src/graph/GraphNodes.{h,cpp}`

- New `NodeType::foo` in the enum (`GraphNodes.h`).
- Budget constant `static constexpr int maxFoos = N;` next to the others.
- `buildSpecs()` — a `NodeSpec` block:
  ```cpp
  {
      NodeSpec foo;
      foo.id = "foo";
      foo.title = "Foo";
      foo.outputPortId = "out";            // or "" for a sink
      foo.outputType = PortType::signal;   // or frequency
      foo.inputPortIds = { "in1", "in2" };
      foo.inputTypes = { PortType::signal, PortType::signal };
      // Exactly one of these if the processor has params; "" means none:
      foo.amountParameterTemplate = "foo%Amount";
      specs.push_back (foo);
  }
  ```
  The template literal `"foo%Param"` is what the registry replaces with the
  instance index to make the concrete APVTS id (`foo2Param`). It **must**
  match what `PluginProcessor::createParameterLayout()` registers.
- `typeOf()` — map `"foo"` → `NodeType::foo`.
- `maxInstancesOf()` — add a `case NodeType::foo: return maxFoos;`.
- If the processor has params, pick the matching helper and add a
  `case`/branch there too (e.g. `amountParameterIdFor`).

### 4. APVTS — `src/PluginProcessor.cpp → createParameterLayout()`

Add a loop for `i < maxFoos` creating one parameter per instance (or the
single master param for singletons). Follow the ID scheme
`"foo" + num + "ParamName"`. Register the raw pointer caches in the
constructor (`voiceParameters.fooParam[i] = parameters.getRawParameterValue(...)`).

### 5. GUI component — `src/gui/components/FooComponent.{h,cpp}`

Minimal panel that binds the processor's APVTS parameters:
- Constructor takes `(juce::AudioProcessorValueTreeState& apvts,
  juce::String paramId1, ...)` — keep it free of node knowledge; the caller
  resolves the concrete instance id.
- Attach via `SliderAttachment` / `ComboBoxAttachment` members (store as
  `std::unique_ptr`, create in the ctor body).
- `setSize(…)` in the ctor — the box frame reads it via `getPreferredSize`.
- Parameterless node (like `RingModulatorComponent`) is fine: a static label
  describing the math (`in1 × in2`) is enough.
- If the processor publishes an effective-value atomic (see §1), the
  constructor takes one trailing
  `std::function<juce::Range<float>()>` per modulatable control and builds a
  `ModIndicatorOverlay` on that slider (see `FMModulationComponent::amountOverlay`).
  The provider callback stays the editor's business — the component knows no
  processor.

### 6. Editor factory + palette — `src/PluginEditor.{h,cpp}`

- `makeFooContent(instanceId, apvts)` free function in the anonymous
  namespace, mirroring the existing `makeRingModulatorContent` /
  `makeOscillatorContent`. It translates `instanceId → APVTS id` via
  `GraphNodeRegistry::amountParameterIdFor(...)` and returns the component.
- `paletteButton` member `fooButton` + `configure("foo", "Foo", <icon lambda>,
  GraphNodeRegistry::maxFoos)` in the ctor — the lambda returns a
  `juce::Path` sketched in the button's local rect.
- Add `&fooButton` to the loop that calls `addAndMakeVisible` /
  `onAddRequested`.
- `addNodeFromToolbar()`: add `{ "foo", makeFooContent }` to the factory map.
  If `makeFooContent` takes provider parameters (modulatable control, §5),
  keep the static map capture-free and add a runtime
  `if (baseId == "foo")` branch next to the `output` precedent instead —
  capturing `this` into the `static` map would dangle when the editor is
  recreated.  Build the provider with the `effectiveRangeProvider` helper.
- `onCreateMissingNode` lambda: add `if (baseId == "foo") return
  graphPanel.addNodeOfType("foo", ..., makeFooContent, false);`
- `refreshToolbarBadges()`: `fooButton.setRemaining(maxFoos -
  graphPanel.countBoxesOfType("foo"));`
- `resized()`: append `fooButton` to the palette strip.

### 7. `.smolfm` serialisation — `src/graph/SmolFmFile.cpp`

- `parametersForNode(nodeId)` — add a branch that returns the
  `NodeParameterSpec` list for this baseId, mapping a short XML attribute
  name to the concrete APVTS id. Reuse the registry helper
  (`amountParameterIdFor(nodeId)`).
- Pin round-trip is automatic from the `NodeSpec` in §3 — nothing to do.

The save side already walks the specs; the load side already asks
`onCreateMissingNode` — once §3 and §6 are done, load/save *just work*.

- **Format grammars follow the processor**: every new processor gets an
  explicit per-processor entry in both format grammars:
  1. `doc/formats/smolfm.xsd` - one explicit complexType `FooNodeType`
     (xs:extension of `BaseNodeType`, semantic part) owning only the
     attributes this processor has, with a comment naming the baseId and
     an `xs:appinfo` annotation (namespace `urn:smolfm:node-meta`) with
     `purpose` (max 2 sentences: what the processor does and what it is
     used for - signal processing only, no sound-design advice), a
     `docReference` pointing at the processor md created in step 9
     (e.g. `doc/processors/FooProcessor.md`), its `inputs`/`outputs`
     port lists (`id` + signal `type`) and a `maxInstances` tag
     mirroring `GraphNodeRegistry::maxFoos`;
     enumerated parameters reference a named simpleType (like
     `WaveformType`) and float parameters a range simpleType with
     minInclusive/maxInclusive taken from the APVTS `NormalisableRange`
     (like `StaticFreqRangeType`, 20..20000 Hz); parameter-free processors
     get an empty extension (see `RingModulatorNodeType`).
  2. `doc/formats/smolfm-yaml-grammar.md` - a `### Foo (foo)` section under
     **Dynamic Attributes by Node Type** (one bullet per attribute with its
     named enumeration, or "No parameters"), plus a row in the Enumerations
     table for any new attribute; keep the YAML key identical to the XML
     attribute name.
  3. Parser whitelists, or the patch saves fine but silently loses the
     parameter on load: `knownAttributes[]` in `SmolFmXmlParser.cpp` and
     the key chain in `readNodeAttribute` (`SmolFmYamlParser.cpp`).  For a
     NEW enumerated value set: extend the name tables plus `enumOrNumber` /
     `enumValueText` in `ISmolFmParser.h`.
  4. `SmolFmParserTests.cpp` - add the attributes to both sample documents
     plus one CHECK on the parsed value.
  Positions are presentation: canvas coordinates belong to the `<Layout>`
  part (YAML `layout:` boxes), never to the semantic node block.

### 8. Build — `src/CMakeLists.txt`

Add both `.cpp` files (processor + component) to the `target_sources` list
between the `JUMAKE_SOURCES_BEGIN/END` markers (jumake keeps them sorted;
match the existing pattern).

### 9. Documentation — `doc/processors/FooProcessor.md`

**Template is mandatory**: `doc/processor-template.md` defines the fixed
4-section structure; never reorder, rename or drop sections.  Copy the file
and fill it in.  The file path is referenced by the NodeType annotation
(`docReference`) in `doc/formats/smolfm.xsd` - keep the annotation and the
file name in sync.

Standing template rules (binding for every task under this skill):

- **Update docs in the same work step.**  Docs and code must never drift
  apart: with a new processor *and* with every change to an existing one
  (ports added/removed, changed parameter-flow types, new parameters, new
  UI functions) the matching `doc/processors/*.md` file is updated **in the
  same work step**, not as a follow-up.
- **Reference symbols only.**  Name fields, types and functions with their
  file paths; keep long code routines and implementation detail out of the
  prose (formal math belongs to section 3).
- **New UI functionality (section 2).**  Section 2 holds GUI mode toggles
  and styling options.  When such a feature appears for the first time,
  extend the template itself (new column or subsection in section 2) and
  afterwards migrate **all** `doc/processors/*.md` files to the new
  structure.
Sections:

1. **Ports & real parameters** — copy the tables, replace names; if no
   parameters, write `*(keine)*`. Tabelle 1b lists **every input and output
   port in its own row** (one port = one parameter, set by the wiring rather
   than by a slider); Tabelle 1c has one row per APVTS parameter. Both tables
   carry the mandatory `Kurzbeschreibung` column, written for DSP beginners:
   short sentences, everyday words, one physical/mental analogy when it helps.
   Each description states (1) what flows in/out, (2) the unwired default +
   what happens then, (3) how the port/parameter modulates — replaces a
   static value, multiplies it by (1+m), clamps — and how it shapes the
   isolated processor's sound. Fill `Unverbunden (Default)` from the real
   code (`setDefaultValue`, `jlimit`, velocity-neutral 1.0). Formal math
   stays in §3 (Mathematical description).
2. **UI configuration** — list only *mode toggles* (LFO switch, range
   buttons). If none, the `*(noch keine)*` row. Static labels count as UI
   but live in the same row.
3. **Mathematical description** — symbol table + KaTeX equations + special
   cases (unwired behaviour, edge ranges). Reference the exact C++ symbols
   from `FooProcessor.cpp`.
4. **Symbol ↔ code map** — every symbol from §3 gets a row pointing at the
   C++ variable/function and file.

Keep it short. The doc is for developers and for the patch-design skill,
not for end users.

### 10. Update the instrument-design skill

Every new processor **must** be registered so instruments generated from text
descriptions can reference it. The machine-readable registration is §7 (the
instrument skill treats `doc/formats/smolfm.xsd` as its binding grammar —
the complexType plus appinfo annotation IS the grammar entry). Three
human-readable spots follow:

1. **Node/port map in `.cline/skills/create-smol-fm-instrument/SKILL.md`** —
   one row in the "Node and port map" table
   (`` `foo` | <ins> | <outs> | <budget> ``), mirroring
   `GraphNodeRegistry::maxFoos` and the xsd port annotations exactly.
2. **Common pitfalls in the same SKILL.md** — only if the processor introduces
   a failure mode patch authors must know (new port kind, surprising unwired
   default). Skip for well-behaved nodes.
3. **Technique catalogue** — if the processor enables reusable wiring
   patterns, register them via `.cline/skills/create-smolfm-technique` (one
   user-approved `doc/techniques/*.yaml` per pattern). The instrument skill
   composes instruments FROM that catalogue; embedded ASCII wiring-snippet
   lists inside the instrument skill are legacy and no longer exist there.

Skip this and the new node is invisible to the patch-generation skill: palettes
show it, but no generated instrument will ever wire it in.

---

## Common pitfalls (already burnt into the codebase)

- **Default value**: without `input.setDefaultValue(0.0f)` an unwired input
  reads 440 Hz (signal ports default to 0, frequency to 440 — pick the right
  one for the domain).
- **ID drift**: the string `foo%Amount` appears in **four** places —
  `GraphNodes.cpp` spec, `PluginProcessor::createParameterLayout`,
  `SynthVoiceParameters` cache init, `SmolFmFile::parametersForNode`. A typo
  in any of them means: node visible, but parameter never written.
- **Attribute-name drift**: the short attribute (e.g. `amount`) must be
  spelled identically in six places - `SmolFmFile::parametersForNode`,
  `knownAttributes[]` in `SmolFmXmlParser.cpp`, the key chain in
  `SmolFmYamlParser.cpp::readNodeAttribute`, `doc/formats/smolfm.xsd`,
  `doc/formats/smolfm-yaml-grammar.md` and the sample documents in
  `SmolFmParserTests.cpp`. A missed spot means the patch saves fine but
  silently loses the parameter on load (whitelist) or documents a key no
  parser reads.
- **Missing resolver branch** in `SynthVoice`: node appears in the patch
  browser, wires render, but audio is silent because `resolveInput` returns
  `nullptr`.
- **Render order**: `graph.addProcessor` order = execution order per sample.
  Putting `foo` before its producers means it reads *stale* samples for one
  tick. Check where the existing processors sit before yours.
- **UI only binds the last instance** if you forget the loop/`maxFoos`
  budget — every visible `fooN` slider would control `foo0`.
- **Doc drift**: changing a processor (ports, parameter types, params, UI
  functions) without updating its `doc/processors/*.md` in the same work
  step — the template rule makes the same-step update mandatory, docs and
  code must never diverge.
- **Telemetry drift**: a mod-ported processor without its
  `NeedleWindowTelemetry uiEffective…` member — or a `NodeType` missing
  from `SynthVoice::getEffectiveControlRange` — compiles fine and keeps
  the ghost needle hidden for good (no band ever reaches the UI).  The
  `SmolFM_ModTelemetryTests` target only sees the processor side (it links
  no graphics module); the switch case is a manual checklist item.
- **`isEmpty()` as the telemetry no-data signal**: `juce::Range::isEmpty()`
  is true exactly when start == end, and the voice getter returns `(v, v)` —
  so filtering the aggregator (or the overlay) on `isEmpty()` discards every
  voice and the needle never draws (mono playback showed nothing, once).
  No-data is a NaN length (this repo's juce::Range ctor clips a reversed
  pair with jmax, so a negative length can never exist — the old inverted
  sentinel silently looked like data (1, 1)):
  `smolfm::noEffectiveData()` / `smolfm::effectiveRangeHasData()`
  (src/EffectiveRangeTelemetry.h); pinned by the "7) Range protocol" block
  in `ModTelemetryTests.cpp` — Debug only: a NDEBUG build compiles the
  asserts away and exits 1 by design.
- **Capturing lambdas in the static factory map**: the map is `static` and
  outlives editor recreation, so any lambda capturing `this` dangles on the
  second open.  Nodes needing an editor reference branch at runtime like
  `output` (PluginEditor.cpp).

---

## Minimal example trace

For a new **Wavefolder** (`fold` node, 1 input sample `in`, 1 output sample
`out`, 1 APVTS param `fold%Amount`):

1. `src/processors/WavefolderProcessor.{h,cpp}` — multiplier on
   `sin(π x amount)`; `ProcessRole::generic` (until the role matters).
2. `SynthVoice.h/cpp` — pool, resolver branches, constructor loop.
3. `GraphNodes.{h,cpp}` — `NodeType::wavefolder`, `maxWavefolders = 4`,
   spec, `typeOf`, `maxInstancesOf`, `amountParameterIdFor` case.
4. `PluginProcessor.cpp` — APVTS loop + voiceParameters cache.
5. `src/gui/components/WavefolderComponent.{h,cpp}` — one slider.
6. `PluginEditor.{h,cpp}` — factory + palette + toolbar mapping +
   `onCreateMissingNode`.
7. `SmolFmFile.cpp` — `parametersForNode` branch returning
   `{ "amount", wavefolderAmountIdFor(id) }`.
8. `src/CMakeLists.txt` — add the two new `.cpp` files.
9. `doc/processors/WavefolderProcessor.md` — fill the template.

Expected diff: ~12 files, mostly 3–8 lines each + the two new files.

Stop when the node shows up in the palette, patches save and load, and a
wired `in → out` produces sound. Anything beyond that (visuals, extra modes)
belongs to a follow-up task, not this vertical slice.
