# BREAK LAB 90 — candidat de test MPC Live Gen1

**Version de développement : 0.2.0-rc1 — candidat de validation ARMv7 et MPC Live Gen1.**

BREAK LAB 90 est un instrument sampler de breaks pour OpenMPC, ciblant la MPC Live de première génération (ARM 32 bits / armhf). MPC OS 3.10 est la version visée.

## État de validation

- Compilation ARMv7 / armhf : réussie dans GitHub Actions, run **#72**.
- Tests du wrapper OpenMPC : réussis dans le run #72.
- Tests moteur WAV/MIDI/audio sous ASan + UBSan : réussis dans le run #72.
- Paquet généré : ZIP MPC ARMv7, binaire ELF32 ARM hard-float.
- Contrôles du paquet : sommes SHA-256 valides, scripts installateur/désinstallateur syntaxiquement valides.
- Essai à blanc de l'installation puis de la désinstallation avec un faux MPC.settings : réussi.
- Interface : aperçu de la skin générée inspecté.
- MPC Live 1 physique / OS 3.10 : **pas encore testé**.

Le paquet est donc prêt pour un **premier essai contrôlé sur une MPC de test sauvegardée**. Cela ne constitue pas une certification matérielle et il ne faut pas l'utiliser sur une MPC contenant un projet important sans sauvegarde complète.

## Fonctions disponibles

- Lecture d'un break WAV PCM 16 bits, mono ou stéréo.
- 16 slices MIDI (notes 36 à 51), avec détection de transitoires à l'ouverture du sample quand au moins 15 attaques fiables sont détectées ; sinon, repli sur 16 divisions égales.
- Note MIDI 60 (C4) : audition de la slice sélectionnée.
- Pitch de -24 à +24 demi-tons.
- Lecture Forward / Reverse.
- Modes Clean et Vintage / Paula-style.
- Trois modes de lecture : Paula Hold (sans interpolation), Linear et Cubic.
- Réduction de profondeur de bits et Rate Crush en mode Vintage.
- Filtre passe-bas et volume général.
- 8 voix simultanées avec vol de voix en cas de surcharge.
- Quatre presets : Clean Break, Dusty 12-Bit, Dark Warehouse et Reverse Texture.
- Un break original est fourni.

## Format audio

Le fichier chargé au démarrage est :

    engine/breaks/break01.wav

Formats acceptés : WAV RIFF, PCM linéaire 16 bits, mono ou stéréo, 8 à 192 kHz. 44,1 kHz est recommandé. La mémoire audio décodée est limitée à environ 64 Mio.

Pour remplacer le break, sauvegarder le WAV fourni puis remplacer break01.wav par son propre fichier portant exactement ce nom. Il faut ensuite fermer et rouvrir le plugin. Il n'existe pas encore de navigateur/import WAV à l'écran.

## Installation

L'installation est non officielle et nécessite un accès shell **root** à la MPC. Le script :

1. vérifie l'architecture et les fichiers du paquet ;
2. demande confirmation et arrête MPC ;
3. sauvegarde MPC.settings ;
4. installe le dossier complet du plugin sous /sdcard/Synths ;
5. ajoute l'entrée du plugin à la liste MPC ;
6. redémarre MPC.

Faire une sauvegarde de MPC.settings, des projets importants et de tout dossier plugin existant avant l'installation. Ne jamais interrompre le script pendant la modification des fichiers. La désinstallation est fournie et conserve le dossier de données utilisateur.

Voir le guide d'installation et de test MPC Live 1 (docs/MPC_LIVE_1_TEST.md) et la checklist de validation (TESTING.md).

## Limites connues

- Un seul WAV chargé à la fois.
- Les points de coupe ne sont pas encore éditables individuellement depuis l'écran.
- La détection automatique est une première version heuristique, pas encore un détecteur kick/snare spécialisé.
- Pas de déplacement manuel des points de coupe.
- Pas de time-stretch indépendant du pitch ni de resampling.
- Pas encore de benchmark CPU ni de validation réelle sur MPC Live 1.

## Conclusion

**La branche 0.2.0-rc1 ajoute le premier slicing transitoire et trois modes d'interpolation. Le build ARMv7 et les tests doivent encore passer avant de proposer un paquet à installer.** Le chargement du plugin, l'affichage tactile, les Q-Links, la lecture audio, la sauvegarde/réouverture de projet, le redémarrage et la stabilité 10 minutes restent à confirmer sur la machine physique.

Le break fourni est original. L'utilisateur est responsable des droits sur les fichiers audio qu'il importe.
