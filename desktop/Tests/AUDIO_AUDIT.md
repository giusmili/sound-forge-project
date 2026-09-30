# Audit du moteur audio, 30 septembre 2026

Base : `feature/windows-stable-core`, commit `1164512`.
Périmètre : import, moteur audio, transport, cycle de vie des périphériques,
configuration, destruction de l'interface et validation Windows.

## Défauts identifiés

1. **Bloquant, crash à l'import.** `AudioTrack` appelait
   `AudioTransportSource::setSource(reader, 32768, nullptr, rate)`.
   JUCE 9.0.2 exige un `TimeSliceThread` non nul dès que le tampon de lecture
   anticipée est activé. `juce_AudioTransportSource.cpp`, puis
   `BufferingAudioSource::prepareToPlay`, déréférencent cette référence.
   Retirer le callback audio pendant l'import ne corrige pas ce défaut.
2. **Arrêt dépendant d'un callback.** `AudioTransportSource::stop()` peut attendre
   environ une seconde par piste si le callback n'existe plus. L'ancien moteur
   détruisait les transports après retrait du callback.
3. **Transport non commun.** Les démarrages et recherches étaient effectués
   séquentiellement par piste pendant que le callback continuait. Le playhead
   était le maximum des positions, ce qui masquait les décalages.
4. **Configuration incomplète.** Les champs audio créés dans le JSON n'étaient
   ni alimentés ni restaurés. Un fichier corrompu empêchait les sauvegardes
   ultérieures. Le Master du mixer n'était pas persisté comme celui du bandeau.
5. **Durée de vie du dialogue audio.** Le dialogue asynchrone possédait une
   référence au gestionnaire audio sans être détruit explicitement avant lui.
6. **Validation insuffisante.** Aucun test exécutant réellement le moteur.
   Le workflow signé utilisait `secrets` dans des conditions `if` non prises
   en charge et recherchait un nom d'exécutable différent du produit CMake.

## Corrections

- Un thread de lecture appartenant au moteur, démarré avant les pistes et arrêté
  après leur destruction. Lecteurs, buffers et rééchantillonneurs ont une
  propriété et un ordre de destruction explicites.
- Horloge unique, commandes de transport synchronisées ; suppression des arrêts
  par piste attendant le callback. Perte/reconfiguration du périphérique :
  transport arrêté, position conservée, reprise explicite.
- Rééchantillonnage et lecture disque anticipée conservés. Si un buffer n'est
  pas prêt, aucune piste ni l'horloge n'avancent ; le callback émet du silence.
  Les reprises de cache peuvent donc être audibles, mais ne décalent pas les pistes.
- Callback découpé en blocs bornés, buffers préparés hors rendu, sortie nettoyée,
  valeurs non finies filtrées et somme bornée à [-1, 1]. Une contention avec une
  opération de contrôle produit du silence au lieu d'attendre le contrôle.
  Les verrous internes courts de JUCE restent présents : ce moteur n'est pas
  présenté comme strictement sans verrou.
- Import validé avant publication de la piste, échec journalisé et session
  existante conservée pour les erreurs de décodage initial/d'allocation traitées.
  Import réussi : arrêt et retour à zéro, comportement explicite.
- Configuration atomique, récupération du JSON invalide, persistance et
  restauration du périphérique et du Master ; fermeture possédée du dialogue.
- Tests du moteur intégrés à CTest et obligatoires avant empaquetage Windows.

## Limites de la preuve

La recette matérielle figure dans `WINDOWS_VALIDATION.md`. Des tests de rendu
sans carte son vérifient le moteur, pas les pilotes WASAPI/USB/ASIO, la qualité
à l'écoute ou la réactivité réelle de tous les décodeurs sur disque lent.
Les fautes natives ou blocages internes de décodeurs ne sont pas isolés dans un
processus séparé. Le journal permet de localiser l'étape, ce n'est pas un dump
mémoire Windows. La signature requiert toujours un certificat valide configuré.

Les fonctionnalités MIDI, partition et autres extensions restent hors périmètre.

## Vérifications exécutées

- Reproduction isolée de l'appel d'import d'origine avec JUCE 9.0.2 :
  assertion sur le thread nul, puis erreur mémoire (signal SIGSEGV sous Linux).
- Application et tests compilés en Debug avec GCC 13 / JUCE 9.0.2.
- 167 vérifications réussies : transport, import avant/après préparation,
  fréquences différentes, durées différentes, reprise après EOF, volumes/pan,
  Mute/Solo, sortie mono, offsets de buffer, formats WAV/AIFF/FLAC/OGG/MP3,
  forme d'onde en arrière-plan, réglages invalides et récupération du JSON.
- Stress concurrent : 100 cycles de commandes, imports et redémarrages simulés
  pendant les callbacks ; huit cycles de destruction du moteur en lecture.
- Le chemin de test sans périphérique n'instancie pas les services audio/MIDI
  du système. Il exerce le même rendu que l'application.

La CI Windows doit exécuter ces tests avant de produire le paquet 0.2.3.
Le fichier `BUILD-COMMIT.txt` identifie exactement le code du paquet.
La recette sur du matériel Windows reste à réaliser.
