#![allow(dead_code)]
//! TR-808 drum synthesis engine.
//!
//! Ported from Terminal Beats `tb-core/src/synth.rs` — parametric models
//! derived from the real TR-808 circuit topology.  Adapted for real-time
//! per-sample rendering inside a VST/CLAP plugin.

use std::f32::consts::PI;

// ── Voice enum ──────────────────────────────────────────────────────

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[repr(usize)]
pub enum DrumVoice {
    Kick = 0,
    Snare,
    LowTom,
    MidTom,
    HighTom,
    LowConga,
    MidConga,
    HiConga,
    RimShot,
    Claves,
    Clap,
    Maracas,
    Cowbell,
    Cymbal,
    OpenHat,
    ClosedHat,
}

pub const NUM_VOICES: usize = 16;

impl DrumVoice {
    pub const ALL: [DrumVoice; NUM_VOICES] = [
        DrumVoice::Kick,
        DrumVoice::Snare,
        DrumVoice::LowTom,
        DrumVoice::MidTom,
        DrumVoice::HighTom,
        DrumVoice::LowConga,
        DrumVoice::MidConga,
        DrumVoice::HiConga,
        DrumVoice::RimShot,
        DrumVoice::Claves,
        DrumVoice::Clap,
        DrumVoice::Maracas,
        DrumVoice::Cowbell,
        DrumVoice::Cymbal,
        DrumVoice::OpenHat,
        DrumVoice::ClosedHat,
    ];

    pub fn label(self) -> &'static str {
        match self {
            Self::Kick => "Bass Drum",
            Self::Snare => "Snare Drum",
            Self::LowTom => "Low Tom",
            Self::MidTom => "Mid Tom",
            Self::HighTom => "High Tom",
            Self::LowConga => "Low Conga",
            Self::MidConga => "Mid Conga",
            Self::HiConga => "Hi Conga",
            Self::RimShot => "Rim Shot",
            Self::Claves => "Claves",
            Self::Clap => "Hand Clap",
            Self::Maracas => "Maracas",
            Self::Cowbell => "Cowbell",
            Self::Cymbal => "Cymbal",
            Self::OpenHat => "Open Hat",
            Self::ClosedHat => "Closed Hat",
        }
    }

    pub fn short(self) -> &'static str {
        match self {
            Self::Kick => "BD",
            Self::Snare => "SD",
            Self::LowTom => "LT",
            Self::MidTom => "MT",
            Self::HighTom => "HT",
            Self::LowConga => "LC",
            Self::MidConga => "MC",
            Self::HiConga => "HC",
            Self::RimShot => "RS",
            Self::Claves => "CL",
            Self::Clap => "CP",
            Self::Maracas => "MA",
            Self::Cowbell => "CB",
            Self::Cymbal => "CY",
            Self::OpenHat => "OH",
            Self::ClosedHat => "CH",
        }
    }

    /// Voices that this voice chokes (silences) on trigger.
    pub fn chokes(self) -> &'static [DrumVoice] {
        match self {
            Self::ClosedHat => &[DrumVoice::OpenHat],
            _ => &[],
        }
    }

    /// Color as (R, G, B) matching the original 808 button groups.
    pub fn color_rgb(self) -> (u8, u8, u8) {
        match self {
            Self::Kick => (200, 55, 55),
            Self::Snare | Self::LowTom | Self::MidTom | Self::HighTom => (220, 135, 45),
            Self::LowConga | Self::MidConga | Self::HiConga | Self::RimShot | Self::Claves
            | Self::Cowbell => (215, 195, 60),
            Self::Clap | Self::Maracas | Self::Cymbal | Self::OpenHat | Self::ClosedHat => {
                (195, 195, 195)
            }
        }
    }
}

/// Map GM percussion MIDI note → DrumVoice.
pub fn midi_to_voice(note: u8) -> Option<DrumVoice> {
    match note {
        35 | 36 => Some(DrumVoice::Kick),
        38 | 40 => Some(DrumVoice::Snare),
        41 => Some(DrumVoice::LowTom),
        43 | 47 => Some(DrumVoice::MidTom),
        45 | 48 => Some(DrumVoice::HighTom),
        60 | 63 => Some(DrumVoice::LowConga),
        61 | 64 => Some(DrumVoice::MidConga),
        62 | 65 => Some(DrumVoice::HiConga),
        37 => Some(DrumVoice::RimShot),
        75 => Some(DrumVoice::Claves),
        39 => Some(DrumVoice::Clap),
        70 => Some(DrumVoice::Maracas),
        56 => Some(DrumVoice::Cowbell),
        49 | 52 | 57 => Some(DrumVoice::Cymbal),
        46 => Some(DrumVoice::OpenHat),
        42 | 44 => Some(DrumVoice::ClosedHat),
        _ => None,
    }
}

// ── Shared DSP primitives ───────────────────────────────────────────

#[derive(Clone)]
pub struct Rng(u64);

impl Rng {
    pub fn new(seed: u64) -> Self {
        Self(if seed == 0 { 0x9E37_79B9_7F4A_7C15 } else { seed })
    }

    #[inline]
    fn next_u64(&mut self) -> u64 {
        let mut x = self.0;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        self.0 = x;
        x.wrapping_mul(0x2545_F491_4F6C_DD1D)
    }

    #[inline]
    pub fn noise(&mut self) -> f32 {
        let bits = (self.next_u64() >> 40) as u32;
        (bits as f32 / 0x0080_0000 as f32) - 1.0
    }

    #[inline]
    pub fn unit(&mut self) -> f32 {
        self.noise()
    }
}

#[inline]
pub fn soft_clip(x: f32) -> f32 {
    x.tanh()
}

#[derive(Clone, Copy, Default)]
pub struct OnePole {
    state: f32,
}

impl OnePole {
    #[inline]
    pub fn lowpass(&mut self, x: f32, cutoff_hz: f32, sample_rate: f32) -> f32 {
        let cutoff = cutoff_hz.clamp(1.0, sample_rate * 0.49);
        let a = 1.0 - (-2.0 * PI * cutoff / sample_rate).exp();
        self.state += a * (x - self.state);
        self.state
    }

    #[inline]
    pub fn highpass(&mut self, x: f32, cutoff_hz: f32, sample_rate: f32) -> f32 {
        x - self.lowpass(x, cutoff_hz, sample_rate)
    }
}

#[derive(Clone, Copy, Default)]
pub struct Svf {
    low: f32,
    band: f32,
}

pub struct SvfOut {
    pub low: f32,
    pub band: f32,
    pub high: f32,
}

impl Svf {
    #[inline]
    pub fn process(&mut self, x: f32, cutoff_hz: f32, q: f32, sample_rate: f32) -> SvfOut {
        let f = 2.0 * (PI * cutoff_hz.clamp(20.0, sample_rate * 0.45) / sample_rate).sin();
        let damp = (1.0 / q.max(0.1)).min(2.0);
        let high = x - self.low - damp * self.band;
        self.band += f * high;
        self.low += f * self.band;
        SvfOut {
            low: self.low,
            band: self.band,
            high,
        }
    }
}

#[inline]
pub fn exp_env(t: f32, tau: f32) -> f32 {
    if tau <= 0.0 {
        return 0.0;
    }
    (-t / tau).exp()
}

#[inline]
fn decay_to_tau(decay_seconds: f32) -> f32 {
    decay_seconds / 4.605_170_2
}

// ── Humanization ────────────────────────────────────────────────────

#[derive(Clone, Copy)]
pub struct Humanize {
    pub tune_mul: f32,
    pub decay_mul: f32,
    pub jitter_samples: i32,
}

impl Humanize {
    pub fn roll(rng: &mut Rng, sample_rate: f32) -> Self {
        Self {
            tune_mul: 1.0 + rng.unit() * 0.005,
            decay_mul: 1.0 + rng.unit() * 0.03,
            jitter_samples: (rng.unit() * 0.0007 * sample_rate) as i32,
        }
    }

    pub fn none() -> Self {
        Self {
            tune_mul: 1.0,
            decay_mul: 1.0,
            jitter_samples: 0,
        }
    }
}

// ── Per-hit oscillator state ────────────────────────────────────────

#[derive(Clone, Default)]
pub struct VoiceState {
    hp1: OnePole,
    hp2: OnePole,
    svf1: Svf,
    svf2: Svf,
    phase: [f32; 6],
}

const METALLIC_FREQS: [f32; 6] = [205.3, 304.4, 369.6, 522.7, 540.0, 800.0];

// ── Envelope ────────────────────────────────────────────────────────

#[derive(Clone, Copy)]
pub struct Envelope {
    pub attack_ms: f32,
    pub decay_ms: f32,
    pub sustain: f32,
    pub release_ms: f32,
}

impl Default for Envelope {
    fn default() -> Self {
        Self {
            attack_ms: 0.1,
            decay_ms: 0.0,
            sustain: 1.0,
            release_ms: 0.0,
        }
    }
}

pub fn apply_adsr(env: Envelope, t: f32, gate_seconds: f32) -> f32 {
    let attack_s = (env.attack_ms / 1000.0).max(0.0001);
    let decay_s = (env.decay_ms / 1000.0).max(0.0);
    let sustain = env.sustain.clamp(0.0, 1.0);
    let release_s = (env.release_ms / 1000.0).max(0.0);
    let release_start = (gate_seconds - release_s).max(attack_s + decay_s);

    if t < attack_s {
        t / attack_s
    } else if t < attack_s + decay_s {
        let d = (t - attack_s) / decay_s.max(0.0001);
        1.0 + (sustain - 1.0) * d
    } else if t < release_start {
        sustain
    } else if release_s > 0.0 {
        let r = ((t - release_start) / release_s).clamp(0.0, 1.0);
        sustain * (1.0 - r)
    } else {
        sustain
    }
}

// ── Voice durations ─────────────────────────────────────────────────

pub fn natural_duration_seconds(voice: DrumVoice) -> f32 {
    match voice {
        DrumVoice::Kick => 0.55,
        DrumVoice::Snare => 0.30,
        DrumVoice::LowTom | DrumVoice::MidTom | DrumVoice::HighTom => 0.45,
        DrumVoice::LowConga | DrumVoice::MidConga | DrumVoice::HiConga => 0.30,
        DrumVoice::RimShot | DrumVoice::Claves => 0.10,
        DrumVoice::Clap => 0.35,
        DrumVoice::Maracas => 0.08,
        DrumVoice::Cowbell => 0.40,
        DrumVoice::Cymbal => 1.2,
        DrumVoice::OpenHat => 0.9,
        DrumVoice::ClosedHat => 0.12,
    }
}

// ── Per-voice synthesis ─────────────────────────────────────────────

fn synth_sample(
    voice: DrumVoice,
    t: f32,
    sr: f32,
    hz: Humanize,
    rng: &mut Rng,
    st: &mut VoiceState,
    tail: f32,
) -> f32 {
    let stretch = |base: f32| base.max(tail * 0.9);
    match voice {
        DrumVoice::Kick => kick(t, sr, hz, st, stretch(0.42)),
        DrumVoice::Snare => snare(t, sr, hz, rng, st, stretch(0.10)),
        DrumVoice::LowTom => tuned_tom(t, sr, hz, st, 90.0, stretch(0.35)),
        DrumVoice::MidTom => tuned_tom(t, sr, hz, st, 130.0, stretch(0.32)),
        DrumVoice::HighTom => tuned_tom(t, sr, hz, st, 180.0, stretch(0.28)),
        DrumVoice::LowConga => tuned_tom(t, sr, hz, st, 220.0, stretch(0.18)),
        DrumVoice::MidConga => tuned_tom(t, sr, hz, st, 280.0, stretch(0.16)),
        DrumVoice::HiConga => tuned_tom(t, sr, hz, st, 340.0, stretch(0.14)),
        DrumVoice::RimShot => ping(t, sr, hz, rng, 1400.0, 0.05),
        DrumVoice::Claves => ping(t, sr, hz, rng, 2500.0, 0.06),
        DrumVoice::Clap => clap(t, sr, hz, rng, st),
        DrumVoice::Maracas => maracas(t, sr, hz, rng, st),
        DrumVoice::Cowbell => cowbell(t, sr, hz, st, stretch(0.28)),
        DrumVoice::Cymbal => metallic(t, sr, hz, st, stretch(0.9), true),
        DrumVoice::OpenHat => metallic(t, sr, hz, st, stretch(0.55), false),
        DrumVoice::ClosedHat => metallic(t, sr, hz, st, stretch(0.08), false),
    }
}

/// BD: bridged-T sine resonator with fast pitch sweep + click transient.
fn kick(t: f32, _sr: f32, hz: Humanize, _st: &mut VoiceState, body_decay_s: f32) -> f32 {
    let rest = 58.0 * hz.tune_mul;
    let sweep_tau = 0.018;
    let phase = 2.0 * PI * (rest * t + (rest * 3.0 * sweep_tau) * (1.0 - exp_env(t, sweep_tau)));
    let body = phase.sin() * exp_env(t, decay_to_tau(body_decay_s) * hz.decay_mul);
    let click = (2.0 * PI * 900.0 * t).sin() * exp_env(t, 0.004);
    soft_clip((body * 0.9 + click * 0.15) * 1.1)
}

/// SD: two shell tones + bright noise + retriggered micro-burst skitter.
fn snare(t: f32, sr: f32, hz: Humanize, rng: &mut Rng, st: &mut VoiceState, noise_decay_s: f32) -> f32 {
    let f1 = 180.0 * hz.tune_mul;
    let f2 = 330.0 * hz.tune_mul;
    let tone_env = exp_env(t, decay_to_tau(0.07) * hz.decay_mul);
    let tone = ((2.0 * PI * f1 * t).sin() * 0.6 + (2.0 * PI * f2 * t).sin() * 0.4) * tone_env;

    let burst_spacing = 0.0035;
    let mut skitter_env = 0.0f32;
    for i in 0..4 {
        let bt = t - i as f32 * burst_spacing;
        if bt >= 0.0 {
            skitter_env = skitter_env.max(exp_env(bt, 0.0025));
        }
    }

    let main_noise_env = exp_env(t, decay_to_tau(noise_decay_s) * hz.decay_mul);
    let raw_noise = rng.noise();
    let hp = st.hp1.highpass(raw_noise, 2200.0, sr);
    let rattle = st.svf1.process(raw_noise, 3600.0, 4.5, sr).band;
    let noise = (hp * 0.7 + rattle * 0.5) * (main_noise_env.max(skitter_env * 0.9));
    soft_clip(tone * 0.35 + noise * 1.05)
}

/// Pitched sine resonator for toms and congas.
fn tuned_tom(t: f32, _sr: f32, hz: Humanize, _st: &mut VoiceState, rest_hz: f32, decay_s: f32) -> f32 {
    let rest = rest_hz * hz.tune_mul;
    let sweep_tau = 0.012;
    let phase = 2.0 * PI * (rest * t + (rest * 1.6 * sweep_tau) * (1.0 - exp_env(t, sweep_tau)));
    let env = exp_env(t, decay_to_tau(decay_s) * hz.decay_mul);
    soft_clip(phase.sin() * env)
}

/// RS / CL: bright ping — sine + noise, fast decay.
fn ping(t: f32, _sr: f32, hz: Humanize, rng: &mut Rng, freq_hz: f32, decay_s: f32) -> f32 {
    let env = exp_env(t, decay_to_tau(decay_s) * hz.decay_mul);
    let tone = (2.0 * PI * freq_hz * hz.tune_mul * t).sin();
    let noise = rng.noise() * 0.15;
    soft_clip((tone * 0.85 + noise) * env)
}

/// CP: retriggered band-passed noise bursts + decaying tail.
fn clap(t: f32, sr: f32, hz: Humanize, rng: &mut Rng, st: &mut VoiceState) -> f32 {
    let burst_spacing = 0.010 * hz.tune_mul.max(0.5);
    let n_bursts = 3;
    let mut burst_env = 0.0f32;
    for i in 0..n_bursts {
        let burst_t = t - i as f32 * burst_spacing;
        if burst_t >= 0.0 {
            burst_env = burst_env.max(exp_env(burst_t, 0.006));
        }
    }
    let tail_start = (n_bursts - 1) as f32 * burst_spacing;
    let tail_t = (t - tail_start).max(0.0);
    let tail_env = exp_env(tail_t, decay_to_tau(0.22) * hz.decay_mul) * 0.6;
    let env = burst_env.max(tail_env);
    let filtered = st.svf1.process(rng.noise(), 1100.0, 2.5, sr).band;
    soft_clip(filtered * env * 1.4)
}

/// MA: short high-passed noise burst.
fn maracas(t: f32, sr: f32, hz: Humanize, rng: &mut Rng, st: &mut VoiceState) -> f32 {
    let env = exp_env(t, decay_to_tau(0.045) * hz.decay_mul);
    let hp = st.hp1.highpass(rng.noise(), 6000.0, sr);
    soft_clip(hp * env * 1.2)
}

/// CB: two square waves band-passed, two-stage decay.
fn cowbell(t: f32, sr: f32, hz: Humanize, st: &mut VoiceState, tail_decay_s: f32) -> f32 {
    let f1 = 587.0 * hz.tune_mul;
    let f2 = 845.0 * hz.tune_mul;
    let square = |f: f32, t: f32| -> f32 {
        if (f * t).fract() < 0.5 { 1.0 } else { -1.0 }
    };
    let raw = square(f1, t) * 0.5 + square(f2, t) * 0.5;
    let filtered = st.svf1.process(raw, 1600.0, 3.0, sr).band;
    let hit_env = exp_env(t, decay_to_tau(0.05) * hz.decay_mul);
    let tail_env = exp_env(t, decay_to_tau(tail_decay_s) * hz.decay_mul) * 0.5;
    soft_clip(filtered * hit_env.max(tail_env))
}

/// Six inharmonic square oscillators → band-pass + high-pass for hats/cymbal.
fn metallic(t: f32, sr: f32, hz: Humanize, st: &mut VoiceState, decay_s: f32, bright: bool) -> f32 {
    let mut raw = 0.0f32;
    for (i, base) in METALLIC_FREQS.iter().enumerate() {
        let f = base * hz.tune_mul;
        let ph = (f * t + st.phase[i]).fract();
        raw += if ph < 0.5 { 1.0 } else { -1.0 };
    }
    raw /= METALLIC_FREQS.len() as f32;

    let bp = st
        .svf1
        .process(raw, if bright { 9000.0 } else { 8000.0 }, 1.4, sr)
        .band;
    let hp = st
        .hp1
        .highpass(bp, if bright { 6500.0 } else { 7200.0 }, sr);
    let hp2 = st
        .hp2
        .highpass(hp, if bright { 6500.0 } else { 7200.0 }, sr);
    let env = exp_env(t, decay_to_tau(decay_s) * hz.decay_mul);
    let _ = &mut st.svf2;
    soft_clip(hp2 * env * 2.5)
}

// ── Active-hit tracker for real-time rendering ──────────────────────

pub struct ActiveHit {
    pub voice: DrumVoice,
    state: VoiceState,
    rng: Rng,
    humanize: Humanize,
    envelope: Envelope,
    accent: bool,
    position: usize,
    length: usize,
    fade_out: bool,
    level: f32,
    tune_offset: f32,
    decay_scale: f32,
}

impl ActiveHit {
    pub fn new(
        voice: DrumVoice,
        accent: bool,
        level: f32,
        tune_offset: f32,
        decay_scale: f32,
        sample_rate: f32,
        seed: u64,
    ) -> Self {
        let mut h_rng = Rng::new(seed);
        let humanize = Humanize::roll(&mut h_rng, sample_rate);
        let dur = natural_duration_seconds(voice) * decay_scale.max(0.2);
        let length = (dur * sample_rate) as usize;
        Self {
            voice,
            state: VoiceState::default(),
            rng: Rng::new(seed.wrapping_add(1)),
            humanize: Humanize {
                tune_mul: humanize.tune_mul * (1.0 + (tune_offset - 0.5) * 0.4),
                ..humanize
            },
            envelope: Envelope::default(),
            accent,
            position: 0,
            length: length.max(64),
            fade_out: false,
            level,
            tune_offset,
            decay_scale,
        }
    }

    #[inline]
    pub fn render_sample(&mut self, sample_rate: f32) -> f32 {
        if self.position >= self.length {
            return 0.0;
        }
        let t = self.position as f32 / sample_rate;
        let gate_seconds = self.length as f32 / sample_rate;
        let tail = (gate_seconds - 0.03).max(0.0);

        let mut v = synth_sample(
            self.voice,
            t,
            sample_rate,
            self.humanize,
            &mut self.rng,
            &mut self.state,
            tail,
        );
        v *= if self.accent { 1.35 } else { 1.0 };
        v *= apply_adsr(self.envelope, t, gate_seconds);

        if self.fade_out {
            let fade_len = (sample_rate * 0.003).max(1.0) as usize;
            let remaining = self.length.saturating_sub(self.position);
            if remaining < fade_len {
                v *= remaining as f32 / fade_len as f32;
            }
        }

        self.position += 1;
        v * self.level
    }

    #[inline]
    pub fn is_done(&self) -> bool {
        self.position >= self.length
    }

    pub fn start_fade_out(&mut self) {
        if !self.fade_out {
            self.fade_out = true;
            let fade_samples = (self.length as f32 * 0.003).max(64.0) as usize;
            self.length = self.position + fade_samples;
        }
    }
}
