# SonoForge Studio Desktop 0.4

La version 0.4 introduit les premieres operations d'edition de clips.

## Nouveautes

- selection visuelle d'un clip
- commandes Dupliquer et Supprimer
- grille musicale 1/4, 1/8 et 1/16
- snapping adapte a la subdivision choisie
- duplication avec conservation du fichier source, du gain, du pan, du Mute, du Solo et de la plage source
- decalage automatique du duplicata d'une cellule de grille
- suppression propre de la piste et de sa source dans le mixer
- preparation du decoupage non destructif

## Modele non destructif

Le fichier audio original n'est jamais modifie.

Chaque clip possede maintenant :

```text
sourceFile
sourceStart
sourceEnd
startOffset
gain
pan
mute
solo
```

La duree visible et lue du clip est :

```text
clipDuration = sourceEnd - sourceStart
```

La position de fin dans le projet est :

```text
projectEnd = startOffset + clipDuration
```

Cette structure permet d'ajouter ensuite Split, Trim et redimensionnement sans reecrire les fichiers audio.

## Grille musicale

Pour un BPM donne :

```text
1/4  = 60 / BPM
1/8  = (60 / BPM) / 2
1/16 = (60 / BPM) / 4
```

Exemple a 120 BPM :

```text
1/4  = 0.500 s
1/8  = 0.250 s
1/16 = 0.125 s
```

## Fonctions conservees

- transport global
- clips deplacables
- Play / Pause / Stop
- seek
- waveforms
- zoom 1x a 8x
- navigation temporelle
- BPM 40 a 240
- volume / pan / Mute / Solo
- Master
- configuration audio

## Prochaine iteration

- Split au playhead
- trim gauche / droite
- selection multiple
- undo / redo
- sauvegarde de projet
- enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
