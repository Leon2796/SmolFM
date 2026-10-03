//! TR-808 Rhythm Composer — VST3 / CLAP drum machine plugin.
//!
//! Proof-of-concept: the Terminal Beats 808 synthesis engine packaged as a
//! standalone audio plugin with a skeuomorphic GUI.

use nih_plug::prelude::*;
use std::sync::atomic::{AtomicBool, AtomicI32, AtomicU16, AtomicUsize, Ordering};
use std::sync::Arc;

mod dsp;
mod editor;

use dsp::{ActiveHit, DrumVoice, NUM_VOICES};

// ── Atomic f32 helper (pack into AtomicU32) ─────────────────────────

pub struct AtomicF32(std::sync::atomic::AtomicU32);

impl AtomicF32 {
    pub const fn new(v: f32) -> Self {
        Self(std::sync::atomic::AtomicU32::new(v.to_bits()))
    }

    pub fn load(&self, order: Ordering) -> f32 {
        f32::from_bits(self.0.load(order))
    }

    pub fn store(&self, v: f32, order: Ordering) {
        self.0.store(v.to_bits(), order);
    }
}

// ── Shared state (lock-free, audio ↔ GUI) ───────────────────────────

pub struct SharedState {
    /// 16-step pattern per voice (bit N = step N active).
    pub patterns: [AtomicU16; NUM_VOICES],
    /// 16-step accent pattern (shared across all voices).
    pub accents: AtomicU16,
    /// Currently selected voice in the GUI (index into DrumVoice::ALL).
    pub selected_voice: AtomicUsize,
    /// Current playing step (-1 = stopped).
    pub current_step: AtomicI32,
    /// Per-voice level (0.0–1.0).
    pub voice_levels: [AtomicF32; NUM_VOICES],
    /// Per-voice tune offset (0.0–1.0, center = 0.5).
    pub voice_tunes: [AtomicF32; NUM_VOICES],
    /// Per-voice decay scale (0.0–1.0).
    pub voice_decays: [AtomicF32; NUM_VOICES],
    /// Master level (0.0–1.0).
    pub master_level: AtomicF32,
    /// Accent level (0.0–1.0, how much louder accented hits are).
    pub accent_level: AtomicF32,
    /// BPM for display.
    pub tempo_bpm: AtomicF32,
    /// Internal play state (for when host isn't providing transport).
    pub playing: AtomicBool,
}

impl SharedState {
    fn new() -> Self {
        const ZERO_U16: AtomicU16 = AtomicU16::new(0);
        Self {
            patterns: [ZERO_U16; NUM_VOICES],
            accents: AtomicU16::new(0),
            selected_voice: AtomicUsize::new(0),
            current_step: AtomicI32::new(-1),
            voice_levels: std::array::from_fn(|_| AtomicF32::new(0.8)),
            voice_tunes: std::array::from_fn(|_| AtomicF32::new(0.5)),
            voice_decays: std::array::from_fn(|_| AtomicF32::new(0.5)),
            master_level: AtomicF32::new(0.75),
            accent_level: AtomicF32::new(0.7),
            tempo_bpm: AtomicF32::new(120.0),
            playing: AtomicBool::new(false),
        }
    }

    fn load_default_pattern(&self) {
        //  Classic 808 four-on-the-floor starter pattern.
        //  BD: 1 . . . 5 . . . 9 . . . 13 . . .
        self.patterns[DrumVoice::Kick as usize].store(
            0b0001_0001_0001_0001,
            Ordering::Relaxed,
        );
        //  SD: . . . . 5 . . . . . . . 13 . . .
        self.patterns[DrumVoice::Snare as usize].store(
            0b0000_0001_0000_0001 << 4 >> 4 | 1 << 4 | 1 << 12,
            Ordering::Relaxed,
        );
        //  CH: every other step
        self.patterns[DrumVoice::ClosedHat as usize].store(
            0b0101_0101_0101_0101,
            Ordering::Relaxed,
        );
        //  OH: . . . . . . . . . . . . . . 15 .
        self.patterns[DrumVoice::OpenHat as usize].store(1 << 14, Ordering::Relaxed);
    }
}

// ── Plugin parameters (host-automatable) ────────────────────────────

#[derive(Params)]
struct EightOhEightParams {
    #[id = "master"]
    master_level: FloatParam,
}

impl Default for EightOhEightParams {
    fn default() -> Self {
        Self {
            master_level: FloatParam::new(
                "Master",
                0.75,
                FloatRange::Linear { min: 0.0, max: 1.0 },
            )
            .with_unit(" dB")
            .with_value_to_string(formatters::v2s_f32_percentage(0))
            .with_string_to_value(formatters::s2v_f32_percentage()),
        }
    }
}

// ── Plugin ──────────────────────────────────────────────────────────

struct EightOhEight {
    params: Arc<EightOhEightParams>,
    shared: Arc<SharedState>,
    egui_state: Arc<nih_plug_egui::EguiState>,

    // Audio-thread state (not shared)
    sample_rate: f32,
    active_hits: Vec<ActiveHit>,
    last_step: i32,
    /// Fractional sample position within the current bar, for sub-sample
    /// accurate step triggering when the host doesn't provide transport.
    internal_pos: f64,
    hit_seed: u64,
}

impl Default for EightOhEight {
    fn default() -> Self {
        let shared = Arc::new(SharedState::new());
        shared.load_default_pattern();
        Self {
            params: Arc::new(EightOhEightParams::default()),
            shared,
            egui_state: nih_plug_egui::EguiState::from_size(editor::WIDTH, editor::HEIGHT),
            sample_rate: 44100.0,
            active_hits: Vec::with_capacity(32),
            last_step: -1,
            internal_pos: 0.0,
            hit_seed: 0xDEAD_BEEF,
        }
    }
}

impl EightOhEight {
    fn next_seed(&mut self) -> u64 {
        self.hit_seed = self.hit_seed.wrapping_mul(6364136223846793005).wrapping_add(1);
        self.hit_seed
    }

    fn trigger_voice(&mut self, voice: DrumVoice, accent: bool) {
        // Choke group
        for &choked in voice.chokes() {
            for hit in &mut self.active_hits {
                if hit.voice == choked {
                    hit.start_fade_out();
                }
            }
        }

        let idx = voice as usize;
        let level = self.shared.voice_levels[idx].load(Ordering::Relaxed);
        let tune = self.shared.voice_tunes[idx].load(Ordering::Relaxed);
        let decay = self.shared.voice_decays[idx].load(Ordering::Relaxed);
        let decay_scale = 0.3 + decay * 1.7; // 0.3x – 2.0x

        let seed = self.next_seed();
        self.active_hits.push(ActiveHit::new(
            voice,
            accent,
            level,
            tune,
            decay_scale,
            self.sample_rate,
            seed,
        ));
    }

    fn trigger_step(&mut self, step: usize) {
        let accent_bits = self.shared.accents.load(Ordering::Relaxed);
        let step_accent = (accent_bits >> step) & 1 == 1;

        for (i, voice) in DrumVoice::ALL.iter().enumerate() {
            let pat = self.shared.patterns[i].load(Ordering::Relaxed);
            if (pat >> step) & 1 == 1 {
                self.trigger_voice(*voice, step_accent);
            }
        }
    }
}

impl Plugin for EightOhEight {
    const NAME: &'static str = "808 Rhythm Composer";
    const VENDOR: &'static str = "Terminal Beats";
    const URL: &'static str = "";
    const EMAIL: &'static str = "michaelalanwilson@gmail.com";
    const VERSION: &'static str = env!("CARGO_PKG_VERSION");

    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout {
        main_input_channels: None,
        main_output_channels: NonZeroU32::new(2),
        ..AudioIOLayout::const_default()
    }];

    const MIDI_INPUT: MidiConfig = MidiConfig::Basic;
    const SAMPLE_ACCURATE_AUTOMATION: bool = true;

    type SysExMessage = ();
    type BackgroundTask = ();

    fn params(&self) -> Arc<dyn Params> {
        self.params.clone()
    }

    fn editor(&mut self, _async_executor: AsyncExecutor<Self>) -> Option<Box<dyn Editor>> {
        editor::create(self.egui_state.clone(), self.shared.clone())
    }

    fn initialize(
        &mut self,
        _audio_io_layout: &AudioIOLayout,
        buffer_config: &BufferConfig,
        _context: &mut impl InitContext<Self>,
    ) -> bool {
        self.sample_rate = buffer_config.sample_rate;
        true
    }

    fn process(
        &mut self,
        buffer: &mut Buffer,
        _aux: &mut AuxiliaryBuffers,
        context: &mut impl ProcessContext<Self>,
    ) -> ProcessStatus {
        // Handle MIDI input
        while let Some(event) = context.next_event() {
            match event {
                NoteEvent::NoteOn { note, velocity, .. } => {
                    if let Some(voice) = dsp::midi_to_voice(note) {
                        self.trigger_voice(voice, velocity > 0.8);
                    }
                }
                _ => {}
            }
        }

        let transport = context.transport();
        let host_playing = transport.playing;
        let host_bpm = transport.tempo.unwrap_or(120.0) as f32;
        let internal_playing = self.shared.playing.load(Ordering::Relaxed);
        let playing = host_playing || internal_playing;

        self.shared.tempo_bpm.store(host_bpm, Ordering::Relaxed);

        let master = self.params.master_level.smoothed.next();
        let sr = self.sample_rate;
        let samples_per_16th = (sr as f64 * 60.0) / (host_bpm as f64 * 4.0);

        for (sample_idx, mut channel_samples) in buffer.iter_samples().enumerate() {
            // Advance sequencer
            if playing {
                let step = (self.internal_pos / samples_per_16th) as i32 % 16;
                if step != self.last_step {
                    self.last_step = step;
                    self.shared.current_step.store(step, Ordering::Relaxed);
                    self.trigger_step(step as usize);
                }
                self.internal_pos += 1.0;
                // Wrap at bar boundary to avoid f64 precision loss
                let bar_samples = samples_per_16th * 16.0;
                if self.internal_pos >= bar_samples {
                    self.internal_pos -= bar_samples;
                }
            } else {
                if self.last_step != -1 {
                    self.last_step = -1;
                    self.shared.current_step.store(-1, Ordering::Relaxed);
                    self.internal_pos = 0.0;
                }
            }

            // Render active hits
            let mut mix = 0.0f32;
            for hit in self.active_hits.iter_mut() {
                mix += hit.render_sample(sr);
            }

            // Clean up finished hits periodically
            if sample_idx % 64 == 0 {
                self.active_hits.retain(|h| !h.is_done());
            }

            // Apply master and soft clip
            mix *= master;
            mix = dsp::soft_clip(mix);

            // Write stereo output
            for sample in channel_samples.iter_mut() {
                *sample = mix;
            }
        }

        ProcessStatus::Normal
    }
}

impl ClapPlugin for EightOhEight {
    const CLAP_ID: &'static str = "com.terminalbeats.808-rhythm-composer";
    const CLAP_DESCRIPTION: Option<&'static str> =
        Some("TR-808 drum machine with analog-modeled synthesis");
    const CLAP_MANUAL_URL: Option<&'static str> = None;
    const CLAP_SUPPORT_URL: Option<&'static str> = None;
    const CLAP_FEATURES: &'static [ClapFeature] = &[
        ClapFeature::Instrument,
        ClapFeature::DrumMachine,
        ClapFeature::Synthesizer,
    ];
}

impl Vst3Plugin for EightOhEight {
    const VST3_CLASS_ID: [u8; 16] = *b"TB808RhythmComp!";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] = &[
        Vst3SubCategory::Instrument,
        Vst3SubCategory::Drum,
        Vst3SubCategory::Synth,
    ];
}

nih_export_clap!(EightOhEight);
nih_export_vst3!(EightOhEight);
