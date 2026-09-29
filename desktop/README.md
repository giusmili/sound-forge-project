# SonoForge Studio Desktop 0.5

Cette iteration transforme le snapshot interne en vrai fichier projet persistant.

## Fichier .sonoforge

Un projet SonoForge est un fichier JSON versionne et lisible.

Il conserve :

```text
format
formatVersion
appVersion

position du playhead
BPM
master
zoom
vue
grille
snap

tracks[]
    absolutePath
    relativePath
    startOffset
    sourceStart
    sourceEnd
    gain
    pan
    mute
    solo
```

## Enregistrer

Le bouton Enregistrer cree un fichier `.sonoforge`.

Apres la premiere sauvegarde, les sauvegardes suivantes reutilisent le meme fichier projet.

## Ouvrir

Le bouton Ouvrir projet restaure :

- toutes les pistes
- les plages audio non destructives
- les positions des clips
- gain et pan
- Mute et Solo
- BPM
- grille et snapping
- zoom et position de vue
- Master
- position du playhead

L'historique Undo / Redo est reinitialise apres ouverture d'un projet.

## Medias audio

Chaque piste enregistre deux chemins :

- chemin absolu
- chemin relatif au dossier du projet

Au chargement, SonoForge essaie d'abord le chemin absolu puis le chemin relatif.

Cela permet de deplacer un dossier de projet avec ses fichiers audio tout en conservant les liens.

## Fichiers manquants

Avant toute restauration, SonoForge verifie que tous les fichiers audio sont disponibles.

Si un ou plusieurs medias sont absents :

- la liste des fichiers manquants est affichee
- le projet actuel reste intact
- aucun chargement partiel n'est effectue

## Format

Version actuelle du format :

```text
SonoForgeProject
formatVersion = 1
```

Le numero de format est distinct de la version de l'application afin de permettre de futures migrations.

## Fonctions conservees

- transport global
- clips deplacables
- selection / duplication / suppression
- Split au playhead
- trim non destructif
- Undo / Redo
- grille 1/4, 1/8, 1/16
- BPM et snapping
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

- autosave
- projet recent / sauvegarde de secours
- puis enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
