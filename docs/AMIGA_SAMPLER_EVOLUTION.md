# BREAK LAB 90 — Amiga Sampler Evolution Plan

Status: architecture and implementation plan, created on branch `amiga-sampler-next`.
Target remains the first-generation MPC Live (ARMv7/armhf) through the existing OpenMPC VST2 wrapper. Do not change the plugin ABI, parameter IDs already shipped, or installation layout without a compatibility test.

## Product direction

Turn BREAK LAB 90 into a playable breakbeat instrument with two complementary identities:

1. **PAULA mode** — deliberately authentic, crunchy Amiga-style sample playback: 8-bit quantisation, period/clock-derived playback rate, selectable nearest/linear interpolation, deterministic sample-hold / aliasing character, classic stereo placement, and optional LED-style low-pass.
2. **STUDIO mode** — modern, clean sampler playback with high-quality interpolation, independent pitch and tempo, transient-aware slicing, envelopes, loop tools and controlled saturation.

The point is not to make every sound lo-fi. The user should be able to move from accurate vintage character to clean modern editing without changing instruments.

## Reference research and reuse policy

Use these projects as technical references, not as code to paste blindly:

- BitLink (MIT): Amiga-inspired break sampler, crunchy resampling, transient slicing and compact workflow. Evaluate its exact source/license before adapting any implementation.
- Paula Tracker: software Paula emulation, period-based playback, ProTracker effects and tracker-oriented interaction. Use as a behavioural reference for PAULA mode.
- MilkyTracker Amiga resampler notes: important distinction between mathematically clean interpolation and authentic Paula output; true hardware character requires a non-interpolating / sample-hold path.
- Signalsmith Stretch (MIT, C++11): candidate for offline/worker-thread pitch/time processing. Its documentation recommends moderate stretch ratios for best results and exposes split computation to spread CPU work.
- libsamplerate (BSD-2-Clause): optional high-quality sample-rate conversion path; benchmark SINC_FASTEST / MEDIUM / BEST against the existing linear path on ARMv7.
- aubio: onset and beat analysis reference. It is GPL-3.0-or-later, so do not link it into the plugin unless the project explicitly accepts the resulting licensing obligations. Prefer a small in-house transient detector for the first implementation.
- Shortcircuit XT: reference for mature sampler concepts (groups, variants, per-voice processors, modulation), not a dependency for this constrained MPC target.

All third-party code must retain its upstream license and notices. No source should be copied until the repository's own licensing position and the upstream license are checked.

## Engine architecture

Keep the current engine ABI and separate DSP into small, testable modules. No allocation, file I/O, locks, logging, or unbounded work on the audio thread.

### A. Sample asset and import
- Parse RIFF/WAVE safely; add PCM 8/16/24/32 and IEEE float32 decoding after tests.
- Add mono/stereo handling, sample-rate metadata, peak/RMS, duration and DC estimate.
- Load/decode on a non-audio thread; publish immutable sample data to the renderer with a safe handoff.
- Multiple sample slots / banks, initially bounded (e.g. 16 slots) with explicit memory budget for Gen1.
- Browser/import UI must never replace a sample while an audio callback is reading it.

### B. Intelligent slicing
- Analysis runs off the audio thread.
- Compute multi-band energy flux / spectral positive difference plus short-window RMS rise.
- Combine onset strength with local peak and minimum-spacing rules; suppress double triggers and low-energy false positives.
- Sensitivity, minimum gap, transient emphasis and grid strength.
- Beat/grid assist: estimate tempo candidates from onset intervals, offer 1/2, 1/4, 1/8 and 1/16 grids; never silently force quantisation.
- Manual slice handles remain authoritative. Add snap, nudge, audition, merge, split and undo.
- Store slice markers in sample-frame coordinates, not normalised floating point alone.
- Add zero-crossing search and short equal-power edge fades to reduce clicks.

### C. Playback and pitch
- Replace fixed 16 equal divisions with a slice table (retain 16-slice default and MIDI notes 36–51 for backward compatibility).
- Per-slice start/end, gain, pan, tune, reverse, one-shot/loop and choke group.
- Playback interpolation choices: PAULA hold/nearest, linear, and a CPU-benchmarked higher-quality mode.
- Use a polyphase band-limited resampler only where quality warrants it; precompute/cache where possible.
- Correct sample-rate conversion independently from musical pitch.
- Preserve the existing 8-voice baseline first; measure before increasing polyphony.
- Voice stealing by quietest/oldest policy with short de-click fade; avoid resetting shared global rate-crush state when a new voice starts.

### D. Time and pitch algorithms
- Separate **varispeed** (pitch and duration move together) from **time-stretch** (tempo changes while pitch is held).
- First implement safe offline / background render using Signalsmith Stretch, then cache the rendered result.
- Do not run expensive stretch analysis in the MPC audio callback.
- Add transient preservation / transient re-alignment for breakbeats; benchmark 0.75x–1.5x first.
- If the CPU or memory budget is exceeded, fall back gracefully to varispeed and tell the user via a status parameter.

### E. Character and FX
- PAULA: 8-bit / 6-bit / 12-bit options, nearest/hold resampling, clock/period pitch, controlled aliasing, LED-style low-pass and L/R/R/L pan option.
- STUDIO: clean interpolation, per-slice envelope, HP/LP filter, saturation, transient shaper and soft clip.
- Keep effects simple and bounded: no unbounded feedback; clamp all parameters; test denormals and NaNs.
- Master output should avoid accidental hard clipping; provide a clip indicator rather than silently destroying peaks.

## UI / template proposal

A touch-first three-page layout. Large controls, short labels, no tiny tracker text on the MPC screen.

### PAGE 1 — PLAY
- Large 16-pad slice grid with clear selected/playing states.
- Main waveform overview with playhead and slice colour bands if the OpenMPC skin API permits.
- Big TEMPO / SYNC / PITCH controls and PAULA ↔ STUDIO mode switch.
- Four performance macros: CRUNCH, SPACE, TONE, DECAY.
- Master level and peak/clip indicator.
- Q-Links: Slice, Pitch, Mode, Crunch, Filter, Decay, Tempo, Output.

### PAGE 2 — SLICE
- Waveform zoom / selected slice start and end.
- AUTO SLICE, sensitivity, minimum gap, grid and snap.
- Start/end nudge, zero-crossing, fade, audition, reverse.
- Slice length and transient/beat marker feedback.
- Undo / reset markers.

### PAGE 3 — CHARACTER
- PAULA controls: bit depth, playback rate/clock, interpolation, LED filter, stereo mode.
- STUDIO controls: stretch amount, pitch, envelope, filter, saturation.
- A/B audition and level-matched compare.
- CPU / memory status readouts where the host skin supports them.

### UX rules
- One action should produce an audible result immediately.
- Selected slice is always obvious.
- Auto analysis is non-destructive; manual edits are never overwritten without explicit reset.
- Every macro has a useful musical range and level-matched defaults.
- Presets describe musical outcomes (Dusty Break, Jungle Chopper, Clean Pocket, Reverse Ghost), not unexplained engineering values.
- Keep existing preset IDs and current parameter keys stable; add new parameters compatibly.

## Implementation sequence and release gates

### Phase 0 — baseline and safety
- Record current main commit and compare it with the exact build the user tested.
- Preserve the current working build as a tagged rollback point.
- Add benchmark harness for render time, peak memory, voice count and output equivalence.
- Confirm actual OpenMPC skin and parameter limits before UI work.

### Phase 1 — safe sample model + manual slicing
- Immutable sample asset handoff.
- Multi-format decoder tests.
- Slice table and editable boundaries.
- Zero-crossing and de-click fades.
- Keep old 16 equal slices as default compatibility mode.

### Phase 2 — intelligent transient analysis
- Custom multi-band energy-flux detector.
- Sensitivity / spacing / grid assist.
- Golden WAV fixtures with expected onset ranges and false-positive tests.
- Analysis runs outside the audio thread.

### Phase 3 — PAULA and STUDIO playback
- Accurate PAULA-style sample/period playback path.
- Modern interpolation path.
- Per-slice controls, envelopes and choke groups.
- CPU profiling on ARMv7 before adding more voices.

### Phase 4 — time-stretch and resampling
- Benchmark Signalsmith Stretch and libsamplerate on the target.
- Background render/cache only at first.
- Add transient-preserving mode only if CPU, latency and sound-quality tests pass.

### Phase 5 — UI polish and acceptance
- Three-page skin and Q-Link map.
- Preset design and level matching.
- Automated DSP tests + ARMv7 build + package inspection.
- User confirms real MPC playback, touch/Q-Link workflow, save/reopen and long-session stability.

## Non-negotiable technical constraints
- Target: ARMv7 32-bit hard-float, first-generation MPC Live.
- Keep the existing OpenMPC plugin wrapper and VST2 packaging path.
- Audio callback must remain real-time safe.
- Every DSP feature needs unit tests and an ARMv7 CPU/memory benchmark.
- No claims of "perfect", "100% compatible" or "pro quality" from a successful compile alone.
- A clean modern path and an authentic deliberately degraded Amiga path must coexist.

## Research links
- https://github.com/frangedev/bitlink
- https://github.com/RednibCoding/Paula-Tracker
- https://github.com/Signalsmith-Audio/signalsmith-stretch
- https://github.com/libsndfile/libsamplerate
- https://github.com/aubio/aubio
- https://github.com/8bitbubsy/pt2-clone
- https://github.com/milkytracker/MilkyTracker
- https://surge-synth-team.org/shortcircuit-xt/
