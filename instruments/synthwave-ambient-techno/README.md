# SmolFM Soundpack — "synthwave-ambient-techno"

Hochwertiges Soundpack mit **30 Instrumenten** für den SmolFM-Katalog:
Synthwave-Ästhetik gemischt mit Ambient-Flächen und dunkler Techno-Maschine.
Alle Patches sind Kompositionen der Techniken aus `doc/techniques/` und gegen
die Grammatik (`doc/formats/smolfm.xsd`, Budgets aus `src/graph/GraphNodes.h`)
validiert. Stereo-Breite entsteht konsequent über Split/Merge (kanalgetreu),
nicht über parallele Output-Inputs (der Output summiert kanalweise — Mono-
Ketten landen zentriert).

## Synthwave ( melodisch / retro )
| Instrument | Klangbild | Techniken |
|---|---|---|
| `neon-skyline-pad.smolfm` | Breites Neon-Pad, zwei 2-OP-FM-Voices hard L/R, langsamer Sustain | classic-two-op-fm, stereo-micropitch-spread |
| `sunset-drive-lead.smolfm` | Detun. Doppelsäge mit Tast-Spillover-Vibrato + Soft-Clip | lfo-vibrato-fm, serial-drive-filter-sculpt |
| `outrun-brass-stack.smolfm` | Outrun-Brass: geclippte Detune-Saws + Oktav-Layer | fscale-interval-power-stack |
| `cassette-epiano.smolfm` | DX-Electric-Piano mit Index-Envelope (Tine-Bounce) | index-envelope-fm, classic-two-op-fm |
| `grid-runner-bass.smolfm` | Retro-Bass: Pitch-Drop-Saw, resonantes LP + Sub-Layer | fadsr-pitch-drop, subtractive-voice-chain |
| `neon-rain-arp.smolfm` | Glas-Arpeggio in Dreieckswelle, tempo-synced Eighths-Echo | classic-two-op-fm, tempo-synced-delay-tail |
| `arcade-bell-keys.smolfm` | Arcade-Bell-Keys (3.5545/1.1886 FM-Kaskade) + Slapback | chained-fm-cascade, delay-slapback-doubler |
| `dusk-laser-zap.smolfm` | Laser-Zap: Pitch-Dive + Self-Ring-Oktavierung | fadsr-pitch-drop, self-ring-octave-double |
| `vhs-cloud-pad.smolfm` | Drift-Pad: Perlin auf FM-Index + Haas-Stereo (38 ms) | perlin-drift-modulation, stereo-haas-branch-delay |

## Drum-Machine ( Percussion, spielbar / getriggert )
| Instrument | Klangbild | Techniken |
|---|---|---|
| `chrome-kick.smolfm` | Drum-Machine-Kick mit Stick-Noise-Layer, soft-geclippt | sine-dive-kick |
| `concrete-kick.smolfm` | Härterer Techno-Kick (hard-clip, tiefer Down-Faktor) | sine-dive-kick, serial-drive-filter-sculpt |
| `silver-hat.smolfm` | Closed Hat ohne Noise: inharmonisches Square-Square-Ring | metal-hat-squares |
| `neon-hex-clap.smolfm` | Hand-Clap: BP-Noise durch zwei serielle Mikro-Delays | multi-burst-clap |

## Ambient ( Flächen, Drones, Texturen )
| Instrument | Klangbild | Techniken |
|---|---|---|
| `aurora-glass-pad.smolfm` | Glas-Pad, 3x-Modulator-Paar +0.4/−0.4 ‰, sehr lang | classic-two-op-fm, stereo-micropitch-spread |
| `glacier-shimmer-bell.smolfm` | Eiskaskaden-Bell mit 340-ms-Echo-Schweif | chained-fm-cascade, burst-delay-resonance |
| `nebula-strings.smolfm` | String-Ensemble, Panned-Detune + langsames Tremolo | am-tremolo-lfo, fscale-detune-unison |
| `misty-tide-pad.smolfm` | Tide-Pad, Perlin-Cutoff-Drift (key-getrackt) | perlin-cutoff-drift |
| `meditation-bowls.smolfm` | Singing Bowls: 3.902er temperierter FM-Partial + Sub-Schale | fscale-temper-beating |
| `deep-drift-drone.smolfm` | Perpetuum-Drohne, Simplex-Drift auf dem Gain | gain-drift-swell |
| `fog-horn-sub-drone.smolfm` | Nebelhorn: Oktav- + Quint-Sub, parallel gestaged | fscale-organ-stack, sub-instrument-layering |
| `quiet-rain-wash.smolfm` | Rain-Wash: Noise-BP wandert perlin-driftig, Haas-Breite | perlin-cutoff-drift, tuned-noise-percussion |
| `void-mallet-plink.smolfm` | Space-Mallet: F-ADSR-Bandpass-Pluck + 240-ms-Tap | fadsr-cutoff-sweep |
| `ethereal-choir-pad.smolfm` | Chor-Pad: Joint-Highpass-Formant + LP + Chor-Tremolo | subtractive-voice-chain, am-tremolo-lfo |

## Techno / Dark
| Instrument | Klangbild | Techniken |
|---|---|---|
| `acid-line-303.smolfm` | 303-Säure: F-ADSR-Sweep am Cutoff, Hard-Clip, Squirm-Vibrato | fadsr-cutoff-sweep |
| `tunnel-wobble-bass.smolfm` | Dub-Wobble-Bass: LFO-Cutoff-Wobble + Sub | lfo-cutoff-wobble |
| `hoover-rave-stab.smolfm` | Hoover: Hard-Panned-Detune-Saws + Quint-FM-Growl | classic-two-op-fm, fscale-interval-power-stack |
| `pulse-chord-stab.smolfm` | Moll-Square-Stab, drei Ketten á synced Eighth-Delay | fscale-interval-power-stack, tempo-synced-delay-tail |
| `dark-gong-anvil.smolfm` | Industrie-Gong (inharmonisches 2.37-FM) mit Mega-Decay | fscale-interval-modulator-fm |
| `warehouse-riser.smolfm` | 4-s-Riser: Noise+Saw teilen einen F-ADSR-Sweep, Fold-Shape | fadsr-cutoff-sweep, fold-wave-oscillator |
| `rave-airhorn-lead.smolfm` | Airhorn: Vibrato-Square-Pair, Hard-Clip, Oktav-Layer | fscale-interval-power-stack, lfo-vibrato-fm |

Die `techniques=`-Attribute der Dateien listen die exakt verwendeten
Katalog-Namen; `description` hält pro Patch den Klang-Zielsatz. Percussion-
Instrumente (Chrome Kick, Concrete Kick, Silver Hat, Neon Hex Clap) nutzen
den `note`-Node als Trigger bzw. als Body-Register (Kick), die Noten-losen
(Hat/Clap) tragen ihn unverbunden, wie im Techniquen-Katalog vorgesehen.