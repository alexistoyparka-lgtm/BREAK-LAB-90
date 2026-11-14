# BREAK LAB 90 0.2.1 — candidat USB MPC Live Gen1

Cible : MPC Live première génération, OS 3.10.1, ARMv7/armhf.
La découverte et le fonctionnement de ce candidat restent à confirmer sur la MPC physique.

## Téléchargement

Dans GitHub Actions, ouvrir le build réussi de la branche `feat/mpc-pads-break-bank-retro`,
puis télécharger l'artefact **break-lab-90-v0.2.1-usb-candidate**.
Il contient deux ZIP et `SHA256SUMS`. Pour la copie USB, utiliser uniquement
**BREAK-LAB-90-0.2.1-mpc-sdk.zip**. Le ZIP `mpc-armv7` est destiné au Plugin Manager.
Vérifier le SHA-256 du ZIP intérieur avec le fichier fourni.

## Copie USB

1. Sauvegarder le projet en cours et fermer BREAK LAB 90.
2. Conserver une copie de l'ancien dossier du plugin sur l'ordinateur, hors du
   dossier `VST2 Plugins`. Ne pas fusionner les dossiers 0.1.6 et 0.2.1.
   Pour un premier essai, une autre clé USB permet de garder la clé d'origine intacte.
3. Extraire le ZIP `mpc-sdk`. Copier son unique dossier directement dans
   `VST2 Plugins` à la racine de la clé, avec cette structure exacte :

```text
VST2 Plugins/
└── OpenBench - VST - BREAK LAB 90/
    ├── plugin.json
    ├── break_lab_90-arm32.so
    ├── Plugin Skins/
    ├── version.xml
    └── engine/breaks/break01.wav
```

Ne pas copier le ZIP lui-même, le dossier `portable`, ni un dossier contenant
un deuxième niveau `OpenBench - VST - BREAK LAB 90`.

4. Dans Menu → Preferences → Enhancements, vérifier que les options
   Enable Enhancements et External Plugin Support utilisées pour la 0.1.6 sont
   activées. Si elles viennent d'être activées, attendre leur sauvegarde et redémarrer.
5. Brancher uniquement la clé portant la version testée pour éviter deux plugins
   de même UID. Créer un projet vide ; chercher External VST Plugins → OpenBench
   → BREAK LAB 90. Laisser la clé branchée pendant l'utilisation.

Cette méthode n'exécute aucun script et ne modifie pas manuellement `MPC.settings`.
Si le plugin reste absent, ne pas modifier ce fichier : relever le firmware exact,
le support, l'arborescence et le contenu de `plugin.json`.

## Essai physique court

- Vérifier l'apparition, l'ouverture et le statut du sample READY.
- Déclencher les notes MIDI 36 à 51, puis l'audition sur 60.
- Calibrer les pads comme indiqué dans PADS_BANK_RETRO_FR.md.
- Vérifier les quatre pages, les Q-Links, START/END et Undo.
- Essayer CLEAN, VINTAGE, PAULA, Hold/Linear/Cubic et les cinq presets historiques et les dix nouveaux breaks.
- Sauvegarder dans un nouveau projet de test, fermer puis rouvrir ; vérifier les valeurs.
- Jouer dix minutes avec plusieurs voix, puis redémarrer et vérifier le rechargement.

En cas de problème, fermer le projet et remettre la clé 0.1.6 ou restaurer son
ancien dossier sauvegardé. Ne supprimer aucun sample ni projet utilisateur.
La réussite de cet essai ne constitue pas encore un benchmark CPU ni un test longue durée.

## Autre méthode : Plugin Manager

Conserver `BREAK-LAB-90-0.2.1-mpc-armv7.zip` intact et le placer dans le dossier
`local-plugins` du Plugin Manager suffisamment récent, selon sa documentation.
Cette installation utilise son mécanisme d'enregistrement ; sauvegarder les réglages
et le dossier du plugin avant toute mise à jour par cette voie. Ne pas installer
simultanément une copie gérée et une copie USB de même UID pour le premier essai.
