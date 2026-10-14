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

## Effets audio

Chaque tranche du mixer (piste audio ou MIDI / Instrument) possede un bouton FX. Il ouvre un panneau d'effets pour cette piste. Le bouton FX est colore des qu'au moins un effet est actif.

Les six effets sont chaines dans cet ordre, avant le fader et le panoramique :

| Effet | Reglages |
|---|---|
| Filtre | passe-bas / passe-haut, frequence, resonance |
| Distorsion | gain, mix |
| Chorus | vitesse, profondeur, mix |
| Delay | temps (1 a 2000 ms), feedback, mix |
| Echo | temps, feedback, tonalite, mix (repetitions ping-pong qui s'assombrissent) |
| Reverb | taille, amorti, mix |

Double-clic sur un bouton rotatif pour revenir a sa valeur par defaut.

Les effets sont enregistres dans le projet (`effects` par piste). Les projets plus anciens se chargent sans effet actif. Les queues de reverb et de delay continuent de sonner apres la fin d'une note ou pendant la pause, et sont coupees par Mute / Solo.

Limites de cette premiere version : pas d'effet sur le Master, pas de synchronisation des delays au BPM, distorsion sans suréchantillonnage, et les reglages d'effets ne sont pas dans l'historique Annuler / Retablir (comme les faders du mixer).

## Fichiers de projet

Les projets sont enregistres avec l'extension courte **.sfs**. Les sauvegardes automatiques et de secours suivent le meme schema : `nom.autosave.sfs` et `nom.backup.sfs`.

Les projets crees avec une version precedente (`.sonoforge`) s'ouvrent normalement et sont enregistres sous leur nom d'origine. Pour les convertir, utiliser Fichier > Enregistrer sous... : le nom propose est le meme avec l'extension .sfs, et l'ancien fichier n'est pas modifie. Une sauvegarde automatique `.autosave.sonoforge` laissee par une ancienne version est reprise automatiquement pour la recuperation.
## Edition des clips au clic droit

Un clic droit sur un clip audio ouvre un menu :

- Couper ici / Couper au curseur de lecture
- Raccourcir le debut ici / la fin ici (position de la souris)
- Raccourcir le debut / la fin au curseur de lecture
- Dupliquer, Muet, Solo, Supprimer

La position de la souris suit la grille quand Snap Grid est actif. Chaque coupe ou rognage est annulable.

## Export du mixage

Fichier > Exporter le mixage... (ou le bouton Exporter) rend le mixage complet dans un fichier :

- formats : WAV 16 / 24 bits, WAV 32 bits flottant, FLAC 16 / 24 bits
- taux d'echantillonnage : 44100, 48000 ou 96000 Hz
- normalisation optionnelle a -1 dBFS

Le rendu est fait hors ligne, plus vite que le temps reel, sans toucher a la lecture. Il applique les memes effets, volumes, pans, Mute / Solo et gain Master que la lecture. Une queue de 3 secondes est ajoutee quand un effet est actif, pour laisser sonner reverb et delay.

Si le signal depasse 0 dBFS, l'export le signale (echantillons ecretes). Annuler pendant le rendu ne touche pas un fichier existant du meme nom.

Limite : les pistes MIDI / Instrument ne sont pas exportees, car elles n'ont pas encore de clips (le piano roll arrive plus tard).

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
- sauvegarde de projet .sfs (les anciens .sonoforge restent ouvrables)
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
