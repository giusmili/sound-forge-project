# SonoForge Studio Desktop 0.7

Cette iteration integre les premiers retours de test utilisateur sur l'ergonomie generale et les types de pistes.

## Fermeture et menus

SonoForge dispose maintenant d'une barre de menus standard :

```text
Fichier
  Ouvrir projet...
  Enregistrer
  Importer audio...
  Quitter

Edition
  Annuler
  Retablir

Piste
  Ajouter piste audio...
  Ajouter piste MIDI / Instrument
  Configuration audio...
```

Le bouton Quitter est egalement visible dans l'interface.

Toutes les voies de fermeture utilisent la fermeture securisee :

- bouton X de Windows
- menu Fichier > Quitter
- bouton Quitter
- demande de fermeture du systeme

Si le projet contient des changements non enregistres, SonoForge propose Enregistrer, Ne pas enregistrer ou Annuler.

## Ajout de pistes

Deux acces directs sont disponibles :

```text
+ Audio
+ MIDI
```

### Piste Audio

+ Audio ouvre le selecteur de fichiers audio et ajoute les fichiers choisis comme pistes audio.

L'enregistrement microphone/interface reste disponible avec Record.

### Piste MIDI / Instrument

+ MIDI cree une vraie piste instrument dans le moteur audio.

La premiere implementation contient :

- nom de piste Instrument N
- instrument interne Basic Synth
- volume
- Mute
- bouton Tester C4
- suppression individuelle
- mixage dans le Master
- sauvegarde/restauration dans le projet

Le bouton Tester C4 permet de verifier immediatement que la piste instrument produit du son.

Le piano roll, les clips MIDI, l'entree clavier MIDI et les VST3 seront ajoutes dans les iterations suivantes.

## Format projet

Les nouveaux projets utilisent :

```text
SonoForgeProject
formatVersion = 2
appVersion = 0.7.0
```

Le formatVersion 2 ajoute les pistes MIDI / Instrument.

Les projets formatVersion 1 restent compatibles en lecture.

## Fonctions deja disponibles

- lecture multipiste audio
- enregistrement audio natif
- Play / Pause / Stop
- fermeture securisee
- projets recents
- autosave / backup / recovery
- sauvegarde .sonoforge
- Undo / Redo
- Split / Trim
- duplication / suppression
- grille 1/4, 1/8, 1/16
- BPM / snapping
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master
- piste MIDI / Instrument avec Basic Synth

## Prochaine iteration MIDI

- clips MIDI
- piano roll
- creation et edition de notes
- clavier MIDI externe
- choix d'instrument
- puis hebergement VST3

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
