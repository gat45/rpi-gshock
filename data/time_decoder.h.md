# time_decoder.h

**Path:** `data/time_decoder.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* time_decoder - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
int time_decode(const uint8_t* payload, size_t len,
                int* year, int* month, int* day,
                int* hour, int* minute, int* second);
```

## Layout (dérivé de la référence, NON mesuré)

D'après `gshock_api/iolib/time_io.py` (`TimeEncoderPure.encode_current_time`) :

```
[0]     0x09 commande
[1..2]  année, little-endian
[3]     mois
[4]     jour
[5]     heure
[6]     minute
[7]     seconde
[8]     jour de semaine (1 = lundi .. 7 = dimanche)
[9]     octet sous-seconde
[10]    drapeau, toujours 0x01
```

## Statut de la preuve

**Reference-derived, non mesuré.** Le corpus ne contient que des trames
`09 1D …` sur le handle `0x0010`, qui sont des trames d'état DST
(opcode `0x09`, commande `0x1D`), pas l'heure courante. Aucune écriture
`SET_TIME` de l'application officielle n'a été capturée.

Conséquence assumée : ce décodeur ne doit pas être présenté comme validé sur
montre. Les horodatages réellement mesurés viennent des records LifeLog `0x00`
(`data/lifelog_record.h`).

## Known limits

- Refuse `mois` hors 1..12 (−2), `jour` hors 1..31 (−3), `heure` > 23 (−4),
  `minute` > 59 ou `seconde` > 60 (−5), et un payload < 11 octets (−1).
- Pas de validation des jours par mois, ni de l'année.
- Le jour de semaine et l'octet sous-seconde sont lus mais pas exposés.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_decoders.c` sur une trame construite selon le
      layout de référence, plus les cas d'erreur

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » ne décodait rien. Corrigé, mais l'en-tête
  annonce désormais explicitement que le layout n'est **pas mesuré** : le
  décodeur est testé contre le format de la référence, pas contre la montre.
