# USB packaging investigation

## Evidence

Full repository history and all remote branch tips were fetched. The functional
0.1.6 reference is run **79**, ID **43486085109**, commit
`4fa9c134b5956e5f0accc67f951fd4c2760662fa`, artifact **14068205199**.
Its SDK ZIP SHA-256 is
`eaa01153f1935971939c8c4c9d9a9ad9f54a04a96b3a7ef185543d59bacf091f`,
verified against the downloaded artifact and historical documentation.

The 0.2.0 reference is run **106**, ID **43621877021**, commit
`38eda3f6b40f229af43051e1fcecd07ebadf5d86`, artifact **14124059942**.
Its ARMv7 ZIP SHA-256 is
`59b9e5189a42a804601f2444377bfac49ca4bdc4cbe970330a6b875f31c10046`.

| Property | 0.1.6 run 79 | 0.2.0 run 106 |
|---|---|---|
| OpenMPC workflow and tools | `52c1fb072966d7de77e97dd9eec0544548b042dc` | `11d340572dcc93778f341a9484c5ae6c51126abe` |
| USB SDK archive | present | absent |
| Native discovery `plugin.json` | present | absent |
| UID | `0x42726b39` | same underlying VST UID |
| Binary | ELF32 ARM EABI5 hard-float | ELF32 ARM EABI5 hard-float |
| Latest GLIBC symbol | see original ELF | 2.29 |

The newer historical tools added native discovery descriptors in `tools/release.py`
and the single-folder USB conversion in `tools/sdk_zip.py`. The 0.2.0 workflow
reverted to an earlier tools commit which has neither capability. Its
`mpc-plugin.json` is a manager manifest, not the native `plugin.json` descriptor.
`plugin-meta.xml` is for registration, not a substitute for the native descriptor.
Copying the outer release folder also introduces unsupported nesting.

This establishes a packaging regression that explains the missing native discovery.
Only a physical test can confirm that it is the complete cause on the user's device.

## Repair

The existing build/wrapper tools pin is deliberately retained: moving to the 0.1.6
pin also changes `wrapper/vst2_wrap.c`, beyond this packaging repair. A second CI
job consumes the successful ARMv7 artifact, verifies its complete checksum inventory,
adds the historical discovery schema and generates both distributions.

`tools/package_usb.py` follows the descriptor and layout of the original 0.1.6
artifact. The SDK binary stays beside `engine/` and the skins, renamed to
`break_lab_90-arm32.so` as in the historical SDK converter. Its bytes do not change.
The Plugin Manager ZIP retains its original filename, binary path, install scripts,
MODES and portable layout, with refreshed checksums and the added descriptor.
The SDK ZIP contains one plugin folder, no installer and no legacy registration XML.
Its manager metadata follows the historical SDK convention (`arch: universal`, with
only an `armv7` build); `plugin.json` has only `file32`, never `file64`.

No DSP, VST UID, parameter list, presets, waveform or skin payload is changed.
This remains plugin version 0.2.0: a packaging candidate, not a new DSP revision.

## Validation

`tests/test_package_usb.py INPUT-mpc-armv7.zip` exercises actual ARM artifacts:
complete payload equality in both outputs, discovery identity and paths, manager
checksum inventory, deterministic output, rejection of tampering, unsafe paths,
wrong ELF class/float ABI and attempted input overwrite. The packager also checks
ARMv7 attributes, exported VSTPluginMain and GLIBC <= 2.29 via readelf.
The existing engine/DSP and wrapper ASan/UBSan tests remain in the ARM build job.

Hardware discovery, audio, persistence, Q-Links, CPU and prolonged stability remain
unverified for the new candidate. Follow INSTALL_USB_FR.md on the physical Gen1.
