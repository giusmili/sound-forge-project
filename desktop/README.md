# SonoForge Studio Desktop 0.6

Cette iteration ajoute le premier enregistrement audio natif de SonoForge.

## Enregistrement audio

Le bouton Record utilise l'entree audio active du systeme.

Au demarrage :

- SonoForge memorise la position du playhead
- la lecture des pistes existantes continue
- l'entree audio est ecrite dans un fichier WAV 24 bits
- l'ecriture disque est effectuee par un ThreadedWriter en arriere-plan
- l'horloge projet continue d'avancer pendant l'enregistrement

A l'arret :

- le fichier WAV est finalise
- le fichier est importe automatiquement comme nouvelle piste
- le clip est place a la position exacte de depart de l'enregistrement
- l'action peut etre annulee avec Undo

## Dossiers d'enregistrement

Pour un projet deja sauvegarde :

```text
MonProjet/
├── MonProjet.sonoforge
└── Audio/
    └── Recording_<timestamp>.wav
```

Pour une session sans fichier projet :

```text
Documents/
└── SonoForge Recordings/
    └── Recording_<timestamp>.wav
```

## Configuration audio

SonoForge tente maintenant d'ouvrir :

- 1 canal d'entree
- 2 canaux de sortie

Si aucune entree n'est disponible, l'application retombe sur la configuration de lecture seule.

La fenetre Audio permet desormais de choisir jusqu'a 2 canaux d'entree et 2 canaux de sortie.

## Architecture temps reel

La lecture et l'enregistrement passent par un callback audio commun :

```text
Audio Device
     |
     +--> Input --> ThreadedWriter --> WAV
     |
     +--> AudioEngine --> Mixer --> Output
```

Le thread audio ne realise pas directement les ecritures disque.

## Limitations 0.6

- une seule prise enregistree a la fois
- pas encore d'armement individuel par piste
- pas encore de monitoring d'entree configurable
- pas encore de compteur dB d'entree
- pas encore de pre-roll
- pas encore de punch in/out
- latence a valider sur materiel Windows reel
- ASIO non encore integre

## Fonctions conservees

- fermeture securisee
- projets recents
- autosave / backup / recovery
- sauvegarde .sonoforge
- Undo / Redo
- transport global
- clips deplacables
- Split / Trim
- grille musicale
- BPM / snapping
- volume / pan / Mute / Solo
- Master

## Prochaine iteration

- armement d'une piste
- vumetre d'entree
- selection mono/stereo
- monitoring
- test de latence
- puis ASIO

## Compilation Windows

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
