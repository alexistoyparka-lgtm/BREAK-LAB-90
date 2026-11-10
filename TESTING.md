# BREAK LAB 90 — Validation Checklist

This file separates automated host/build validation, package validation and physical MPC acceptance. A successful CI run is not evidence that the plugin works on a real MPC Live 1.

## Automated build and engine checks

| Check | Expected result | Current status |
|---|---|---|
| ARMv7 / armhf build | 32-bit ARM VST binary is produced | Passed in CI run #72 |
| OpenMPC wrapper host test | Parameter, preset and wrapper checks pass | Passed in CI run #72 |
| Engine smoke test under ASan/UBSan | No sanitizer errors; bundled WAV loads | Passed in CI run #72 |
| 48 kHz stereo WAV | Loads, preserves independent left/right channels and uses source-rate playback step | Passed |
| 44.1 kHz mono WAV | Loads and plays at correct host-rate step | Passed |
| Invalid RIFF size / PCM byte rate | File is rejected | Passed |
| 16 MIDI slices | Notes 36–51 each produce audio | Passed |
| Polyphony | Eight voices overlap; ninth trigger steals a voice safely | Passed |
| Selected slice audition | MIDI note 60 plays selected slice | Passed |
| Filter / master volume | Controls alter rendered signal | Passed |
| MIDI velocity | Velocity scales output level | Passed |
| Vintage processing | Bit-depth reduction and sample-hold rate crush alter output | Passed |
| Pitch | Parameter round-trip, clamping and audible playback-rate change work | Passed |
| Reverse | Reverse changes sample order and stays inside selected slice | Passed |
| Slice edge fade | First output frame starts at silence; sample remains audible | Passed |
| Factory presets | Four presets validate and appear in host preset list | Passed |

## Package validation

The latest CI artifact was downloaded and inspected separately:

| Check | Result |
|---|---|
| Artifact ZIP | Opened successfully |
| Inner MPC package | Opened successfully |
| SHA256SUMS | All packaged files matched |
| Plugin binary | ELF 32-bit LSB shared object, ARM, EABI5, hard-float |
| install.sh / uninstall.sh | Both pass sh -n |
| Installer round-trip | Passed against a disposable test MPC.settings and Synths folder |
| Backup / registration / removal | Backup created, plugin UID added once and removed on uninstall |
| Service hooks | Test mode logged stop/start for install and uninstall |
| Skin preview | Inspected; Slice label truncation corrected in 0.1.5 |

The installer round-trip used a disposable fixture and did not access or modify a physical MPC.

## Physical MPC Live 1 / OS 3.10

**NOT YET TESTED — first controlled hardware test is the next release gate.**

Requirements:
- First-generation MPC Live (ARMv7 / armhf).
- MPC OS 3.10 target.
- Working root shell access.
- Full backup of MPC.settings, projects and existing plugin folder.
- Exact artifact from the successful CI run; do not rebuild or repackage it.

| Test | Acceptance criteria | Result |
|---|---|---|
| Plugin discovery | BREAK LAB 90 appears in instrument list without scan crash | Not tested |
| Plugin load | Opens without error; status reads break01.wav READY | Not tested |
| Skin | Custom MPC page displays correctly and touch controls respond | Not tested |
| Q-Links | Each mapped control changes the expected parameter | Not tested |
| Pad mapping | MIDI notes 36–51 trigger all 16 slices | Not tested |
| Selected slice | Selected Slice plus MIDI C4 auditions chosen slice | Not tested |
| Audio quality | No hangs, unexpected silence, severe clicks or distortion | Not tested |
| Pitch / Reverse | Both controls behave correctly during playback | Not tested |
| Vintage | Bits and Rate Crush respond to controls | Not tested |
| Presets | All four factory presets load and restore values | Not tested |
| Project persistence | Save, close, reopen and confirm parameters/preset remain correct | Not tested |
| CPU / stability | No dropouts during a 10-minute pattern with 8 voices | Not tested |
| Restart | Plugin remains discoverable and WAV reloads after MPC restart | Not tested |
| Uninstall | Plugin entry removed; user data and backup remain intact | Not tested |

## Release gate

Do not call this a finished or hardware-certified plugin until:
1. A fresh CI run passes ARMv7 build, wrapper host test and engine smoke test.
2. Package integrity and installer checks pass.
3. The exact package loads on a physical MPC Live 1 running OS 3.10.
4. Skin, Q-Links, audio, MIDI, presets and save/reload pass.
5. Restart and 10-minute stability checks pass.

The current 0.1.5 build is a controlled-test candidate, not a production release.
