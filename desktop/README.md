# SonoForge Studio Desktop 0.4.2

Cette iteration ajoute le trim non destructif gauche/droite en plus du Split au playhead.\n\n## Trim non destructif\n\nSelectionnez un clip puis faites glisser son bord gauche ou droit. Les poignees jaunes indiquent les zones de trim. Le curseur change pour signaler le redimensionnement.\n\nLe bord gauche modifie simultanement sourceStart et startOffset afin de garder la fin du clip en place. Le bord droit modifie sourceEnd. Le snapping 1/4, 1/8 ou 1/16 reste applicable pendant le trim.\n\nLe fichier audio original n'est jamais modifie.

## Split au playhead

Procedure :

1. selectionner un clip
2. placer le playhead a l'interieur du clip
3. cliquer sur Couper

SonoForge cree alors deux clips independants qui referencent toujours le meme fichier audio original.

Avant :

```text
sourceStart ------------------------------ sourceEnd
|                 CLIP                          |
|-----------------------------------------------|
```

Apres une coupe :

```text
sourceStart ------- split ------- sourceEnd
|      CLIP A      |    CLIP B        |
|------------------|------------------|
```

Le clip A conserve le meme startOffset.

Le clip B commence exactement a la position du playhead dans le projet.

## Donnees non destructives

Aucun fichier audio n'est reecrit.

Pour le clip gauche :

```text
sourceStart = ancien sourceStart
sourceEnd   = split
```

Pour le clip droit :

```text
sourceStart = split
sourceEnd   = ancien sourceEnd
startOffset = position du playhead
```

Les reglages gain, pan, Mute et Solo sont recopies sur le clip droit.

## Protection

La commande Couper n'est active que lorsque le playhead se trouve reellement a l'interieur du clip selectionne. Une marge minimale de 10 ms evite la creation accidentelle de fragments quasi nuls.

## Fonctions conservees

- transport global
- clips deplacables
- selection / duplication / suppression
- grille 1/4, 1/8, 1/16
- BPM et snapping
- Play / Pause / Stop
- seek
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

- trim gauche et droite par glisser
- undo / redo
- sauvegarde de projet
- selection multiple
- enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
