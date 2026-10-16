# Patch-Browser: Work-Directory und Patch-Navigation

Der Patch-Browser erlaubt es, ein Arbeitsverzeichnis auszuwählen und zwischen
den darin enthaltenen `.smolfm`-Dateien per Pfeiltasten zu wechseln. Beim
Wechsel wird der Graph direkt in den Canvas geladen.

## UI-Aufbau

Der Browser ist eine schlanke Navigationszeile zwischen Toolbar und
Graph-Canvas ([PluginEditor.cpp](../src/PluginEditor.cpp)); die
Verzeichnis-Auswahl (Folder-Button + Pfad-Label) lebt in der Toolbar darüber.
Die Node-Palette rechts ist ein-/ausklappbar (Palette-Toggle am rechten Ende
der Navigationszeile) und teilt mit dem Canvas Ober- und Unterkante:

```
┌──────────────────────────────────────────────────────────────────┐
│  Toolbar (Titel, Export, Import, Directory, Pfad)                │
├──────────────────────────────────────────────────────────────────┤
│  [<] Instrumentenname [>]                        [Toggle]       │
├───────────────────────────────────────────────┬──────────────────┤
│  DraggablePanel (Graph-Canvas)                │  Node-palette    │
│                                               │  (kol- lapse-    │
│                                               │  bar)            │
└───────────────────────────────────────────────┴──────────────────┘
```

| Element | Bedeutung | Symbol |
|---|---|---|
| Pfad-Label | Zeigt das gewählte Verzeichnis, rechts neben den Toolbar-Buttons | `AudioPluginAudioProcessorEditor::pathLabel` |
| Directory-Button | Öffnet einen Verzeichnis-Chooser | `AudioPluginAudioProcessorEditor::directoryButton` |
| Palette-Toggle | Blendet die rechte Node-Palette ein/aus (kollabierbar), am rechten Ende der Navigationszeile | `AudioPluginAudioProcessorEditor::paletteToggleButton` |
| Pfeile `<` / `>` | Flankieren das Namens-Label am linken Zeilenende (Anordnung `< Name >`); blättern zur vorherigen/nächsten `.smolfm`-Datei (zyklisch) | `PatchBrowser::prevButton`, `nextButton` |
| Name-Label | Sitzt zwischen den Pfeilen, in seinem festen Slot zentriert; fett und in der Akzentfarbe gesetzt | `PatchBrowser::nameLabel` |

Das Name-Label sitzt zwischen den Pfeilen (Anordnung `< Name >`) — in einem
festen Slot zentriert, damit die Pfeile beim Blättern nicht wandern — und ist
fett in der Akzentfarbe (lightblue) gesetzt; es ist das dominierende Element
der Zeile.

## Datenfluss

```
Directory-Button → patchBrowser.onDirectoryChosen(dir) → Editor::setWorkingDirectory(dir)
                 → processorRef.setPatchDirectory(dir)
                 → patchBrowser.setDirectory(dir) → rescan()
                 → erster Patch des neuen Kontexts laden

Pfeil < / >      → step(±1) → onPatchSelected(file)
                            → SmolFmFile::load(panel, apvts, file)
                            → patchBrowser.setInstrumentName(readInstrumentName(file))
```

- **Verzeichnis-Persistenz**: Das Work-Directory lebt als `patchDirectory`
  im Processor ([PluginProcessor.h](../src/PluginProcessor.h)) und wird als
  Attribut `patchDirectory` am APVTS-State-ValueTree gespeichert
  (`getStateInformation`/`setStateInformation`). Host und Standalone
  stellen es pro Session/Preset wieder her.
- **Scan**: `PatchBrowser::rescan()` sammelt mit
  `juce::RangedDirectoryIterator` alle `*.smolfm`-Dateien des Verzeichnisses.
- **Navigation**: `step()` wickelt zyklisch um (letzte → erste Datei und
  umgekehrt), sodass Endlos-Blättern in beide Richtungen möglich ist.

## Speicherstrategie

Es wird **nie mehr als eine Datei** vollständig im Hauptspeicher gehalten:

- Die Dateiliste enthält nur `juce::File`-Handles (Pfade), keine Inhalte.
- Erst beim Wechsel parst `SmolFmFile::load` genau die ausgewählte Datei.
- Der Instrumentenname wird mit `SmolFmFile::readInstrumentName` gelesen —
  dabei wird nur das XML-Root-Element ausgewertet; ohne `name`-Attribut gilt
  der Dateiname als Fallback.
- Beim Export schreibt `SmolFmFile::save` das `name`-Attribut automatisch
  aus dem Dateinamen; `writeInstrumentName` erlaubt nachträgliche Änderung
  ohne das Dokument sonst anzufassen.

## Dateiformat

Das `.smolfm`-Root-Element trägt jetzt ein optionales `name`-Attribut:

```xml
<SmolFM version="2" name="FM Bell">
  ...
</SmolFM>
```

Bestehende Dateien ohne `name` bleiben kompatibel — beim nächsten Export
oder beim Laden zeigt der Browser dann den Dateinamen.
