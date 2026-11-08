# BREAK LAB 90 — User Guide (development 0.1.3)

## Current status

This is a development build for the first-generation Akai MPC Live (32-bit ARM / armhf), with MPC OS 3.10 as the intended target. The latest source changes have not yet been confirmed by a fresh ARM build and host test run, and no physical MPC Live 1 test has been completed. Do not install this development package on a production MPC yet.

## Supplying your own break

The current prototype loads one WAV file at engine startup:

`engine/breaks/break01.wav`

In an installed plugin folder, the expected relative path is:

`engine/breaks/break01.wav`

To use another break, back up the bundled file and replace it with your own audio using the exact filename `break01.wav`. The current prototype has no on-screen file browser or import command. Reopen the plugin after replacing the file so the engine loads it again.

Supported audio:

- RIFF/WAVE container
- Linear PCM, 16-bit
- Mono or stereo
- 8,000–192,000 Hz
- Up to 8,388,608 decoded frames (the engine stores two float channels, capped at about 64 MiB)

24-bit, 32-bit float, MP3, AIFF and compressed WAV formats are not supported. A file with invalid RIFF/chunk lengths or inconsistent PCM format fields is rejected.

## Playing slices

- MIDI notes 36–51 trigger slices 1–16 as one-shots.
- MIDI note 60 (C4) triggers the currently selected slice.
- Slice boundaries are currently 16 equal divisions of the loaded WAV.
- Eight voices can play simultaneously. When all are busy, the engine steals voices in round-robin order.
- MIDI velocity scales playback level.
- Short edge fades reduce clicks at slice starts and ends.

## Sound controls

- **Selected Slice:** 1–16 (internally 0–15).
- **Pitch:** -24 to +24 semitones. Pitch changes affect voices already playing.
- **Direction:** Forward or Reverse; direction is captured when a voice starts.
- **Mode:** Clean or Vintage.
- **Bits:** 4–16-bit quantization in Vintage mode.
- **Rate Crush:** sample-and-hold reduction in Vintage mode.
- **Filter:** one-pole low-pass.
- **Volume:** master output level.

The MPC PRESET menu includes Clean Break, Dusty 12-Bit, Dark Warehouse and Reverse Texture.

## Not implemented yet

- On-screen WAV browser/import
- Multiple sample banks
- Transient detection and editable slice boundaries
- Pitch-preserving time-stretch
- Resampling
- On-device CPU benchmarking and physical MPC Live 1 validation
