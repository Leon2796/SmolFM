# SmolFM Improvement-Plan Skill

Expert skill for turning a user prompt into a structured **improvement
plan** under `doc/improvement-plans/` — the same evidence-first
gap-analysis format as `famous-synth-algorithms-gap-analysis.md` and
`stereo-effects-support-gap-analysis.md`.

Use whenever the user names a topic, synthesizer, effect or feature idea
and wants the format/engine gaps examined and proposed ("make an
improvement plan for …", "what can the format not express in …", "gap
analysis for …"). The plan is always **based on the user's prompt**: the
prompt fixes the topic and reference axis — do not widen the scope
silently, ask instead.

---

## Mental model (read first)

- An improvement plan is a **decision document**, not a wish list. Each
  finding states what the current format/engine can and cannot express,
  which known instrument or effect proves the gap, and gives exactly one
  concrete XSD/processor approach per gap — then priorities P1/P2/P3
  with dependencies.
- Ground truth lives in `doc/formats/smolfm.xsd` (the grammar), the
  processor sources under `src/processors/` and their docs under
  `doc/processors/` — the Abschnitt-1 tables there (1b ports in own rows,
  1c parameters, each with a `Kurzbeschreibung` column) are the binding
  wording for effect and modulation. Verify every claim against these files
  before writing it down — never plan from the memory of an older session.
- Every idea is examined **for both cases** (see
  `.github/copilot-instructions.md`): how it serves accessing famous
  instruments/sounds, and how it serves producing effects. A row that
  only covers one case is incomplete.
- Plans are written **in German**, as numbered sections, opening with a
  reference block citing the files (and sibling-plan sections) used.
- Sibling plans interlock — the stereo plan forks P3.11 of the synth
  plan. Read all existing plans under `doc/improvement-plans/` and mark
  dependencies/overlaps explicitly instead of duplicating rows.
- When a planned item ships later, it is annotated `**P_n erledigt**:`
  with proof (see the velocity row of the synth plan) — plans age by
  annotation, not by rewrite.

---

## Workflow (follow in order)

1. **Anchor on the prompt.** Identify topic and reference axis: known
   instruments (like the synth plan) or the effects themselves (like
   the stereo plan). If the prompt is ambiguous about scope or axis,
   ask before writing anything.
2. **Read the conventions.** `.github/copilot-instructions.md` (style,
   both-cases rule) and every existing plan in `doc/improvement-plans/`
   for tone, depth and section numbering.
3. **Gather evidence.** Read `doc/formats/smolfm.xsd` end to end plus
   the processor sources/docs that touch the topic. For each candidate
   item decide: expressible today / partially / not expressible — with
   the file (and construct) that proves it.
4. **Research the references.** Use your knowledge and online sources
   to bind each gap to a known synth/effect that demonstrates it, the
   way DX7 proves operator feedback or a ping-pong delay proves the
   missing channel dimension. No reference, no row.
5. **Shortlist review.** Present the candidate list (item,
   expressibility, known reference, rough priority) and let the user
   trim and reorder. No file is written before an explicit yes.
6. **Write the plan** to `doc/improvement-plans/<topic>-gap-analysis.md`
   (kebab-case, `-gap-analysis.md` suffix) using the skeleton below.
7. **Self-review.** Run the Review checklist (below) over the fresh
   draft and fix every finding before presenting the file.

### Document skeleton

Numbered sections like the sibling plans; the mandatory elements:

1. `# <Topic> — Lückenanalyse und Verbesserungsvorschläge` + blockquote
   listing references (files, sibling-plan sections built upon).
2. Framing paragraph: the reference axis and how this plan differs from
   its siblings.
3. Status-quo section (capability model / why the gaps exist), grounded
   in engine facts — e.g. the channel-less port domain, the mono
   `MasterOutputProcessor` summing formula, the zyklenfreie DAG.
4. Per-item analysis: one subsection per instrument/effect/concept with
   `**Ausdrückbar:**` / `**Nicht ausdrückbar:**` / `**Teilweise:**`
   plus `**XSD-Ansatz:**` — concrete node/parameter/port proposals.
5. Overview table `Fehlendes Konzept | Beschreibung | Bekannter
   <Synthesizer|Effekt> | Processor-Vorschlag (P1/P2/P3) |
   Klangeigenschaften`.
6. `Priorisierte Verbesserungen`: P1 (größter Expressivitätszuwachs,
   kleine XSD-Änderungen) → P2 (einzelne berühmte Algorithmen/Effekte
   erreichbar) → P3 (Architektur-/Host-Fragen, bewusst spät). Each
   entry names what it enables and which per-item numbers it lifts.

---

## Help

One-shot reference card for agents and users. Display it when someone
asks "what does the improvement-plan skill do", "how do I request a
plan", or needs the format rules — do not start the workflow unless it
is actually requested.

**Input → output**

| You give | You get |
|---|---|
| A topic/prompt: "plan / gap analysis for X" | `doc/improvement-plans/<x>-gap-analysis.md`, skeleton above, in German |
| "review the <topic> plan" | Review mode: one-line findings on an existing plan file |
| "X landed in the engine" | The plan's rows updated: `erledigt` annotation with proof |

**Hard rules (all modes)**

- Evidence before wording: every claim cites the xsd/processor source
  proving it.
- Both cases for every item: famous-instrument access AND effect
  production.
- One `XSD-Ansatz` per gap; no approach that breaks the zyklenfreie DAG
  unless flagged P3 with the risk named.
- Priorities P1/P2/P3 as defined in the skeleton; dependencies ordered
  (prerequisite before dependent).
- New plan files require the user's explicit yes; edits to existing
  plans stay small and local.

---

## Review

Focused review of an existing improvement-plan document (or a section
of it). One line per finding, in the house format
`<file>:L<line>: <what>. <fix>.` — findings only, no rewriting. The
best outcome for a plan is that it gets shorter and better-sourced.

Checklist, most damaging first:

- `evidence:` claim contradicts or ignores current code/xsd (a "cannot
  express" where the mechanism already exists). Verify against
  `doc/formats/smolfm.xsd` + `src/processors/`, then fix or cite.
- `stale:` planned item already implemented but not annotated — add
  the `erledigt` note with proof.
- `scope:` row without anchor in the user's prompt/sibling plans, or a
  silently widened topic — cut it or ask.
- `both-cases:` item assessed for instruments but not effects (or vice
  versa) — complete the missing case.
- `plan:` gap without a concrete `XSD-Ansatz`, or an approach the
  grammar/DAG cannot express — make it concrete and feasible.
- `prio:` priorities/dependencies unsorted (prerequisite must precede
  dependent, as stereo basics precede a stereo chorus) or a duplicate
  of a sibling-plan row — reorder and cross-reference.

Example:
`stereo-effects-support-gap-analysis.md:L51: evidence: chorus listed as not expressible, but time_mod landed per synth plan §5. Recategorize to teilweise.`