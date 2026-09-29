# SonoForge Studio Desktop

SonoForge Studio est maintenant dans la phase 0.2B du moteur natif.

## Fonctionnalites actuelles

- application native C++20 / JUCE 9.0.2
- lecture multipiste
- import de plusieurs fichiers audio
- volume et panoramique par piste
- mute par piste
- volume Master
- configuration du peripherique audio
- timeline commune
- regle temporelle
- playhead synchronise
- formes d'onde generees par JUCE AudioThumbnail
- longueur graphique des clips proportionnelle a leur duree
- clic dans la regle ou dans une piste pour deplacer le playhead

## Architecture

```text
MainComponent
    |
    +-- TimelineRulerComponent
    |
    +-- TrackRowComponent N
    |       |
    |       +-- AudioThumbnail
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

## Prerequis Windows

- Windows 11
- Visual Studio 2022 avec Desktop development with C++
- CMake 3.22 ou plus recent
- Git

## Configuration

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64
```

## Compilation

```powershell
cmake --build build --config Release --parallel
```

## Etapes suivantes

- deplacement des clips dans le temps
- redimensionnement non destructif
- zoom horizontal
- Solo
- pause
- sauvegarde du projet
- enregistrement audio

## Licence JUCE

JUCE 9.0.2 est distribue sous AGPL-3.0-only ou licence commerciale JUCE. Le choix de licence devra etre tranche avant toute distribution du logiciel.
