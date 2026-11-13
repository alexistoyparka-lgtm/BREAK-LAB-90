# BREAK LAB 90 — MPC Live 1 test plan

Target: first-generation MPC Live, MPC OS 3.10.1 or later, ARMv7 / armhf.

This is a controlled USB-folder installation test. It does not require SSH, root access, or manual edits to MPC.settings. Do not claim hardware compatibility until the physical acceptance checklist below passes.

## 1. Get the exact package

1. Open Actions and run the Build BREAK LAB 90 workflow on branch compat/mpc-os-3.10.1-usb-package.
2. Download the artifact named break-lab-90-v0.1.6 from the successful run.
3. Inside it, use only the *-mpc-sdk.zip for MPC OS 3.10.1+. Do not use the legacy *-mpc-armv7.zip for the USB-folder test.
4. Keep the SDK ZIP unchanged and record its SHA-256.

The SDK ZIP is the folder-format package for the MPC's VST2 Plugins location. It must contain the ARMv7 plugin folder and its plugin.json descriptor.

## 2. Prepare the MPC and USB drive

1. Save the current MPC project and back up important projects and samples.
2. On the MPC, open Menu → Preferences → Enhancements.
3. Enable Enable Enhancements and External Plugin Support. Accept the warning.
4. Wait several seconds for the setting to save, then restart the MPC.
5. On a USB drive, create a folder named exactly VST2 Plugins at the drive root.
6. Extract the SDK ZIP. Copy the single plugin folder (the folder containing plugin.json, the .so and Plugin Skins/) directly into VST2 Plugins. Do not add another nesting level.

Leave the USB drive connected while using the plugin: the plugin and its data are read from that drive.

## 3. Load the plugin

1. Insert the USB drive. On removable media, the plugin should appear after a few seconds; no SSH or manual plugin-list registration should be needed.
2. Create a new empty test project.
3. Add a Plugin track and look under External VST Plugins for OpenBench → BREAK LAB 90.
4. Load it. Confirm the sample status reads break01.wav READY.

If it does not appear, do not modify MPC.settings. Photograph the browser and USB folder layout, then stop and report the exact firmware and storage used.

## 4. Functional acceptance

- Confirm the skin opens and touch controls respond.
- Trigger MIDI notes 36–51 and verify all 16 slices.
- Set Slice and audition the selected slice with MIDI C4 / note 60.
- Verify Pitch at -24, 0 and +24 semitones.
- Verify Forward and Reverse on newly triggered slices.
- Verify Clean/Vintage, Bits, Rate Crush, Filter and Volume.
- Trigger eight overlapping slices to check polyphony and voice stealing.
- Load each of the four factory presets.
- Save the project, close it, reopen it and confirm the plugin and values persist.

## 5. Stability, restart and removal

- Run a 10-minute pattern with up to eight overlapping voices. Listen for dropouts, hangs, unexpected silence or severe clicks.
- Restart MPC with the USB drive connected; confirm the plugin is still listed and the WAV loads.
- Close the project before removing the drive.
- To uninstall this USB test, close MPC and remove only the BREAK LAB 90 plugin folder from VST2 Plugins.

## Acceptance criteria

The first hardware test passes only if discovery, loading, skin, touch, MIDI, audio controls, presets, save/reload, restart and the 10-minute stability check all pass on the physical MPC Live Gen1 / OS 3.10.1.

A successful ARM build, host test or ZIP validation is not hardware validation. CPU performance remains unmeasured until tools/bench.sh is run on the device.