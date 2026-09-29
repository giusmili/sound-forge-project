# SonoForge Studio Desktop 0.8

Cette iteration ajoute un mixer leger integre tout en conservant les fonctions d'ergonomie, de pistes audio et de pistes MIDI / Instrument de la version precedente.

## Mixer leger

Le bouton Mixer affiche ou masque une console compacte en bas de la fenetre.

Chaque piste audio dispose d'une tranche avec :

- nom de piste
- vumetre temps reel
- fader de volume vertical
- panoramique
- Mute
- Solo

Chaque piste MIDI / Instrument dispose d'une tranche avec :

- nom de piste
- vumetre temps reel
- fader de volume vertical
- Mute
- indication Basic Synth

Une tranche Master affiche le niveau de sortie et controle le gain general.

Les reglages du mixer et ceux des lignes de pistes pilotent les memes objets audio. Une modification faite dans le mixer est donc immediatement appliquee au moteur audio et synchronisee avec l'interface principale.

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
appVersion = 0.8.0
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
- mixer leger escamotable avec vumetres
- Master avec vumetre
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
