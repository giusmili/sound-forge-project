# SonoForge Studio Desktop

SonoForge Studio 0.2C consolide le fonctions de transport et de mixage.

## Fonctionnalites actuelles

- application native C++20 / JUCE 9.0.2
- lecture multipiste
- import de plusieurs fichiers audio
- volume et panoramique par piste
- Mute et Solo par piste
- volume Master
- Play, Pause et Stop
- configuration du peripherique audio
- timeline commune
- regle temporelle
- playhead synchronise
- formes d'onde via JUCE AudioThumbnail
- longueur graphique des clips proportionnelle a leur duree
- clic dans la regle ou une piste pour deplacer le playhead

## Comportement Solo

Des qu'une piste est en Solo, toutes les pistes non Solo sont silencieuses. Plusieurs pistes peuvent etre mises en Solo simultanement.

## Prochain chantier d'architecture

Le deplacement reel des clips dans le temps demande un transport global avec un offset de debut propre a chaque clip. Cette evolution sera isolee dans une branche dediee afin de ne pas fragiliser le moteur multipiste deja valide.

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

## Licence JUCE

JUCE 9.0.2 est distribue sous AGPL-3.0-only ou licence commerciale JUCE. Le choix de licence devra etre tranche avant toute distribution du logiciel.


## Build de test Windows

La branche `feature/windows-test-build` est utilisee pour valider automatiquement l'executable Windows x64 avant livraison de test.
