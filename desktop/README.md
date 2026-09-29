# SonoForge Studio Desktop

Premiere base native du projet SonoForge Studio.

## Objectif de cette etape

La version 0.1 Desktop fournit le socle minimum du futur DAW :

- application native C++/JUCE
- initialisation de la sortie audio
- import WAV, AIFF, FLAC, OGG et MP3 selon le support JUCE
- lecture et arret
- compteur temporel
- volume master
- fenetre de configuration du peripherique audio

Cette etape ne cherche pas encore a reproduire une timeline multipiste complete. Le but est d'abord de stabiliser le moteur audio.

## Prerequis Windows

- Windows 11
- Visual Studio 2022 avec le workload "Desktop development with C++"
- CMake 3.22 ou plus recent
- Git

JUCE 9.0.2 est recupere automatiquement par CMake avec FetchContent.

## Configuration

Depuis la racine du depot :

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
```

## Compilation

```powershell
cmake --build build --config Release --parallel
```

L'executable est genere dans le dossier d'artefacts CMake/JUCE du build.

## Prochaine etape

Le prochain jalon ajoutera :

- timeline
- pistes audio multiples
- waveform
- positionnement des clips
- mute / solo / volume / pan par piste

## Licence JUCE

JUCE 9.0.2 est distribue sous AGPL-3.0-only ou licence commerciale JUCE. Le choix de licence devra etre tranche avant toute distribution du logiciel.
