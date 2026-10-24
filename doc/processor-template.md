# Template: Prozessor-Dokumentation

Dieses Dokument ist die **verbindliche Strukturvorlage** für alle Dateien unter
`doc/processors/`. Jede Prozessor-Dokumentation muss exakt diese vier Abschnitte
in dieser Reihenfolge enthalten.

## Verbindliche Regeln

1. **Struktur ist fest.** Abschnitte, Reihenfolge, Tabellenform und Überschriften
   dürfen nie geändert, umbenannt, umgestellt oder weggelassen werden. Ein
   Abschnitt ohne Inhalt bleibt stehen und trägt den Hinweis `*(noch keine)*`.
2. **Template-Referenz ist Pflicht.** Jede Prozessor-Datei beginnt mit der Zeile
   `# <Name>` und direkt darunter einer Referenzzeile auf dieses Template:
   `> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.`
3. **Konsistenz mit dem Code ist Pflicht.** Bei neuen Prozessoren und bei jeder
   Änderung an bestehenden Prozessoren (neue/entfernte Ports, Typwechsel im
   Parameterfluss, neue Parameter, neue UI-Funktionen) muss die zugehörige
   Markdown-Datei im selben Arbeitsschritt aktualisiert werden. Dokumentation
   und Code dürfen nie auseinanderlaufen.
4. **Symbole statt Codeblöcke.** Es werden nur Symbolnamen (Felder, Typen,
   Funktionsnamen) und ihre Dateien referenziert. Keine langen Code-Routinen,
   keine Implementierungsdetails im Fließtext.
5. **UI-Funktionen (Abschnitt 2).** Geplant sind GUI-Modi, die Styling oder
   Optionen eines Prozessors umschalten (z. B. LFO ↔ non-LFO per Checkbox,
   wählbarer Hertz-Bereich). Solche Funktionen werden in Abschnitt 2
   festgehalten. **Beim ersten Auftreten einer solchen Funktion ist dieses
   Template gezielt anzupassen** (neue Spalte/Unterabschnitt in Abschnitt 2) —
   und alle Prozessor-Dateien werden anschließend auf die neue Struktur gezogen.

6. **Kurzbeschreibungen sind Pflicht (Abschnitt 1).** Jede Zeile der
   Port-Tabelle (1b) und der Parameter-Tabelle (1c) trägt eine
   Kurzbeschreibung in Einsteiger-/Laien-Sprache: was der Port/Parameter
   bewirkt, wie er moduliert wird und wie er den Klang des isolierten
   Prozessors verändert (bildhafte Analogien erlaubt). Jeder Input- und
   Output-Port steht dabei in Tabelle 1b in einer eigenen Zeile — ein Port
   ist ein eigener Parameter.

## Aufbau

---

### Kopf

```
# <Prozessor-Name>
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

<1–3 Sätze Zweck>
```

---

### Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | `<n>` | `GraphNodeRegistry::max…` | `src/graph/GraphNodes.h` | `<wofür der Instanzen-Pool reicht>` |

Tabelle 1b: Ports — **jeder Input- und Output-Port steht in einer eigenen
Zeile**; ein Port ist ein eigener Parameter (er steuert dieselbe wirksame
Größe wie ein Regler, nur wird er von der Verkabelung bedient statt vom
Drehknopf).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `<id>` | Input / Output | `signal` / `frequency` / `velocity` | `<was der Port liest bzw. was dann passiert>` | `<1–3 Sätze: was hinein-/herausfließt, wie der Port moduliert (ersetzt einen statischen Wert / multipliziert ihn / klemmt …) und wie er den Klang des isolierten Prozessors verändert>` | `InputPort <sym>` / `OutputPort <sym>` | `<prozessor.h>` |
| `*(keine)*` | — | — | — | — | — | `—` |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| `<Anzeigename>` | `<id>` | `<Float/Choice/Bool, Bereich>` | `<1–3 Sätze: was der Regler bewirkt und wie er den Klang des isolierten Prozessors verändert>` | `std::atomic<float>* <symbol>` | `<prozessor.h>` |

Stilregel für die Spalten „Kurzbeschreibung" (1a/1b/1c) und für alle
Kurzbeschreibungen in den Prozessor-Dateien:

- Zielgruppe sind DSP-Einsteiger bis Laien: kurze Sätze, Alltagsworte, eine
  bildhafte Analogie, wo sie hilft (Schlägel auf Trommelfell, Dimmer an der
  Lampe, Sägezahn, hölzerner Klangkasten …).
- Jede Zeile beantwortet: (1) Was fließt hinein bzw. heraus? (2) Was passiert
  unverbunden/neutral? (3) Wie wirkt der Port/Parameter auf den Klang des
  isolierten Prozessors und wie wird er moduliert (ersetzt er einen statischen
  Wert, multipliziert er ihn mit (1+m), klemmt er, …)?
- Korrekt zum Code: Defaults (`setDefaultValue`), Grenzen (`jlimit`) und die
  Modulationsrechnung müssen exakt der Implementierung entsprechen. Die
  formale Mathematik gehört in Abschnitt 3.
- Ports, die in mehreren Prozessoren dasselbe Muster bedienen (z. B. `vel`,
  `*_mod_in`), werden konsistent gleich formuliert.

---

### Abschnitt 2 — UI-Konfiguration

Tabelle der zusätzlichen UI-Funktionen/Modi des Prozessors in der Oberfläche
(Styling-Schalter, Modus-Checkboxen, Bereichs-Umschaltungen o. ä.).

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| `*(noch keine)*` | | | |

Wenn es noch keine gibt: Tabelle mit einer einzigen Zeile `*(noch keine)*`
belassen.

**Effektivwert-Telemetrie (Ghost-Needle).** Der Nadelwert ist Regler ×
Mod-Port: was der Prozessor pro Sample an einem über `*_mod_in` verdrahteten
Parameter rechnet (`*_eff`, danach verworfen), sammelt er zusätzlich in das
`NeedleWindowTelemetry uiEffective<Control>`-Fenster (Min/Max über 256
Samples ≈ 5 ms, gefaltet in Atomics — siehe `src/EffectiveRangeTelemetry.h`;
Display-Only) und trägt genau dafür eine Zeile in dieser Tabelle ein.  Ein
einzelner Overwrite je Sample aliasierte gegen den 30-Hz-Poll der UI — die
Nadel sprang für jeden Mover auf beliebige Punkte im Ring, schon bei einem
reinen Sinus.  Velocity bleibt bewusst Sound-Only und bewegt die Nadel nie
(ein Akzent ist Artikulation, kein Patchzug — ein flirrender Nadelstrich je
Hit wäre Rauschen); ein Mover ohne eigenen Mod-Port (z. B. Resonator-Decay,
rein velocity-getrieben) bekommt deshalb keine Nadel.  Die UI zeichnet ihn
als `ModIndicatorOverlay`: eine Nadel, die dem Band mit Pendel-Ballistik
folgt (langsamer Mover — z. B. LFO-Sinus — pendelt zwischen den echten
Extremen; Audio-Rate verschwimmt zur Bandmitte) plus Band-Bogen
(Attack-instant, Release ~0,5 s: Modulationstiefe/Polyphonie-Ausbreitung);
Daumen/Attachment bleibt unberührt; gepolt über den Weg
`SynthVoice::getEffectiveControlRange` →
`AudioPluginAudioProcessor::getEffectiveControlRange` → Provider-Lambda
(Präzedenzfall: der Master-Meter `peakProvider`).  Reichweiten-Protokoll:
`(v, v)` ist ein echter Einzelwert — `juce::Range::isEmpty()` zählt Start ==
End und darf deshalb nie als „keine Daten"-Signal dienen; „keine Daten" ist
ausschließlich eine Range mit NaN-Länge (`smolfm::noEffectiveData()`,
Prädikat `smolfm::effectiveRangeHasData()` in `src/EffectiveRangeTelemetry.h`;
dieses JUCE-Submodule klammer invertierte Paare im Konstruktor per jmax,
negative Längen sind deshalb konstruktionsunmöglich).

---

### Abschnitt 3 — Mathematische Beschreibung

Beginnt einleitend mit der Symboltabelle, dann der formale Ablauf.

Symboltabelle:

| Symbol | Bedeutung | Einheit |
|---|---|---|
| `<sym>` | `<bedeutung>` | `<einheit>` |

Funktionsablauf: formale, JUCE-analoge DSP-Beschreibung des Prozessors pro
Sample bzw. pro Note (Gleichungen in KaTeX).

---

### Abschnitt 4 — Symbol ↔ Code

Tabelle, die jedes formale Symbol aus Abschnitt 3 seiner Code-Stelle und dem
Berechnungsschritt zuordnet.

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| `<sym>` | `<symbol>` | `<datei>` | `<was hier passiert, ggf. C++-Besonderheit>` |

---
