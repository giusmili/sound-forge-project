# SonoForge Studio Desktop 0.4.3

Cette iteration ajoute un historique Undo / Redo base sur des snapshots de projet.

## Undo / Redo

Les boutons Annuler et Retablir permettent de revenir sur les principales operations d'edition :

- import audio
- deplacement d'un clip
- trim gauche / droite
- duplication
- suppression
- Split au playhead

L'historique conserve jusqu'a 50 etats.

## Snapshot de projet

Chaque entree memorise :

```text
tracks[]
    sourceFile
    sourceStart
    sourceEnd
    startOffset
    gain
    pan
    mute
    solo

position du playhead
BPM
grille
snap
zoom
vue
master
```

Le moteur peut reconstruire les pistes depuis ce snapshot. Cette architecture servira directement a la prochaine etape : sauvegarder et rouvrir un projet SonoForge.

## Edition continue

Lors d'un deplacement ou d'un trim, SonoForge n'ajoute pas une entree d'historique pour chaque pixel. Un seul snapshot est cree au debut du geste.

## Fonctions conservees

- transport global
- clips deplacables
- selection / duplication / suppression
- Split au playhead
- trim non destructif
- grille 1/4, 1/8, 1/16
- BPM et snapping
- Play / Pause / Stop
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

- sauvegarde d'un projet .sonoforge
- ouverture d'un projet
- detection des fichiers audio manquants
- autosave
- puis enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
