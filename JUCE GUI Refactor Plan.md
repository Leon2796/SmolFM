# JUCE GUI Refactor Plan

Refactor the plugin editor into a component-based UI hierarchy using JUCE `Grid` for the main layout and dedicated child components for each logical section.

## Goals

- Replace the current `removeFromTop()` / `removeFromLeft()` layout logic with a clear component hierarchy.
- Use `juce::Grid` for the main editor layout.
- Keep each logical UI section self-contained in its own `juce::Component`.
- Make the layout structure readable from the component tree.

## Directory Structure

Create:

```text
source/gui/components/
```

Place each GUI component in this directory, with separate `.h` / `.cpp` files where appropriate.

Suggested structure:

```text
source/
└── gui/
    └── components/
        ├── OscillatorPanel.h
        ├── OscillatorPanel.cpp
        ├── FMModulationComponent.h
        ├── FMModulationComponent.cpp
        ├── AdsrPanel.h
        └── AdsrPanel.cpp
```

## Components

### `OscillatorPanel`

Create one reusable `OscillatorPanel` class for both the Carrier and Modulator.

- Do **not** create separate Carrier/Modulator classes.
- Both are instances of the same component.
- The panel should contain its own:
  - title/label
  - frequency/ratio control
  - waveform selection controls(oriented in a vertical way)
- The oscillator's frequency unit must be displayed as **Hz**.
- The component should expose whatever parameters/state the editor needs to configure it as Carrier or Modulator.
- Use `juce::Grid` or `juce::FlexBox` for the internal layout rather than manual rectangle slicing where practical.

### `FMModulationComponent`

Create a dedicated component for the FM Amount control.

- Own its label and slider.
- Handle its own internal layout.
- Keep FM-specific UI logic out of the main editor.

### `AdsrPanel`

Create a dedicated component for the ADSR controls.

- Own the Attack, Decay, Sustain and Release controls.
- Handle its own labels/layout.
- Use a structured layout (`Grid` or `FlexBox`) rather than chained `removeFromLeft()` calls.

## Main Editor Layout

The main `AudioPluginAudioProcessorEditor` should only be responsible for composing the high-level UI.

Use a `juce::Grid` to arrange the main controls as follows:

```text
┌─────────────────────────────────────────────┐
│                    Title                    │
├─────────────────────────────────────────────┤
│  Carrier Osc.  │  FM Amount  │  Modulator  │
│               │   [Slider]   │              │
├─────────────────────────────────────────────┤
│                     ADSR                    │
└─────────────────────────────────────────────┘
```

The Carrier, FM Amount, and Modulator must occupy **one horizontal row**, in this order:

```text
Carrier → FM Amount → Modulator
```

The `FMModulationComponent` is therefore positioned **between** the two `OscillatorPanel` instances.

The editor's `resized()` should primarily define this high-level grid and delegate internal layout to the child components.

Avoid manually calculating child rectangles with repeated `removeFromTop()`, `removeFromLeft()`, etc.

## Refactoring Requirements

- Preserve existing functionality and parameter bindings.
- Preserve the existing visual intent and controls.
- Do not duplicate Carrier and Modulator layout code.
- Keep component-specific layout inside the respective component.
- Keep the main editor focused on composition rather than detailed geometry.
- Prefer clear semantic component names over generic containers.
- Ensure the project builds cleanly after the refactor.