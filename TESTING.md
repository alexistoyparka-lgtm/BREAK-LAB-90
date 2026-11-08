# BREAK LAB 90 — Validation Checklist

This file deliberately separates automated host/build validation from a real MPC test. A successful host test is not evidence that the plugin works on the MPC Live 1.

## Automated checks

| Check | Expected result | Current status |
|---|---|---|
| ARMv7 / armhf build | 32-bit ARM VST binary is produced | Awaiting a fresh 0.1.4 CI result |
| OpenMPC wrapper host test | Parameter, preset and wrapper checks pass | Awaiting a fresh 0.1.4 CI result |
| Engine smoke test under ASan/UBSan | No sanitizer errors; bundled WAV loads | Awaiting a fresh 0.1.4 CI result |
| 48 kHz stereo WAV | Loads and plays | Test added |
| 44.1 kHz mono WAV | Loads and plays | Test added |
| Invalid RIFF size / PCM byte rate | File is rejected | Test added |
| 16 MIDI slices | Notes 36–51 each produce audio | Test added |
| Selected slice audition | MIDI note 60 plays selected slice | Test added |
| MIDI velocity | Velocity scales output level | Test added |\n| Vintage processing | Bit-depth reduction and sample-hold rate crush change the rendered signal | Tests added |
| Pitch | Parameter round-trip, clamping and audible playback-rate change work | Test added |
| Reverse | Reverse playback changes sample order and remains inside the selected slice | Direction comparison added; boundary guard included |
| Slice edge fade | First output frame starts at silence; sample remains audible | Test added |
| Factory presets | Four presets validate and appear in host preset list | Preset data added; CI check pending |

## Physical MPC Live 1 / OS 3.10

**NOT TESTED**

Only test after a fresh CI run has produced the ARMv7 package. Back up the MPC's plugin list and any existing plugin folder before installing. Keep the first test non-destructive.

| Test | Acceptance criteria | Result |
|---|---|---|
| Plugin discovery | BREAK LAB 90 appears in the instrument list without a scan crash | Not tested |
| Plugin load | Opens without error; sample status reads `break01.wav READY` | Not tested |
| Pad mapping | Pads/MIDI notes 36–51 trigger all 16 slices | Not tested |
| Selected slice | Selected Slice plus MIDI C4 auditions the chosen slice | Not tested |
| Audio quality | No hangs, unexpected silence, severe clicks or distortion at normal levels | Not tested |
| Pitch | -24 to +24 semitones behaves correctly while playing | Not tested |
| Reverse | Newly triggered slices play backwards; already-playing voices keep their original direction | Not tested |
| Vintage | Bit-depth and sample-rate reduction respond to their controls | Not tested |
| Presets | All four factory presets load and restore their values | Not tested |
| Project persistence | Save, close, reopen and confirm parameters/preset remain correct | Not tested |
| CPU / stability | No audio dropouts during a 10-minute pattern with 8 voices | Not tested |
| Restart | Close and relaunch MPC; plugin remains discoverable and sample loads | Not tested |

## Release gate

Do not mark this plugin finished or publish a release until:
1. A fresh CI run passes the ARMv7 build, wrapper host test and engine smoke test.
2. The generated skin preview has been inspected.
3. The exact generated package has been installed and tested on a physical MPC Live 1 running OS 3.10.
4. Save/reload, restart and CPU/stability checks pass.
