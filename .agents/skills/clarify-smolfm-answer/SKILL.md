# Clarify SmolFM Answer Skill

Re-explains answers (yours or earlier turns') so a DSP beginner understands
them, using **only functional terms that exist in the actual SmolFM software**.
Every unknown or 808-source jargon word gets translated into the vocabulary of
the real processors/patches, an analog↔digital mapping is drawn when the term
comes from the analog world, and every teaching ends with a glossary.

Use whenever the user says they did not understand an explanation, asks what a
term means, quotes a passage full of unfamiliar words, or asks how a described
behaviour maps onto the installed modules — also proactively after dense
analysis reports (loop math, level/velocity reports, reference-repo findings).

---

## Inputs

- **Optional argument:** a comma-separated list of the words/terms the user
  specifically did not understand (e.g. `RMS, Anregerkopie, pulse shaper`).
  Missing argument: infer the unclear terms from the quoted answer — every
  term that is (a) 808-Reference-repo jargon (pulse shaper, colour knob,
  exciter, trim, …), (b) DSP vocabulary (RMS, peak, dB, comb filter,
  group delay, …) or (c) refers to circuit-level analog behaviour.
- **Context:** the answer/passage to clarify (quoted by the user, or the
  assistant's most recent report), plus the relevant processor docs.

---

## Workflow (follow in order)

1. **Ground every claim in the installed software.**  For each functional
   statement in the answer, name the *existing* module that realizes it:
   check `doc/processors/<Processor>.md` (the processor docs are the binding
   vocabulary — Tabelle 1b lists every port in its own row and Tabelle 1c
   every parameter, each with a layman-friendly `Kurzbeschreibung` column;
   quote these checked descriptions instead of inventing paraphrases), the
   node ids/attributes the `.smolfm` patches actually use,
   and the measured test numbers (`SmolFM_GraphTests`, the stage probes) if
   levels/behaviour are involved.  If a needed building block does **not**
   exist, say so under a separate "Abweichungen" heading instead of
   paraphrasing it away.
2. **Translate the jargon.**  Every 808-source term gets a two-step mapping:
   analog meaning (what the circuit does) → SmolFM element (which processor,
   which port, which patch value).  Example: "pulse shaper click" → the
   8-ms `AdsrProcessor` burst from the noise `OscillatorProcessor`, copied
   through an own `GainProcessor` into a free `MasterOutputProcessor` input.
3. **Rebuild the explanation beginner-safe:**
   - short sentences, one idea each;
   - every new term defined inline at first use;
   - reuse the layman descriptions from the processor docs (Tabellen 1b/1c,
     `Kurzbeschreibung` columns) instead of coining fresh paraphrases;
   - numbers only from real measurements/tests (cite the probe/test that
     produced them); no invented values;
   - block diagram as ASCII with the actual node ids before the prose.
4. **Say what deviates.**  Under "Wie die Bausteine abweichen" list: what was
   solvable with existing blocks, and what the engine lacks vs. what the
   references assume (burst envelope, free-running phase bank, measured
   trim, …).
5. **Append the glossary.**  Every teaching ends with a glossary table:

   | Begriff | Bedeutung (einfach) | In SmolFM |

   It must cover (a) all optional-argument terms, (b) every term newly
   introduced in the teaching.  One row per term, plain-language meaning,
   and the concrete SmolFM module/parameter/patch location.
6. **Persist as a teaching file** under `doc/teaching/<topic>.md` (kebab-case
   name, e.g. `bass-drum-audibility-teaching.md`), so glossaries accumulate
   and later answers can reference earlier teachings.

---

## Embedded template

```markdown
# Teaching: <topic in plain words>

Diese Erklärung benutzt nur Bausteine, die es in SmolFM wirklich gibt
(<list of the processors involved>). Fachworte werden im Text übersetzt;
ein Glossar hängt am Ende.

## 1. Was in dem Patch/der Kette steckt (Bausteine, wie sie da sind)
<ASCII chain with REAL node ids + REAL parameter values>

## 2. Was beobachtet wurde (das Symptom, in Messzahlen wenn vorhanden)

## 3. Die Erklärung (analog ↔ digital, Baustein für Baustein)

## 4. Wie die Bausteine abweichen
- Gelöst mit vorhandenen Bausteinen: …
- Engine hat (noch) nicht: …

## 5. Analog ↔ Digital (Übersetzungstabelle)

## 6. Glossar (die neuen/unverständlichen Worte)
| Begriff | Bedeutung (einfach) | In SmolFM |
```

---

## Hard rules

- Never explain with a module that does not exist; verify names against
  `doc/processors/` and the xsd (`doc/formats/smolfm.xsd`) before using them.
- Never drop a claim that the installed software cannot fulfil into the main
  explanation — move it to the deviations list with an honest "nicht
  vorhanden".
- Keep measurements honest: label estimated values as "geschätzt", measured
  values with their source (test name).
- The glossary is mandatory, not optional — even for a two-paragraph answer.
- User-facing explanations default to German (the project language).

## Common pitfalls

- Explaining the reference repos' internals with 808 jargon instead of the
  installed SmolFM vocabulary — the user asked for the opposite.
- Quoting numbers without their measurement source.
- Glossary rows that only echo the term without the SmolFM mapping.
- Silently treating an engine deficiency as solved.