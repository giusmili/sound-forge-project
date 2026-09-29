# SonoForge Studio Desktop 0.5.1

Cette iteration ajoute l'autosave et une sauvegarde de secours avant ecrasement manuel.

## Autosave

SonoForge verifie l'etat du projet toutes les 30 secondes.

Le fichier n'est reecrit que si le contenu de la session a reellement change.

Pour un projet deja enregistre :

```text
MonProjet.sonoforge
MonProjet.autosave.sonoforge
```

Pour une session qui n'a pas encore de nom, la recuperation est stockee dans le dossier de donnees utilisateur :

```text
SonoForge/Autosave/Recovery.autosave.sonoforge
```

## Recuperer

Le bouton Recuperer devient disponible lorsqu'un autosave exploitable existe.

Pour un projet nomme, l'autosave doit etre plus recent que le fichier principal pour etre propose.

La recuperation restaure la session sans transformer le fichier autosave en projet principal.

## Sauvegarde de secours

Avant d'ecraser manuellement un projet existant, SonoForge copie l'ancienne version :

```text
MonProjet.backup.sonoforge
```

Si cette copie de secours ne peut pas etre creee, SonoForge refuse d'ecraser le fichier principal.

Apres une sauvegarde manuelle reussie, l'autosave devenu inutile est supprime.

## Contenu protege

Autosave, Backup et projet principal utilisent le meme format versionne. Ils conservent :

- fichiers audio sources
- sourceStart / sourceEnd
- startOffset
- volume et pan
- Mute / Solo
- playhead
- BPM
- grille et snapping
- zoom et vue
- Master

## Format

```text
SonoForgeProject
formatVersion = 1
appVersion = 0.5.1
```

## Fonctions conservees

- transport global
- clips deplacables
- Split et Trim non destructifs
- selection / duplication / suppression
- Undo / Redo
- sauvegarde et ouverture .sonoforge
- detection des medias manquants
- grille 1/4, 1/8, 1/16
- BPM et snapping
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

- fermeture securisee avec detection des modifications non sauvegardees
- projet recent
- puis enregistrement audio

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
