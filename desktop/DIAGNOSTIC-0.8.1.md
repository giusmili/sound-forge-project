# SonoForge Studio 0.8.1 — diagnostic et validation

Date : 2 octobre 2026. Plateforme testee : Windows x64, sur ce PC.
Depot : https://github.com/giusmili/sound-forge-project.git
Base exacte : 57ac68025cbffcf2aed46e7141e5b6262700e00e (0.8.0).
Branche locale : fix/midi-undo-stability.

## Probleme constate dans 0.8.0

Le demarrage et l'ajout MIDI fonctionnent. Pendant la sequence + MIDI,
Tester C4, Annuler, le processus a plante. Windows confirme un APPCRASH
a 15:28:52, exception 0xc0000005, module signale ntdll.dll.
Le dump original est dans AppData/Local/CrashDumps/SonoForge Studio.exe.14376.dmp.
La pile du dump n'a pas ete symbolisee ; le module signale ne suffit pas
a attribuer la cause a Windows.

## Corrections

1. Les lignes MIDI et controles du mixer sont detruits avant leurs pistes
   lors d'une restauration/annulation ou suppression. Le destructeur MIDI
   envoyait allNotesOff a une piste deja liberee. Les interfaces sont
   reconstruites si la restauration echoue. La suppression par bouton MIDI
   est differee jusqu'a la fin du clic.
2. Le tampon MIDI est efface avant la synthese. JUCE ajoute les echantillons
   de synthese, alors que son mixer attend une source remplacant le tampon.
   Cela evite de reutiliser les echantillons d'une autre piste.
3. Les transports audio disposent d'un thread partage de lecture anticipee.
   La version precedente demandait un tampon de 32768 echantillons avec
   un thread nul, contrairement au contrat JUCE. Le thread reste vivant
   jusqu'a la destruction de tous les transports.

Version et instructions de tests mises a jour. La compilation GitHub est
preparee pour executer les tests avant de produire le package 0.8.1.
Les changements restent locaux ; aucune publication GitHub n'a ete faite.

## Validation reussie

- Compilation Release MSVC x64 : application et programme de tests.
- CTest : 1 suite reussie, 0 echec, duree 15,70 secondes.
- 50 cycles MIDI : ajout, note active, annulation, retablissement,
  suppression et restauration, conservation du volume.
- Synthese : echantillons non nuls et finis ; silence en mode muet.
- Piste MIDI inactive : effacement des anciens echantillons.
- Fichier WAV genere : import, duree, lecture, pause, arret et mode muet.
- Projet audio/MIDI : sauvegarde et reouverture, conservation du volume
  et du mode muet, presence de la piste audio.
- Restauration avec un fichier absent : echec propre et interfaces utilisables.
- Interface Windows de l'executable : 10 cycles MIDI, Mixer ouvert,
  Tester C4 suivi immediatement d'Annuler ou Supprimer, annuler/retablir
  la suppression, puis fermeture propre par Quitter.
- Verification de la version binaire : 0.8.1.

Le programme de tests calcule le rendu audio en memoire. Le test d'interface
utilise les commandes de l'application ; il ne mesure pas la qualite sonore.

## Limites

L'enregistrement micro, l'import MP3, la latence, les sessions longues et
les anciens projets personnels n'ont pas ete testes. La version conserve
les limites MIDI existantes (pas de piano roll ou de VST3).
Ces tests valident les fonctions exercees et les corrections ciblees,
pas toutes les configurations audio ou toutes les fonctions de l'application.

L'executable utilise le runtime Microsoft Visual C++ x64, disponible sur ce PC.
La version 0.8.0 est conservee dans son dossier original sur le bureau.

## Reproduire

Depuis le depot local, avec CMake disponible :

```powershell
cmake -S desktop -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
$env:_CL_ = "/MP2"
cmake --build build --config Release --parallel 1
ctest --test-dir build -C Release --output-on-failure
& desktop/Tests/Smoke-Windows.ps1 -Executable "$PWD/build/SonoForge_artefacts/Release/SonoForge Studio.exe"
```

Le test d'interface lance une nouvelle instance et verifie un projet vide.
Le journal CTest fourni dans le package contient les resultats observes.
