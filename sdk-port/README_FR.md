# BREAK LAB 90 — SDK 0.3.0 TEST 1

Statut : port natif expérimental. Sauvegarder ses projets avant tout test.

## Installation — MPC Live Gen1 / OS 3.10.1 ou ultérieur
1. Sauvegarder les projets et une copie du plugin actuellement fonctionnel.
2. Retirer l'ancien dossier BREAK LAB 90 de VST2 Plugins avant le test. Deux versions partageant le même UID Brk9 ne doivent pas cohabiter.
3. Décompresser l'archive GitHub Actions.
4. Copier le dossier OpenBench - VST - BREAK LAB 90 dans la racine de la clé USB, sous VST2 Plugins.
5. Sur MPC : Preferences > Enhancements > Enable Enhancements et External Plugin Support.
6. Dans un NOUVEAU projet, ajouter un instrument External Plugins > BREAK LAB 90.
7. Tester les pads 1 à 16, reverse, Clean HQ Sinc, Vintage, PAULA, Q-Links, sauvegarde et réouverture.
8. Vérifier les craquements et la stabilité avec huit voix pendant dix minutes.
9. En cas de problème, retirer le nouveau dossier, restaurer l'ancien plugin et redémarrer.

## Contenu
ARM32 et ARM64 ; échantillon de démonstration WAV ; pages PLAY, CHOP et CHAR ; 16 slices ; 8 voix ; HQ Sinc.
Identifiant Brk9 et indices 0-12 maintenus pour réduire les incompatibilités de projet.

## Limitations
- Le moteur reste un bridge int16 -> float32 ; port DSP full-float à venir.
- Pas encore de time-stretch indépendant du pitch.
- Les marqueurs de slices ne sont pas encore déplaçables librement au doigt.
- Pas encore de navigateur de samples.
- L'état des anciens projets OpenMPC doit être testé avant toute migration de production.
- Les tests CI ne remplacent pas les tests sur le matériel physique.

Code : https://github.com/stormzi1312/BREAK-LAB-90/pull/5
SDK : https://github.com/inmusic-software/mpc-plugin-builder-sdk
