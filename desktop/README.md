# SonoForge Studio Desktop

SonoForge Studio : stabilisation du moteur audio sur `feature/windows-stable-core`.

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

## Stabilisation en cours

Aucune nouvelle fonctionnalité avant validation du moteur sur Windows.
L'audit est dans [Tests/AUDIO_AUDIT.md](Tests/AUDIO_AUDIT.md) et la recette
matérielle dans [Tests/WINDOWS_VALIDATION.md](Tests/WINDOWS_VALIDATION.md).

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

La branche `feature/windows-stable-core` compile le programme et exécute les
tests du moteur avant de produire le paquet de test Windows.

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Les tests utilisent le même moteur avec des fichiers temporaires, sans ouvrir
de carte son et sans modifier la configuration de l'utilisateur.


## Diagnostic Windows

La branche stable-core cree automatiquement les donnees utilisateur dans le dossier d'application Windows de l'utilisateur :

- config/settings.json : preferences persistantes et valeurs audio
- logs/sonoforge.log : journal de demarrage, initialisation audio et import de pistes

Ces fichiers sont crees au premier lancement et permettent de diagnostiquer un echec d'import sans modifier l'executable.
