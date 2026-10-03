# Standalone-Startparameter `--instrument`

Die Standalone-App (SmolFM.exe) akzeptiert einen optionalen Parameter, um beim
Start direkt ein Instrument zu laden:

```
SmolFM.exe --instrument <Pfad\zu\instrument.smolfm>
SmolFM.exe --instrument=<Pfad\zu\instrument.smolfm>
SmolFM.exe -i <Pfad\zu\instrument.smolfm>
SmolFM.exe <Pfad\zu\instrument.smolfm>      (einzelnes Positional-Argument)
```

## Verhalten

- **Relativer Pfad**: aufgelöst relativ zum Arbeitsverzeichnis; ein absoluter
  Pfad bleibt unverändert (`File::getChildFile` reicht absolute Pfade durch).
- Genau wie eine Patchbrowser-Auswahl: das Verzeichnis der Datei wird als
  Patch-Verzeichnis gesetzt, die Datei geladen, der Instrumentenname
  angezeigt und der Eintrag im Browser markiert.
- **Ohne Parameter** verhält sich der Start unverändert (erster Patch des
  gespeicherten Verzeichnisses).
- Datei fehlt oder keine `.smolfm`-Datei: Warnbox, Start läuft normal weiter.
- Gilt **nur für die Standalone-Exe**; im Plugin (VST3) wird die
  Kommandozeile des Hosts ignoriert (Runtime-Guards:
  `JUCEApplicationBase::getInstance()` + `wrapperType == wrapperType_Standalone`).
- Erster erfasster Parameter gewinnt (`--instrument=` vor `--instrument`
  vor Positional-Argument).

## Build & Start

```
jumake build
jumake_build\src\SmolFM_artefacts\Release\Standalone\SmolFM.exe --instrument instruments\debug\resonator-demo.smolfm
```

Implementiert in `src/PluginEditor.cpp`
(`AudioPluginAudioProcessorEditor::applyStartupInstrumentParameter`), aufgerufen
am Ende des Editor-Konstruktors.