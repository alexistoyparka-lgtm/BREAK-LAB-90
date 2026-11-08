# BREAK LAB 90

A breakbeat sampler plugin project for OpenMPC, targeting first-generation MPC hardware (ARM32 / armhf).

## Project status

This repository is being prepared for a real standalone MPC plugin. The current prototype is an early, unvalidated engine and is **not yet a finished or hardware-tested plugin**.

## Planned features

- Original built-in break material
- 16 MIDI/pad-triggered slices
- Pitch and reverse
- Clean and vintage processing modes
- Low-pass filtering, bit-depth and sample-rate reduction
- Later milestones: WAV import, transient detection, editable slice points, resampling and time-stretch

## Target and validation

- Target: first-generation MPC Live (32-bit ARM / armhf)
- Build and API compatibility must be checked against the current OpenMPC SDK
- A successful remote build does not replace installation and smoke-testing on the actual MPC

## Copyright

Bundled break material must be original or appropriately licensed. Users are responsible for rights to audio they import.
