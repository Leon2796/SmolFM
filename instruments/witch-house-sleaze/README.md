# SmolFM Soundpack — "witch-house-sleaze"

Dunkles 25er-Soundpack: **10 Percussion** (Kicks, Hats, Snares, Perc, FX),
**5 Bässe**, **5 Pads**, **5 Leads** im Sprachbild Crystal Castles /
Witch House / Electro Punk / Indie Sleaze. Alle Patches sind Kompositionen
der Techniken aus `doc/techniques/` — darunter die drei aktuellen Katalog-
Neuheiten (`keyed-dual-resonator-drum`, `band-envelope-choreography`,
`onset-stagger-stack`) — und validieren gegen die Grammatik
(`doc/formats/smolfm.xsd`) und die Budgets aus `src/graph/GraphNodes.h`.
Stereo-Breite entsteht konsequent über Split/Merge (kanalgetreu); Percussion
trägt den `note`-Node unverbunden (Trigger), Spielbare drums tracken die Taste.

## Percussion (10)
| Instrument | Klangbild | Techniken |
|---|---|---|
| `ritual-kick.smolfm` | Ritual-Kick: soft-geclippte Dive-Sine + Stick-Noise | sine-dive-kick, serial-drive-filter-sculpt, sub-instrument-layering |
| `catacomb-kick.smolfm` | Größenraum-Kick: harter Long-Dive + Delay-Doppelfunk | sine-dive-kick, burst-delay-resonance, sub-instrument-layering |
| `hex-hat.smolfm` | Closed Hat: inharmonisches Square-Square-Ring, HP-Sizzle | metal-hat-squares |
| `veil-open-hat.smolfm` | Open Hat mit 220-ms-Schimmer-Echo | metal-hat-squares |
| `noir-snare.smolfm` | Noir-Snare: gestaffelte Onsets (1/8/19 ms), vel-akzentuiert | onset-stagger-stack, tuned-noise-percussion, sub-instrument-layering |
| `riot-clap.smolfm` | Riot-Clap: BP-Noise durch zwei serielle Mikro-Delays | multi-burst-clap, tuned-noise-percussion |
| `bone-tom.smolfm` | Keyed Tom/Talking-Drum: Dual-Sine-Bodies 1.7838x + Carpet | keyed-dual-resonator-drum, tuned-noise-percussion |
| `choreo-perc.smolfm` | Spektral-Choreografie-Hit: 4 Bänder, Decay-Leiter | band-envelope-choreography |
| `glitch-gate.smolfm` | Glitch-Perk: BP-Noise mit AM-Tremolo + Gain-Gate-Chops | tuned-noise-percussion, am-tremolo-lfo, gain-gate-chops |
| `tomb-riser.smolfm` | Tomb-Riser: key-getrackte F-ADSR-Cutoff-Sweep + Fold-Saw | fadsr-cutoff-sweep, fold-wave-oscillator, sub-instrument-layering |

## Bässe (5)
| Instrument | Klangbild | Techniken |
|---|---|---|
| `occult-howl-bass.smolfm` | Heulender FM-Growl (71 Hz Saw auf Key), Drive-Envelope | classic-two-op-fm, drive-envelope-growl, sub-instrument-layering |
| `wraith-wobble-bass.smolfm` | Key-centred LFO-Wobble-LP auf Square + Sub | lfo-cutoff-wobble, sub-instrument-layering |
| `sleaze-organ-bass.smolfm` | Sleaze-Bass-Organ: Sub/Root/2.0035x-Octave, Add-Bus | fscale-organ-stack, static-detune-beating, serial-drive-filter-sculpt |
| `saturn-growl-bass.smolfm` | Ring-Growl: Saw x 2.37x-Square mit 320-Hz-BP-Pickup | osc-ring-mod-cross, sub-instrument-layering |
| `dead-sub-anchor.smolfm` | Dead-clean Sub-Anker mit temperiertem Drittel-Partial | sub-instrument-layering, fscale-temper-beating |

## Pads (5)
| Instrument | Klangbild | Techniken |
|---|---|---|
| `ice-veil-pad.smolfm` | Eis-Schleier: Microdetune-Saws, Notch-Hollow, Haas 19 ms | fscale-detune-unison, serial-drive-filter-sculpt, am-tremolo-lfo, stereo-haas-branch-delay |
| `ghost-choir-pad.smolfm` | Ghost-Chor: temperierte Dreiecks-Partials, Vokal-BP | fscale-organ-stack, subtractive-voice-chain, am-tremolo-lfo, fscale-temper-beating |
| `vhs-graveyard-pad.smolfm` | VHS-Friedhof: Perlin-Cutoff-Drift, Twin-Delay-Space | fscale-detune-unison, perlin-cutoff-drift, serial-drive-filter-sculpt, double-delay-space |
| `lilith-glass-pad.smolfm` | Glas-FM-Pad: zwei 2-OP-Voices (2.76/2.769), Haas 260 ms | classic-two-op-fm, fscale-detune-unison, stereo-haas-branch-delay |
| `ritual-drone.smolfm` | Ritual-Drohne: Quint-/Sub-Saw-Stack, Perlin-Gain-Swell | fscale-interval-power-stack, gain-drift-swell, fscale-detune-unison |

## Leads (5)
| Instrument | Klangbild | Techniken |
|---|---|---|
| `hex-bell-lead.smolfm` | Hex-Bell: 3.5545/1.1886 FM-Kaskade + Slapback | chained-fm-cascade, delay-slapback-doubler |
| `teeth-saw-lead.smolfm` | Teeth-Saw: korreliertes Vibrato über Microdetune-Saws | lfo-vibrato-fm, fscale-detune-unison, serial-drive-filter-sculpt |
| `cult-punk-lead.smolfm` | Cult-Punk: HP+LP-Square, Snappy-Gate, Eighths-Echo | subtractive-voice-chain, tempo-synced-delay-tail |
| `siren-pale-lead.smolfm` | Pale Siren: Perlin-Drift-Sine + 285-ms-Rechtszweig | perlin-drift-modulation, stereo-haas-branch-delay |
| `gnawl-fold-lead.smolfm` | Gnawl: Fold-Shaper mit Drive-Envelope + 2.003x-Octave | drive-envelope-growl, fold-wave-oscillator, fscale-interval-power-stack, sub-instrument-layering |

Die `techniques=`-Attribute listen die exakt verwendeten Katalog-Namen;
`description` hält pro Patch den Klang-Zielsatz. Validierbar mit
`.cline/skills/create-smol-fm-instrument/validate-pack.ps1`.