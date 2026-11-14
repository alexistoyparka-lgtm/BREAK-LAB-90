# BREAK LAB 90 0.2.1 — pads, dix breaks et nouvelle interface

L'utilisateur a confirmé que la 0.2.0 USB du build 107 apparaît, s'ouvre et produit du
son sur sa MPC Live Gen1. L'ordre physique des slices est incorrect dans sa
configuration chromatique. Cela ne valide pas encore toutes les autres fonctions.

## Remettre les slices dans l'ordre des pads

MPC transmet les **notes du mode Pad Perform**, pas le numéro physique du pad.
La note de départ et l'intervalle entre les rangées peuvent varier selon la piste.

1. Garder la piste MPC en mode **Chromatic**.
2. Ouvrir la page **PADS** du plugin et activer **LEARN**.
3. Quand le plugin demande `HIT PAD 1`, frapper le pad 1 physique, en bas à gauche.
4. Quand il demande `HIT PAD 5`, frapper le pad 5 physique, juste au-dessus.
5. Le plugin règle la note de départ et l'espacement des rangées. Jouer ensuite les
   16 pads : slices 1–4 de gauche à droite en bas, puis 5–8, 9–12 et 13–16 en haut.

Les deux frappes d'apprentissage servent au calibrage et ne jouent pas de slice.
En cas de `BAD ROW; RETRY`, relancer LEARN et frapper1 puis 5, pas 1 puis 2.
Les options manuelles de disposition/note/rangée restent disponibles.
Le mode Legacy conserve les notes historiques36–51 pour les anciens projets.
Le changement de preset ne remet pas le calibrage à zéro.

## Banque de breaks

Les cinq presets historiques gardent leurs positions. Dix presets de breaks sont
ajoutés, ainsi qu'un sélecteur de break utilisable sans changer le caractère sonore.
Les dix WAV ont été sélectionnés dans les 17 fichiers de `Archive.zip`, en privilégiant
la longueur exploitable, les attaques et une variété de profils sonores.
Les noms sont créatifs : ils ne prétendent pas identifier les disques d'origine.
Les fichiers WAV sont renommés sans normalisation ni conversion des données.

| Preset | Fichier fourni |
|---|---|
| Warehouse Run | 22 Track 22_001.wav |
| Chrome Break | 13 Track 13_002.wav |
| Redline Break | 13 Track 13_004.wav |
| Neon Shuffle | 13 Track 13_005.wav |
| Dust Circuit | 13 Track 13_006.wav |
| Razor Step | 13 Track 13_008.wav |
| Basement Snap | 13 Track 13_011.wav |
| Ghost Runner | 13 Track 13_013.wav |
| Concrete Swing | 19 Track 19_006.wav |
| Subway Funk | 19 Track 19_009.wav |

`engine/breaks/bank.json` conserve les noms d'origine, formats et SHA-256.
Les samples et leur découpage initial sont préparés au chargement de l'instrument ;
le changement de break n'effectue ni lecture disque ni analyse de transitoires.
Le budget de mémoire audio décodée de toute la banque est limité à64 Mio.

## Visuel

La nouvelle skin s'inspire des samplers et trackers Amiga : graphite, relief gris,
accents rouges et typographie rétro, tout en conservant des contrôles tactiles.
Les waveforms sont calculées à partir des vrais WAV et suivent le break sélectionné.
Ce sont des aperçus du fichier, pas une tête de lecture animée ni un éditeur tactile
libre des marqueurs. START/END et les Q-Links restent les outils d'édition.

## Installation et essai

Suivre INSTALL_USB_FR.md et utiliser le ZIP **mpc-sdk** 0.2.1. Conserver une copie du
dossier 0.2.0 fonctionnel hors de VST2 Plugins ; ne pas fusionner les deux dossiers.

Dans un nouveau projet vide : calibrer les pads, jouer 1–16, essayer les dix presets,
vérifier que waveform et sample changent ensemble, puis sauvegarder/réouvrir et
vérifier le break, le calibrage et les bornes éditées. Tester les quatre pages et les
Q-Links puis une séquence dix minutes. La0.2.1 reste à valider sur la MPC physique.
