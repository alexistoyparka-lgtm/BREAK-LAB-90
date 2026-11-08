# BREAK LAB 90 — prototype OpenMPC Gen1

<badge></badge>
**État : prototype v0.1.1 compilé avec succès en ARMv7 et tests hôte OpenMPC réussis (AddressSanitizer + UBSan). Pas encore testé sur une MPC réelle.**

La compilation, les tests du wrapper et la génération du paquet passent sur GitHub Actions. Cela confirme la production d'un binaire ARM 32 bits, mais ne garantit pas encore que le plugin se charge et fonctionne correctement sur une MPC Live 1 sous OS 3.10. **Ne pas installer cette version sur une MPC de production.**

## Fonctions présentes

- Instrument VST2 avec 16 slices déclenchées par MIDI : notes 36 à 51.
- Lecture WAV PCM 16-bit mono ou stéréo.
- Pitch de -24 à +24 demi-tons.
- Reverse, filtre passe-bas et volume.
- Modes CLEAN et VINTAGE (réduction de profondeur de bits et de fréquence d'échantillonnage).
- 8 voix de lecture simultanées.
- Un break original est fourni dans le paquet.

## Limites et prochaines étapes

- Un seul WAV : `engine/breaks/break01.wav`.
- Format : PCM 16-bit, mono ou stéréo ; 44,1 kHz recommandé.
- Découpage égal en 16 tranches ; pas encore de détection de transitoires ni d'édition des points de coupe.
- Pas encore de mesure CPU ni de test de chargement sur la MPC réelle.
- Import WAV depuis l'écran MPC, banques multiples, resampling, time-stretch indépendant du pitch et workflow tactile personnalisé restent à développer.
- L'installation non officielle nécessite un accès shell root et modifie la liste des plugins MPC. Sauvegarde indispensable.

## Cible et validation

- MPC Live 1 = Gen1 ARM32 (armhf), OS 3.10 visé.
- CI : compilation ARMv7, validation du paquet et tests hôte avec ASan/UBSan — **réussis**.
- Matériel réel : **pas encore testé**. Le chargement, l'interface, les Q-Links, la sauvegarde de projet et les performances restent à valider sur la MPC Live 1.

## Références

- https://open-mpc.github.io/build.html
- https://github.com/open-mpc/mpc-vst-plugins
