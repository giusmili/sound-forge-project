# SonoForge Studio Desktop 0.3.1

Cette iteration ajoute les premiers outils d'edition temporelle au transport global de SonoForge.

## Nouveautes

- snapping des clips a la seconde
- activation/desactivation du snapping
- zoom horizontal de 1x a 8x
- navigation dans la portion visible de la timeline
- regle temporelle adaptee a la fenetre de zoom
- seek relatif a la portion visible
- waveforms conservees a la bonne echelle pendant le zoom
- deplacement des clips calcule dans l'echelle temporelle visible

## Commandes

- Snap 1 s : active ou desactive l'aimantation sur les secondes entieres
- Zoom : agrandit la timeline horizontalement
- Vue : deplace la fenetre temporelle lorsque le zoom est superieur a 1x

## Moteur audio

Le moteur 0.3 reste inchange :

```text
projectPosition -> AudioEngine -> AudioTrack
                                  startOffset
```

Le zoom et le snapping sont des fonctions d'edition et n'alterent pas le moteur de lecture.

## Limites actuelles

- snapping fixe a 1 seconde
- pas encore de grille musicale basee sur BPM
- pas encore de selection multiple
- pas encore de duplication ou decoupage
- scheduling audio toujours aligne sur les blocs

## Prochaines etapes

- grille BPM / mesures / temps
- selection d'un clip
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
