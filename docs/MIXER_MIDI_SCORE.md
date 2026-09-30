# Mixer et partition MIDI

## Objectif de la branche

Cette branche introduit deux briques complémentaires dans SonoForge Studio :

1. une console de mixage légère, synchronisée avec les pistes de l'arrangement ;
2. un modèle de piste MIDI pouvant alimenter un piano roll puis une vue partition.

## Mixer V0.2

La console réutilise l'état des pistes existantes au lieu de créer un second moteur audio.

Chaque tranche de console expose :

- nom de piste ;
- Mute / Solo ;
- volume ;
- panoramique ;
- indicateur de niveau ;
- sortie vers le master.

Le master reste piloté par le gain master Web Audio existant.

## Modèle MIDI

Une piste MIDI possède au minimum :

```js
{
  type: "midi",
  notes: [
    { pitch: 60, start: 0, duration: 1, velocity: 100 }
  ],
  channel: 1,
  program: 0
}
```

Les temps sont exprimés en battements. Le BPM du projet assure la conversion battements / secondes.

## Partition

La vue partition est dérivée des événements MIDI. La première version doit gérer :

- clé de sol ;
- mesure 4/4 ;
- notes et silences courants ;
- regroupement par mesure ;
- sélection d'une piste MIDI ;
- bascule Piano Roll / Partition.

La notation ne doit pas devenir la source de vérité : les événements MIDI restent le modèle principal.

## Export cible

Ordre prévu :

1. affichage de la partition ;
2. export MusicXML ;
3. impression / PDF via la vue partition.

MusicXML permettra ensuite l'échange avec des logiciels de notation musicale.
