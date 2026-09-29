# SonoForge Studio Desktop

Base native du projet SonoForge Studio.

## Etat actuel

La branche de developpement 0.2 ajoute un premier moteur multipiste au socle 0.1 :

- application native C++20 / JUCE 9.0.2
- sortie audio Windows
- import de plusieurs fichiers WAV, AIFF, FLAC, OGG et MP3
- lecture simultanee de plusieurs pistes
- volume individuel jusqu'a +3.5 dB environ
- panoramique gauche / droite
- mute par piste
- volume Master
- transport Play / Stop
- compteur temporel
- configuration du peripherique audio

La prochaine sous-etape ajoutera la representation graphique des clips et des waveforms.

## Prerequis Windows

- Windows 11
- Visual Studio 2022 avec le workload Desktop development with C++
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

## Architecture en cours

```text
MainComponent
    |
    v
AudioEngine
    |
    +-- MixerAudioSource
          |
          +-- AudioTrack 1
          +-- AudioTrack 2
          +-- AudioTrack N
```

Chaque piste possede son propre transport et ses propres reglages. Le moteur melange les pistes avant d'appliquer le gain Master.

## Licence JUCE

JUCE 9.0.2 est distribue sous AGPL-3.0-only ou licence commerciale JUCE. Le choix de licence devra etre tranche avant toute distribution du logiciel.
