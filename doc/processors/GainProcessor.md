# GainProcessor
> Template: [../processor-template.md](../processor-template.md) — Struktur nicht ändern.

Konstante Verstärkung/Dämpfung: multipliziert das eingehende Signal mit dem
Gain-Faktor und leitet das Ergebnis weiter. Stateless — kein Envelope, kein
per-Note-State. `gain = 1` ist transparent, ein unverbundener Node liefert
Stillstand (Default-Eingang 0.0).

## Abschnitt 1 — Echte Prozessor-Parameter (Sends/Inputs)

Tabelle 1a: Instanzen.

| Eigenschaft | Wert | Symbol / Typ | Datei | Kurzbeschreibung |
|---|---|---|---|---|
| Max. Instanzen | 16 | `GraphNodeRegistry::maxGains` | [src/graph/GraphNodes.h](../../src/graph/GraphNodes.h) | Bis zu 16 Dreh-Verstärker pro Voice — je Layer/Kopie ein eigener Lautheits-Hahn (Budgets sind verdoppelt; der Code registriert sie per `for (i < GraphNodeRegistry::maxGains)`). |

Tabelle 1b: Ports — jeder Port in eigener Zeile (ein Port ist ein eigener Parameter).

| Port | Richtung | Port-Typ | Unverbunden (Default) | Kurzbeschreibung (Wirkung & Modulation) | Symbol im Code | Datei |
|---|---|---|---|---|---|---|
| `in` | Input | `signal` | liest 0 → Stillstand (der Hahn dreht auf Nichts) | **Rohsignal.** Das Signal, das mit dem wirksamen Faktor multipliziert wird — der Gain verändert nur die Lautheit, nie die Gestalt. | `InputPort input` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |
| `factor_mod_in` | Input | `signal` | liest 0 → vollständig transparent (neutral) | **Fremdfahrer des Faktors.** Multipliziert den Regler-Gain ($\cdot\,(1+m)$, auf 0–10 geclampt) — macht aus dem Gain ein steuerbares VCA: LFO = sanftes Tremolo/Swell, Square = rhythmisches Gate, ADSR = eigene Volume-Envelope pro Layer. | `InputPort modInput` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |
| `vel` | Input | `velocity` | liest 1.0 → neutral, klanginvariant | **Lautheits-Hälfte des Anschlags.** Anschlagstärke multipliziert den wirksamen Gain (Accent): hard = lauter, zart = leiser — und zwar nur in verdrahteten Gains (opt-in pro Layer), unverdrahtete Layers ignorieren den Anschlag. | `InputPort velIn` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |
| `out` | Output | `signal` | — | **Skaliertes Signal:** der Eingang mal wirksamer Faktor — lauter, leiser oder komplett stumm (Faktor 0 = Layer-Mute). | `OutputPort output` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

Tabelle 1c: Daraus folgende echte Parameter (APVTS).

| Parameter | APVTS-ID | Typ / Bereich | Kurzbeschreibung (Wirkung) | Symbol im Prozessor | Datei |
|---|---|---|---|---|---|
| Gain | `gain<N>Factor` | Float, 0–10, Default 1 (UI-Bipolar: Mitte ≙ 1.0/Unity — linke Hälfte linear 0–1, rechte Hälfte quadratisch 1–10; Rohwert bleibt 0–10) | Der Lautheits-Faktor des Layers: 1 = neutraler Durchlass, 0 = Mute, über 1 = Boost (Vorsicht vor Clipping am gestapelten Master-Bus). | `std::atomic<float>* gain` | [src/processors/GainProcessor.h](../../src/processors/GainProcessor.h) |

`<N>` = Instanzindex 0–15 (Budgets seit Commit ecfdef2 verdoppelt; der Code
registriert die Parameter über `for (i < GraphNodeRegistry::maxGains)`).

## Abschnitt 2 — UI-Konfiguration

| UI-Funktion | Bedeutung | UI-Symbol | Datei |
|---|---|---|---|
| *(keine)* | ein Rotary-Slider (Gain-Faktor, Suffix „x") | `GainComponent::gainSlider` | [src/gui/components/GainComponent.h](../../src/gui/components/GainComponent.h) |

## Abschnitt 3 — Mathematische Beschreibung

| Symbol | Bedeutung | Einheit |
|---|---|---|
| $x_n$ | Eingangssample | Amplitude |
| $G$ | Gain-Faktor | linear |
| $G_{\text{eff}}$ | wirksamer Gain-Faktor | linear |
| $y_n$ | Ausgangssample | Amplitude |

Pro Sample:

$$y_n = G_{\text{eff}} \cdot x_n$$

Spezialfälle:
- $G = 1$: transparent.
- $G = 0$: Ausgang stumm (Layer-Mute).
- $G < 0$ (API-only): Phasen-Inversion.
- $G > 1$: Boost; Vorsicht vor Clipping am Master-Output (8 gestackte Layer).

### Abschnitt 3b — Modulations-Input `factor_mod_in`

Optionaler Mod-Eingang für den **Gain-Faktor** (PortType `signal`). Der
Stereo-Mittelwert wird auf $[-1,+1]$ geclampt und multipliziert den
statischen Faktor: $G_{\text{eff}} = \mathrm{clamp}(G\cdot(1+m), 0, 10)$.
Unverbunden ($m = 0$) ist der Port vollständig transparent. Damit wird das
Gain zum modulierbaren VCA: LFO → lineares Tremolo/Swell (ohne die AM-
Ring-Semantik), Square-LFO → rhythmisches Gate, statischer Osc → ADSR →
eigene Volume-Envelope pro Layer.
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`gain`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `factor_mod_in`.

### Abschnitt 3c — Velocity-Input `vel`

Optionaler Velocity-Eingang (PortType `velocity`, 0–1, Anschlagstärke der
aktuellen Note). Der Stereo-Mittelwert multipliziert den wirksamen Gain:
$G_{\text{eff}} = \mathrm{clamp}(G_{\text{eff}} \cdot v, 0, 10)$.
Unverbunden (Default 1.0) ist der Port vollständig sound-invariant. Rolle
im 808-Schema: Accent — die Lautheits-Hälfte des TR-808-Accents, pro Layer
opt-in (nur verdrahtete Gains antworten auf die Anschlagstärke).
Port-ID-Sync: `GraphNodes`-Spec, XSD-Annotation (`gain`-complexType) und
`SynthVoice::resolveInput` tragen denselben Namen `vel`.

## Abschnitt 4 — Symbol ↔ Code

| Formales Symbol | C++-Symbol / Aufruf | Datei | Berechnungsschritt |
|---|---|---|---|
| $G$ | `gain->load()` → `gBase` → `g` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Basis gelesen; `factor_mod_in` verbunden: $G_{\text{eff}} = \mathrm{clamp}(G\cdot(1+m), 0, 10)$, $m$ = Stereo-Mittel geclampt $[-1,+1]$ |
| $x_n$ | `input.getSample()` → `sourceSample` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Port lesen (Default 0.0f unverbunden) |
| $y_n$ | `sourceSample * g` → `output.setSample()` | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | Multiplikation, Ergebnis in den Port |
| $v$ | `velIn.getSample()` → Stereo-Mittel | [GainProcessor.cpp](../../src/processors/GainProcessor.cpp) | `vel` verbunden: $G_{\text{eff}} = \mathrm{clamp}(G_{\text{eff}} \cdot v, 0, 10)$; unverbunden: Default 1.0 (neutral) |