# SonoForge Studio Desktop 0.3

Cette etape introduit un changement d'architecture important : la position du projet n'est plus deduite des transports individuels. SonoForge possede maintenant une horloge globale de projet.

## Nouveautes 0.3

- transport global independant des pistes
- position de projet partagee
- offset de debut propre a chaque piste
- longueur du projet calculee avec les offsets
- deplacement horizontal d'un clip a la souris
- la lecture respecte reellement la nouvelle position du clip
- le clip affiche son offset en secondes
- Play, Pause, Stop et seek restent synchronises
- Mute, Solo, volume, pan et Master sont conserves
- waveforms et playhead restent synchronises

## Utilisation

Importer plusieurs pistes puis faire glisser directement une forme d'onde vers la droite. Le debut du fichier audio se deplace sur la timeline et la lecture attend cette position avant de demarrer la piste.

## Architecture

```text
Project clock
    |
    +---- projectPosition
    +---- playing
    |
    v
AudioEngine
    |
    +-- AudioTrack A  startOffset = 0 s
    +-- AudioTrack B  startOffset = 8 s
    +-- AudioTrack C  startOffset = 14 s
```

Chaque AudioTrack convertit la position globale du projet en position locale :

```text
localPosition = projectPosition - startOffset
```

Avant le debut du clip, la piste reste silencieuse. Lorsque l'horloge atteint son offset, le transport local commence au debut du fichier.

## Limite actuelle

Le declenchement est aligne sur les blocs audio. Une future etape rendra le placement sample-accurate pour les usages professionnels exigeants.

## Prochaines etapes

- snapping sur une grille
- zoom horizontal
- selection de clips
- duplication
- decoupage non destructif
- sauvegarde du projet
- undo / redo
- enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
