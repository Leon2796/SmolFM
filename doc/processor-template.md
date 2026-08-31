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

Tabelle 1a: Instanzen & Ports.

| Eigenschaft | Wert | Symbol / Typ | Datei |
|---|---|---|---|
| Max. Instanzen | `<n>` | `GraphNodeRegistry::max…` | `src/graph/GraphNodes.h` |
| Input-Ports | `<id (PortType), …>` | `InputPort <symbol>` | `<prozessor.h>` |
| Output-Ports | `<id (PortType)>` | `OutputPort <symbol>` | `<prozessor.h>` |

Tabelle 1b: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Symbol im Prozessor | Datei |
|---|---|---|---|---|
| `<Anzeigename>` | `<id>` | `<Float/Choice/Bool, Bereich>` | `std::atomic<float>* <symbol>` | `<prozessor.h>` |

---

### Abschnitt 2 — UI-Konfiguration

Tabelle der zusätzlichen UI-Funktionen/Modi des Prozessors in der Oberfläche
(Styling-Schalter, Modus-Checkboxen, Bereichs-Umschaltungen o. ä.).

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| `*(noch keine)*` | | | |

Wenn es noch keine gibt: Tabelle mit einer einzigen Zeile `*(noch keine)*`
belassen.

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
