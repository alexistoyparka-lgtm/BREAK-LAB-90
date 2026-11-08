# BREAK LAB 90

A breakbeat sampler plugin project for OpenMPC, targeting first-generation MPC hardware (ARM32 / armhf).

## Project status

**Development version 0.1.4.** The earlier v0.1.1 baseline passed the ARMv7 build and OpenMPC host test suite with AddressSanitizer/UBSan. The current development branch adds bounded RIFF parsing, click-reducing slice fades, MIDI velocity handling, live pitch, round-robin voice stealing, selected-slice audition, reverse-boundary protection, factory presets and an on-screen sample status; these latest changes still need a fresh CI run.

This is not yet a finished or hardware-tested plugin. The CI checks validate the wrapper/API integration and produce the correct 32-bit ARM binary, but they do not prove that the plugin loads or performs correctly on an MPC Live 1 running OS 3.10. Do not install it on a production device yet.

[Read the development user guide](docs/USER_GUIDE.md) for WAV requirements, file placement, MIDI mapping and control behavior. See [TESTING.md](TESTING.md) for automated checks and the physical MPC acceptance checklist.

## Current prototype features

- Original bundled break material
- 16 MIDI/pad-triggered slices
- Pitch (live adjustment) and selectable Forward/Reverse playback direction
- Clean/Vintage processing modes with dedicated on-screen selectors
- Low-pass filtering, bit-depth and sample-rate reduction
- Eight simultaneous playback voices with short per-slice fade-in/fade-out to reduce boundary clicks
- MPC page readout confirms whether the bundled WAV loaded
- Four factory presets in the MPC PRESET menu: Clean Break, Dusty 12-Bit, Dark Warehouse and Reverse Texture

## Known limitations / next steps

- One bundled WAV only; PCM 16-bit mono or stereo, 8–192 kHz accepted (44.1 kHz recommended); decoded sample memory capped at 64 MiB
- Pads MIDI notes 36–51 trigger the 16 slices; MIDI C4 (note 60) auditions the slice selected by the Selected Slice control
- Slices are split into 16 equal regions; no transient detection or editable slice points yet
- WAV parsing validates chunk bounds and format fields; automated smoke tests load the bundled break, exercise 48 kHz stereo channel separation and 44.1 kHz mono input, verify source sample-rate conversion and audible pitch-rate changes and clamping, MIDI velocity scaling, Vintage bit-depth/rate-crush processing, eight-voice polyphony/voice stealing, low-pass filter response, selected-slice audition, reverse-direction changes, and reject malformed RIFF sizes and inconsistent PCM metadata
- No on-device CPU benchmark or real MPC smoke test
- WAV import, multiple banks, resampling, pitch-preserving time-stretch and a custom multi-page workflow remain future work
- Hardware installation requires root shell access and changes to the MPC plugin list; back up first

## Target and validation

- Target: first-generation MPC Live (32-bit ARM / armhf)
- CI: v0.1.1 baseline passed ARMv7 build and ASan/UBSan host tests; v0.1.4 changes are awaiting a fresh CI result
- Hardware: not tested; MPC Live 1 / OS 3.10 validation remains outstanding

## Copyright

Bundled break material is original. Users are responsible for rights to audio they import.
