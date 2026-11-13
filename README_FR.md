# BREAK LAB 90 — candidat MPC Live Gen1

**Version de développement : 0.1.6 — paquet USB MPC OS 3.10.1 en préparation.**

BREAK LAB 90 est un sampler breakbeat pour MPC de première génération (ARMv7 / armhf).

## Compatibilité visée

La cible est la MPC Live Gen1 sous MPC OS 3.10.1 ou ultérieur, avec External Plugin Support activé. Le workflow mis à jour utilise l'outil de packaging OpenMPC actuel pour produire, en plus du ZIP ARMv7 historique, un ZIP au format dossier SDK destiné au répertoire VST2 Plugins.

Cette évolution vise une installation par clé USB, sans SSH, sans root et sans modification manuelle de MPC.settings. Elle n'est pas encore validée sur une MPC physique.

## Installation prévue sur MPC OS 3.10.1+

1. Activer Menu → Preferences → Enhancements → Enable Enhancements et External Plugin Support, puis redémarrer.
2. Lancer le workflow Build BREAK LAB 90 sur la branche compat/mpc-os-3.10.1-usb-package.
3. Télécharger l'artefact du build réussi et extraire uniquement le fichier *-mpc-sdk.zip.
4. Copier le dossier contenant plugin.json directement dans VST2 Plugins à la racine d'une clé USB.
5. Laisser la clé branchée, créer une piste Plugin et chercher External VST Plugins → OpenBench → BREAK LAB 90.

Ne pas utiliser l'ancien ZIP ARMv7 comme paquet USB direct. Voir docs/MPC_LIVE_1_TEST.md.

## Fonctions du prototype

- 16 slices déclenchées par MIDI notes 36 à 51 ; audition de la slice sélectionnée par MIDI C4 / note 60.
- Lecture WAV PCM 16 bits mono/stéréo.
- Pitch de -24 à +24 demi-tons et lecture Forward / Reverse.
- Modes Clean et Vintage, filtre passe-bas, Bitcrush et Rate Crush.
- Huit voix simultanées et quatre presets d'usine.

## Validation

- Compilation ARMv7 et tests hôte : résultats historiques dans GitHub Actions.
- Nouveau paquet MPC OS 3.10.1 : à confirmer par une nouvelle exécution du workflow.
- Détection, affichage, audio, MIDI, sauvegarde/réouverture et stabilité sur MPC Live Gen1 : pas encore testés sur matériel physique.
- Benchmark CPU sur MPC : non mesuré.

Aucune compatibilité à 100 % ne sera revendiquée avant validation de la checklist matérielle complète.

## Limites fonctionnelles

Un seul WAV chargé, 16 divisions égales, pas encore de navigateur/import WAV, de points de coupe éditables, de détection de transitoires ni de time-stretch préservant la hauteur.

Le break fourni est original. L'utilisateur est responsable des droits sur ses propres fichiers audio.