# BREAK LAB 90 — prototype OpenMPC Gen1

**État : prototype source uniquement — pas encore compilé ni testé sur une MPC.**

Cible : MPC Live 1 / Live II / One / X / Key 61 Gen1 (ARM32), via le framework OpenMPC.

## Fonctions présentes dans ce premier moteur

- Instrument VST2 avec 16 slices déclenchées par MIDI : notes 36 à 51.
- Lecture WAV PCM 16-bit mono ou stéréo.
- Pitch de -24 à +24 demi-tons.
- Reverse, filtre passe-bas, volume.
- Modes CLEAN et VINTAGE (réduction de profondeur de bits et de fréquence d'échantillonnage).
- 8 voix de lecture simultanées.

## Limites

- Un seul WAV est prévu pour le moment : `engine/breaks/break01.wav`.
- Le WAV doit être PCM 16-bit, mono ou stéréo. 44,1 kHz recommandé.
- Le découpage est égal en 16 tranches ; pas encore de détection de transitoires ni d'édition des points de coupe.
- L'import depuis l'écran MPC, les banques multiples, le time-stretch indépendant du pitch, le resampling et le skin tactile personnalisé restent à développer.
- Le moteur doit être adapté/vérifié contre l'API OpenMPC actuelle, compilé, testé sur PC, puis validé sur le matériel réel. **Ne pas installer cette version sur la MPC.**

## Cible de compilation

MPC Live 1 = Gen1 ARM32 (armhf). Le build doit utiliser la toolchain OpenMPC actuelle. Le build distant via GitHub Actions est envisagé afin de ne pas nécessiter Docker sur le Mac.

## Références

- https://open-mpc.github.io/build.html
- https://github.com/open-mpc/mpc-vst-plugins
