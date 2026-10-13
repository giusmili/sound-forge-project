# SonoForge Studio 0.8.2 — deplacement de fenetre

Date : 2 octobre 2026. Version locale Windows x64.

## Cause observee

La taille initiale fixe de la fenetre depassait la zone utile de l'ecran
avec la mise a l'echelle Windows. Dans le test avant correction, le haut
de la fenetre etait a -28 pixels : sa barre de titre native se trouvait
partiellement hors ecran. Le grand titre dans l'application n'etait pas
une zone de deplacement.

## Corrections

- Taille initiale adaptee a la zone utile du moniteur principal, en pixels
  logiques JUCE, avec une marge pour les bordures et la barre des taches.
- Grand titre de l'application utilisable comme poignee de deplacement.
- Double-clic sur ce titre pour agrandir puis restaurer la fenetre.
- Zone etendue au bandeau libre, sans recouvrir les boutons ni le selecteur
  de projets. La barre de titre Windows reste native.
- Corrections audio et MIDI de 0.8.1 conservees.

## Validation

Compilation Release x64 reussie.
Test souris sur un projet vide de l'executable compile :

```text
Avant glissement : x=135, y=38, largeur=1650, hauteur=945
Apres glissement : x=255, y=98, largeur=1650, hauteur=945
PASS : deplacement horizontal 120 pixels, vertical 60 pixels
PASS : double-clic agrandit et restaure la taille
```

La barre de titre demarre maintenant dans la zone visible de l'ecran.
La suite audio/MIDI a ete relancee : 1 suite reussie, 0 echec,
25,93 secondes. Voir RESULTATS-TESTS.txt.

## Utilisation

Cliquer et maintenir le grand titre SonoForge Studio dans l'application,
puis faire glisser. Si la fenetre est agrandie, double-cliquer sur ce titre
pour la restaurer avant de la deplacer. La barre de titre Windows permet
aussi le deplacement habituel.

Sources locales : C:/Users/giuse/sound-forge-project.
Les versions 0.8.0 et 0.8.1 sont conservees sur le bureau.
Aucune publication GitHub effectuee.
