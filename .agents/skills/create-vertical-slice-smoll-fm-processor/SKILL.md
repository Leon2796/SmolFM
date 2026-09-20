"# SmolFM Processor Vertical-Slice Skill

Expert skill for adding a **complete new processor** (vertical slice) to SmolFM:
DSP core → voice graph → GraphNodeRegistry → palette button → editor factory
→ GUI component → `.smolfm` (de)serialisation → documentation.

Use whenever the user asks for a new node/processor type, or asks how to add
one. Produces a checklist-driven, minimal-diff implementation plan that keeps
every layer consistent.

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

## The layered checklist

Adding processor `Foo` (baseId `foo`) touches **every** layer below, in this
order. Skipping one leaves a half-wired node: palette tile appears but sound
never reaches the output, or the `.smolfm` save/load round-trips nothing.

### 1. DSP — `src/processors/FooProcessor.{h,cpp}`

Inherits `Processor`. Contract:

- `prepare(double sampleRate)` — allocate zero; called once per voice.
- `startNote()` — reset any per-note state (phases, envelopes). Called from
  `SynthVoice::startNote`.
- `processSample()` — **must be real-time safe**: no allocations, no locks,
  no atomics beyond the port reads. Read inputs via `inputN.getSample()`,
  write via `output.setSample(x)`, return `x`.
- Ports are plain members: `InputPort {PortType::signal}` /
  `OutputPort {PortType::..., *this}`.
- Constructor: pass the role to `Processor(role)` and set per-port default
  values (`setDefaultValue(0.0f)` for signals, `440.0f` for frequency in
  most cases) so an unwired node produces silence / neutral values, not
  garbage.

If the processor has mutable parameters, it stores `std::atomic<float>*`
pointers handed in at construction (see `FMModulationProcessor` for the
pattern). Never read the APVTS directly from `processSample`.

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

### 8. Build — `src/CMakeLists.txt`

Add both `.cpp` files (processor + component) to the `target_sources` list
between the `JUMAKE_SOURCES_BEGIN/END` markers (jumake keeps them sorted;
match the existing pattern).

### 9. Documentation — `doc/processors/FooProcessor.md`

**Template is mandatory**: `doc/processor-template.md` defines the fixed
4-section structure; do not reorder or rename. Copy the file and fill it in.
Sections:

1. **Ports & real parameters** — copy the tables, replace names; if no
   parameters, write `*(keine)*`.
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

Every new processor **must** be registered in
`.agents/skills/create-smolfm-instrument/SKILL.md` so instruments generated
from text descriptions can reference it. Add / update all four spots:

1. **"Complete Processor Reference" table** — one row: `**FooProcessor** | `foo` | <max> | <ins> | <outs> | <1-line function>`.
2. **"Node ID Convention" table** — row: `` `foo` | `foo0` through `foo<N-1>` | 0-<N-1> ``.
3. **"Processor Port Details"** — a `#### FooProcessor` block matching the
   existing format (Purpose, Inputs, Outputs, Parameters, Behavior, special
   cases like unwired defaults).
4. **"Graph Wiring Principles"** — three short ASCII wiring snippets named
   after their use case, each 5–8 lines plus an "Example settings" list of
   concrete parameter values. Copy the style of the `#### Amplitude Modulation`
   entries. These snippets are what the instrument designer copies verbatim
   into new `.smolfm` patches.

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
- **Missing resolver branch** in `SynthVoice`: node appears in the patch
  browser, wires render, but audio is silent because `resolveInput` returns
  `nullptr`.
- **Render order**: `graph.addProcessor` order = execution order per sample.
  Putting `foo` before its producers means it reads *stale* samples for one
  tick. Check where the existing processors sit before yours.
- **UI only binds the last instance** if you forget the loop/`maxFoos`
  budget — every visible `fooN` slider would control `foo0`.

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
5. `gui/components/WavefolderComponent.{h,cpp}` — one slider.
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
"