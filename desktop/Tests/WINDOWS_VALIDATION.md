# Validation du moteur audio Windows

La compilation et les tests sans matériel ne suffisent pas à valider un pilote
Windows ou une carte son réelle. Aucun ajout fonctionnel avant cette recette.
Consigner le commit testé, la version de Windows, le périphérique, le pilote,
la fréquence et la taille de tampon pour chaque essai.

## Données et diagnostic

Au premier lancement, SonoForge crée :

- `%APPDATA%\SonoForge\config\settings.json`
- `%APPDATA%\SonoForge\logs\sonoforge.log`

Le périphérique, sa configuration JUCE et le volume Master sont persistants.
Un JSON invalide est sauvegardé dans `settings.invalid.json`, puis remplacé
par des valeurs par défaut. Une sortie indisponible doit être visible sous le
titre ; le bouton Audio permet de choisir un périphérique.

## Recette sur Windows 11

| Essai | Résultat attendu |
| --- | --- |
| Premier lancement, puis relancement | Configuration et journal présents ; Master et sortie restaurés |
| WAV mono et stéréo, 44,1 / 48 / 96 kHz | Import sans fermeture ; durée et forme d'onde cohérentes ; son à vitesse normale |
| MP3, AIFF, FLAC, OGG | Import et lecture ; vérifier le début, une recherche et la fin pour chaque format |
| Nom avec accents, espaces et chemin long | Import et forme d'onde corrects |
| 10 pistes, durées différentes | Lecture synchronisée ; fin de chaque piste silencieuse ; arrêt à la fin de la plus longue |
| 100 séquences Play / Pause / Stop / recherche | Aucun blocage ; Pause conserve la position ; Stop revient à zéro |
| Import pendant la lecture | Arrêt et retour à zéro, piste ajoutée ; Play redémarre toutes les pistes ensemble |
| Fichier absent, vide, faux WAV, tronqué | Message d'échec si le décodeur rejette le fichier ; session précédente utilisable |
| Volume, pan, Mute, Solo et Master | Niveaux cohérents ; Mute reste prioritaire ; plusieurs Solo possibles |
| Changement 44,1 / 48 / 96 kHz et tampons 128 / 512 / 1024 | Arrêt propre ; position préservée ; reprise seulement sur Play |
| Débranchement/rebranchement USB en lecture | Aucun crash ou gel ; état audio visible ; choix de sortie et reprise possibles |
| Sortie absente ou occupée | Application et import utilisables ; pas de faux état de lecture |
| Fermeture avec dialogue Audio ouvert, en lecture | Fermeture immédiate sans crash ; 20 lancements/fermetures successifs |
| Lecture prolongée, 30 minutes, puis manipulation | Pas de coupures répétées, dérive, hausse continue de mémoire ou blocage |

Tester au minimum WASAPI partagé avec la sortie intégrée et, si disponible,
une interface USB. Les autres modes/pilotes doivent être validés séparément.
Le moteur accepte actuellement les fichiers mono/stéréo de 8 à 384 kHz et
jusqu'à 24 heures. Les autres flux sont refusés avec un message explicite.

## En cas d'échec

Conserver le journal, le fichier audio déclencheur et les étapes exactes.
Pour une fermeture brutale, relever également le module fautif et le code
exception dans l'Observateur d'événements Windows, Journaux Windows > Application.
Ne pas qualifier la version de stable tant qu'un essai ci-dessus échoue.
