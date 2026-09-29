# SonoForge Studio 0.7 - Guide de test Windows

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
