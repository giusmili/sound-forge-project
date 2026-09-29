# SonoForge Studio Desktop 0.3.2

Cette iteration transforme le snapping temporel en premiere grille musicale.

## Nouveautes

- BPM reglable de 40 a 240
- grille visuelle basee sur les temps
- snapping des clips sur les temps
- recalcul immediat de la grille lors du changement de BPM
- grille coherente avec le zoom horizontal
- Snap Beat activable/desactivable

A 120 BPM, un temps correspond a 0,5 seconde :

```text
60 / 120 = 0.5 s
```

A 90 BPM :

```text
60 / 90 = 0.666... s
```

## Edition

Le drag d'un clip utilise maintenant :

```text
position brute
    |
    v
intervalle = 60 / BPM
    |
    v
position quantifiee sur le temps
```

La grille reste visible meme lorsque le snapping est desactive, afin de conserver un repere musical.

## Fonctions conservees

- transport global
- offsets reels des clips
- Play / Pause / Stop
- seek
- waveforms
- zoom 1x a 8x
- navigation temporelle
- volume / pan / Mute / Solo
- Master
- configuration audio

## Etapes suivantes

- subdivisions 1/4, 1/8, 1/16
- mesures en 4/4
- selection des clips
- duplication
- decoupage non destructif
- sauvegarde de projet
- undo / redo
- enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
