# SonoForge Studio 0.6 - Guide de test Windows

Cette build est une version de test du MVP natif SonoForge Studio.

## Environnement recommande

- Windows 11 64 bits
- processeur Intel ou AMD x64
- sortie audio Windows WASAPI
- microphone ou interface audio si vous testez Record
- fichiers audio WAV, MP3, AIFF, FLAC ou OGG

## Demarrage

1. Extraire completement le ZIP.
2. Lancer `SonoForge Studio.exe`.
3. Si Windows affiche SmartScreen, verifier que le fichier provient bien du package GitHub SonoForge avant de poursuivre.
4. Ouvrir `Audio` pour choisir l'entree et la sortie audio.

Cette build n'est pas encore signee numeriquement.

## Parcours de test conseille

### Lecture et montage

1. Importer deux fichiers audio.
2. Lancer Play, Pause et Stop.
3. Deplacer un clip sur la timeline.
4. Tester le Snap avec les grilles 1/4, 1/8 et 1/16.
5. Tester Zoom et Vue.
6. Tester volume, pan, Mute et Solo.
7. Dupliquer puis supprimer un clip.
8. Placer le playhead au milieu d'un clip et utiliser Couper.
9. Redimensionner les bords gauche et droit du clip.
10. Tester Annuler et Retablir.

### Projet

1. Enregistrer un projet `.sonoforge`.
2. Fermer puis relancer SonoForge.
3. Rouvrir le projet.
4. Verifier que les clips, reglages, BPM et positions sont restaures.
5. Modifier le projet et attendre au moins 30 secondes pour tester l'autosave.
6. Verifier la presence des fichiers `.autosave.sonoforge` et `.backup.sonoforge` lorsque cela s'applique.

### Enregistrement audio

1. Dans Audio, choisir une entree microphone ou interface.
2. Positionner le playhead.
3. Cliquer sur Record.
4. Produire quelques secondes d'audio.
5. Arreter l'enregistrement.
6. Verifier qu'une nouvelle piste apparait au bon endroit.
7. Lire la piste enregistree.

## Points a surveiller

- absence de craquements ou coupures audio
- synchronisation du playhead
- latence a l'enregistrement
- comportement apres changement de peripherique audio
- stabilite lors de l'import de plusieurs fichiers
- conservation correcte des projets apres fermeture
- comportement si un fichier audio source a ete deplace

## Etat de la build

SonoForge 0.6 reste une build MVP / alpha technique. Le CI valide la compilation Windows, mais les tests sur du materiel audio reel restent indispensables avant une distribution publique.
