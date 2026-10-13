# SonoForge Studio 0.8.2 - Guide de test Windows

## Deplacement de la fenetre 0.8.2

Glisser le grand titre SonoForge Studio dans le bandeau de l'application.
La fenetre doit suivre la souris. Un double-clic sur ce titre agrandit ou
restaure la fenetre. Les boutons et le selecteur de projets restent independants.
La barre de titre Windows conserve son comportement habituel.

## Regression de stabilite MIDI 0.8.2

- Ajouter une piste MIDI, cliquer Tester C4 puis immediatement Annuler.
- Retablir la piste, cliquer Tester C4 puis Supprimer sur la piste.
- Annuler et retablir cette suppression plusieurs fois.
- Refaire ces operations avec le Mixer ouvert.
- Enregistrer un projet MIDI puis le rouvrir alors qu'une piste MIDI existe deja.
- Verifier la fermeture pendant le test C4.

Aucune de ces operations ne doit planter. Les controles sont detruits avant
leurs pistes afin que leur destructeur ne lise pas une piste deja liberee.

Test automatise : `ctest --test-dir build -C Release --output-on-failure`.
La cible SonoForgeTests repete 50 cycles et verifie qu'une restauration
echouee conserve les pistes existantes. Elle verifie aussi le rendu du
synthetiseur, le silence MIDI, l'import et le transport WAV et la sauvegarde
d'un projet audio/MIDI avec conservation du volume et du mode muet.

Compilation Windows avec limitation de la consommation memoire :

```powershell
$env:_CL_ = "/MP2"
cmake --build build --config Release --parallel 1
ctest --test-dir build -C Release --output-on-failure
```

Compilation sans tests :
`cmake -S desktop -B build -DBUILD_TESTING=OFF`.

## 1. Fermeture

Tester successivement :

- le bouton X de la fenetre
- le bouton Quitter
- Fichier > Quitter

Modifier d'abord le projet afin de verifier la fenetre :

```text
Enregistrer
Ne pas enregistrer
Annuler
```

Chaque choix doit avoir le comportement attendu.

## 2. Piste Audio

Cliquer sur + Audio ou utiliser :

```text
Piste > Ajouter piste audio...
```

Choisir un WAV ou MP3 puis verifier lecture, waveform, volume, pan, Mute et Solo.

## 3. Piste MIDI / Instrument

Cliquer sur + MIDI ou utiliser :

```text
Piste > Ajouter piste MIDI / Instrument
```

Une ligne violette Instrument doit apparaitre.

Verifier :

- nom Instrument 1, Instrument 2...
- Instrument : Basic Synth
- bouton Tester C4
- volume
- Mute
- Supprimer

Cliquer Tester C4. Un son de synthese court doit etre audible sur la sortie audio active.

## 4. Projet MIDI

Ajouter une piste MIDI, modifier son volume ou Mute, puis enregistrer le projet.

Fermer SonoForge, relancer et ouvrir le .sonoforge.

La piste MIDI doit etre restauree avec ses reglages.

## 5. Ancien projet

Ouvrir un ancien projet .sonoforge cree avec la version 0.6.

Il doit continuer a s'ouvrir normalement.

## Limites MIDI 0.7

Cette version valide le type de piste instrument et le routage sonore. Elle ne contient pas encore :

- piano roll
- clips MIDI
- edition de notes
- clavier MIDI externe
- VST3

Ces fonctions arrivent sur cette base.
