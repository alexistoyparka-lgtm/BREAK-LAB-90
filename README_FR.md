# BREAK LAB 90 — sampler Amiga pour MPC Live Gen1

**Version 0.2.1 : candidat USB ARMv7/armhf pour MPC OS3.10.1.**

La version 0.2.0 USB du build 107 a été ouverte sur la MPC physique de l'utilisateur
et produit du son. L'ordre des pads de sa piste chromatique nécessitait une
correction. La0.2.1 ajoute le calibrage des pads, dix breaks et une nouvelle skin ;
cette nouvelle version attend encore son essai matériel.

## Installer et jouer

Télécharger l'artefact `break-lab-90-v0.2.1-usb-candidate` d'un build GitHub Actions
réussi. Extraire ensuite **BREAK-LAB-90-0.2.1-mpc-sdk.zip**, puis copier son unique
dossier dans `VST2 Plugins` à la racine de la clé USB.

Conserver l'ancien dossier fonctionnel en sauvegarde hors de VST2 Plugins.
Ne pas fusionner les versions. La copie USB n'exécute aucun script et ne nécessite
aucune modification manuelle de MPC.settings.

- [Procédure USB détaillée et retour arrière](docs/INSTALL_USB_FR.md)
- [Calibrage des pads, banque de breaks et essai0.2.1](docs/PADS_BANK_RETRO_FR.md)
- [Audit du problème de détection0.2.0](docs/USB_PACKAGING_AUDIT.md)

## Pads dans l'ordre

Sur la page **PADS**, appuyer sur **LEARN PAD 1 + 5**, puis frapper le pad 1 physique
(en bas à gauche) et le pad 5 (juste au-dessus). Garder la piste MPC en Chromatic.
Le plugin apprend la note de départ et l'écart des rangées. Les slices suivent
alors1–4 en bas, 5–8, 9–12, 13–16 en haut, de gauche à droite.

Le mode Legacy 36–51 reste disponible pour les anciennes séquences MIDI.
Les réglages de pads sont conservés quand on change de preset.

## Dix breaks supplémentaires

Les cinq presets historiques gardent leurs positions : Clean Break, Dusty12-Bit,
Dark Warehouse, Reverse Texture et Paula Jungle. Les dix nouveaux presets chargent
Warehouse Run, Chrome Break, Redline Break, Neon Shuffle, Dust Circuit, Razor Step,
Basement Snap, Ghost Runner, Concrete Swing et Subway Funk.

Le sélecteur SAMPLE MEMORY change le break sans remplacer les réglages de son.
Les noms sont créatifs ; les sources et empreintes sont dans `engine/breaks/bank.json`.
Les WAV stéréo44,1 kHz/16 bits ont été renommés sans modifier leurs données.
Les onze fichiers et leurs découpages sont préparés au chargement, sous un budget
combiné de64 Mio décodés. Aucun accès disque n'est effectué au changement de break.

## Moteur et interface

- 16 slices,8 voix, vélocité, pitch±24 demi-tons, reverse et fondus anti-clic.
- Détection heuristique des transitoires, avec repli sur16 divisions égales.
- Édition START/END protégée et Undo d'une modification.
- CLEAN, VINTAGE et PAULA, quantification et sample-and-hold.
- Paula Hold, Linear et Cubic ; PAULA reste une approximation créative.
- Filtre passe-bas et volume général.
- Quatre pages tactiles PLAY, SLICE, CHARACTER et PADS avec Q-Links.
- Châssis graphite/gris, typographie rétro et waveform rouge du fichier sélectionné.
- Sauvegarde d'état versionnée du break, des réglages, du calibrage et des bornes.

Les waveforms sont des aperçus statiques des vrais fichiers, sans tête de lecture
animée ni édition tactile directe. Le chargement libre de WAV depuis l'écran,
le time-stretch et le resampling interne ne sont pas encore implémentés.

## Validation

Les tests portent sur le moteur et le wrapper sous ASan/UBSan, l'ordre audio des
16 slices, le calibrage et ses erreurs, les changements de samples en RAM, la
restauration d'état, les fichiers de banque et les deux formats de ZIP.

L'UID VST reste `42726b39`, les paramètres historiques ne sont pas réordonnés.
La compilation ARMv7 et les tests logiciels ne remplacent pas la validation de
la 0.2.1 sur MPC Live Gen1. CPU, longues sessions et persistance MPC restent à mesurer.
