# BREAK LAB 90 — MPC Live 1 test plan

This plan is for the first-generation MPC Live running MPC OS 3.10. Do not install until the matching GitHub Actions run has passed the ARMv7 build and host tests and its package and skin previews have been inspected.

## 1. Get the exact test package

1. Open the repository's **Actions** tab.
2. Open the successful **Build BREAK LAB 90** run for the commit being tested.
3. Download the artifact named `break-lab-90-v0.1.4`. The artifact contains the ARMv7 ZIP and skin preview PNGs.
4. Keep the ZIP unchanged. Test that exact ZIP; do not rebuild or repackage it.

The artifact is a CI run artifact, not a published release, and expires after 14 days.

## 2. Back up before installation

- Locate the device's actual `MPC.settings` file and make a backup before running the installer.
- Back up the existing BREAK LAB 90 plugin folder under `/sdcard/Synths`, if one exists.
- Keep a copy of the downloaded ZIP on the computer.

The package installer requires root access. It edits the MPC plugin list and stops/restarts MPC as part of installation.

## 3. Install the package

1. Transfer the ZIP to the MPC or a USB drive.
2. Extract it on the MPC or on a computer, then transfer the extracted folder without changing its contents.
3. Open a root shell in the extracted package directory.
4. Run `sh install.sh` and follow its confirmation prompt.
5. Wait for MPC to restart, then create a new empty test project.

Do not install while MPC is saving a project. Do not run the installer with MPC still running in a way that bypasses its own stop/restart procedure.

## 4. Functional test

- Confirm BREAK LAB 90 appears in the instrument list and loads without a scan or load error.
- Confirm the sample status reads `break01.wav READY`.
- Trigger MIDI notes 36–51 and verify all 16 slices play.
- Change Selected Slice and audition with MIDI C4 / note 60.
- Verify Pitch across -24, 0 and +24 semitones.
- Verify Forward and Reverse on newly triggered slices.
- Verify Clean/Vintage, Bits and Rate Crush.
- Turn Filter from dark to open and Volume from low to high.
- Trigger eight slices rapidly/overlapping to check polyphony and voice stealing.
- Save the project, close it, reopen it and confirm parameters and preset values persist.

## 5. Stability and uninstall

- Run a 10-minute pattern with up to eight overlapping voices. Listen for dropouts, hangs, unexpected silence or severe clicks.
- Close and relaunch MPC, then confirm the plugin is still listed and the WAV loads.
- If any critical fault occurs, stop testing and record the exact step and symptom.
- When finished, use the package's `sh uninstall.sh` if removing the test build. Keep the backup until the original plugin list and projects are confirmed intact.

## Acceptance

The first hardware test is successful only if plugin discovery, sample loading, all playback controls, save/reload, restart and the 10-minute stability check pass. A successful hardware smoke test does not replace a separate CPU benchmark.
