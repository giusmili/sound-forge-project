# SonoForge Studio Desktop 0.5.2

Cette iteration securise la fermeture de l'application et ajoute les projets recents.

## Modifications non enregistrees

SonoForge compare l'etat courant du projet a la derniere sauvegarde manuelle ou au dernier projet ouvert.

Les modifications prises en compte comprennent notamment :

- import, suppression et duplication de clips
- deplacement, Split et Trim
- plages source non destructives
- gain et pan
- Mute et Solo
- BPM
- grille et snapping
- niveau Master

La simple navigation, comme le deplacement du playhead, le zoom ou la position de vue, ne marque pas le projet comme modifie.

Lorsqu'un projet contient des changements non enregistres, le titre affiche un astérisque :

```text
SonoForge Studio 0.5.2 - MonProjet *
```

## Fermeture securisee

Si l'utilisateur ferme SonoForge avec des changements non sauvegardes, trois choix sont proposes :

```text
Enregistrer
Ne pas enregistrer
Annuler
```

Enregistrer effectue la sauvegarde avant de quitter. Pour un projet sans nom, le selecteur de fichier est affiche.

Annuler laisse l'application ouverte.

## Changement de projet securise

La meme protection est appliquee avant :

- l'ouverture d'un autre projet
- l'ouverture d'un projet recent
- la recuperation d'un autosave

Une session modifiee ne peut donc pas etre remplacee silencieusement.

## Projets recents

SonoForge conserve jusqu'a 8 projets recents entre les lancements.

Ils sont presentes dans la liste Projets recents en haut de la fenetre.

Le fichier de configuration est stocke dans le dossier de donnees utilisateur SonoForge :

```text
SonoForge/recent-projects.txt
```

Les chemins devenus invalides sont retires automatiquement de la liste.

## Autosave et backup

Les protections de la 0.5.1 restent actives :

- autosave toutes les 30 secondes si l'etat a change
- fichier .autosave.sonoforge
- fichier .backup.sonoforge avant ecrasement manuel
- bouton Recuperer
- refus d'ecraser le projet principal si le backup echoue

## Format projet

```text
SonoForgeProject
formatVersion = 1
appVersion = 0.5.2
```

## Fonctions conservees

- transport global
- clips deplacables
- Split et Trim non destructifs
- selection / duplication / suppression
- Undo / Redo
- sauvegarde / ouverture .sonoforge
- autosave et backup
- detection des medias manquants
- grille 1/4, 1/8, 1/16
- BPM et snapping
- waveforms
- zoom et navigation
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

La base de session est maintenant assez solide pour commencer l'enregistrement audio :

- entree audio
- armement d'une piste
- Record / Stop
- creation du fichier WAV
- insertion automatique du nouveau clip dans le projet

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
