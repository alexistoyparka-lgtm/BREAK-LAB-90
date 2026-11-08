# BREAK LAB 90

A breakbeat sampler plugin project for OpenMPC, targeting first-generation MPC hardware (ARM32 / armhf).

## Project status

**Prototype v0.1.1 builds successfully for ARMv7 and passes the OpenMPC host test suite with AddressSanitizer/UBSan enabled.** The package and skin preview are available as GitHub Actions artifacts.

This is not yet a finished or hardware-tested plugin. The CI checks validate the wrapper/API integration and produce the correct 32-bit ARM binary, but they do not prove that the plugin loads or performs correctly on an MPC Live 1 running OS 3.10. Do not install it on a production device yet.

## Current prototype features

- Original bundled break material
- 16 MIDI/pad-triggered slices
- Pitch and reverse
- Clean and vintage processing modes
- Low-pass filtering, bit-depth and sample-rate reduction
- Eight simultaneous playback voices

## Known limitations / next steps

- One bundled WAV only; PCM 16-bit mono or stereo, 44.1 kHz recommended
- Slices are split into 16 equal regions; no transient detection or editable slice points yet
- No on-device CPU benchmark or real MPC smoke test
- WAV import, multiple banks, resampling, pitch-preserving time-stretch and a custom multi-page workflow remain future work
- Hardware installation requires root shell access and changes to the MPC plugin list; back up first

## Target and validation

- Target: first-generation MPC Live (32-bit ARM / armhf)
- CI: ARMv7 build, package validation, host tests with ASan/UBSan — passed
- Hardware: not tested; MPC Live 1 / OS 3.10 validation remains outstanding

## Copyright

Bundled break material is original. Users are responsible for rights to audio they import.
