# SonoForge Studio 0.1

Prototype original de station audionumérique multipiste inspirée des workflows modernes de DAW, sans reprendre le code, les ressources ni l'interface propriétaire de Cubase.

## Lancer

1. Décompresser le dossier.
2. Ouvrir `index.html` dans Chrome ou Edge récent.
3. Cliquer sur **Importer audio** ou glisser-déposer des fichiers audio.
4. Utiliser Play, Pause, Stop, Mute, Solo, Volume, Pan et le décalage de départ.
5. Cliquer sur **Exporter le mix WAV** pour générer un rendu stéréo.

Aucune installation et aucun serveur ne sont nécessaires pour cette V0.1.

## Fonctions présentes

- Multipiste audio
- Import audio pris en charge par le navigateur
- Affichage des formes d'onde
- Lecture synchronisée
- Positionnement temporel par piste
- Curseur de lecture et timeline
- Volume individuel
- Panoramique stéréo
- Mute / Solo
- Volume Master
- BPM
- Métronome
- Boucle du projet
- Zoom horizontal
- Export WAV stéréo 44,1 kHz / 16 bits

## Limites de cette V0.1

- Pas encore d'enregistrement micro
- Pas encore de pistes MIDI
- Pas de VST3
- Pas de découpage / déplacement graphique des clips
- Pas d'automation
- Pas encore de sauvegarde complète avec les fichiers audio
- Le support MP3/M4A/FLAC dépend des codecs du navigateur

## Architecture cible pour une version desktop

Pour une version de production réellement comparable à un DAW de bureau, la cible recommandée est C++ avec JUCE :

- moteur audio temps réel ASIO / WASAPI sous Windows
- enregistrement multipiste
- MIDI et piano roll
- hébergement VST3
- automation
- édition non destructive
- bus, inserts, sends et side-chain
- sauvegarde projet
- undo/redo
- export WAV/FLAC/MP3
- sampler et instruments virtuels

Le prototype web sert à valider rapidement l'ergonomie, la timeline et les workflows avant passage au moteur natif.
