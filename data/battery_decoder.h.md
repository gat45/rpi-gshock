# battery_decoder.h

**Path:** `data/battery_decoder.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* battery_decoder - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
#define CASIO_BATT_LOWER_GENERIC 9
#define CASIO_BATT_UPPER_GENERIC 19

typedef struct {
    int raw_level;      /* measured step index */
    int temperature_c;  /* measured, degrees Celsius */
    int percent;        /* derived from raw_level and the limits */
} CasioBattery;

int battery_decode_full(const uint8_t* payload, size_t len,
                        int lower_limit, int upper_limit, CasioBattery* out);
int battery_decode(const uint8_t* payload, size_t len);
int battery_percent_from_raw(int raw_level, int lower_limit, int upper_limit);
```

## Provenance

`0x28` responses measured in the corpus (39 occurrences) :

```
28 06 1A 00 00 FA 00 00 00 00     raw 6, 26 C
28 07 1B 00 00 FA 00 00 00 00     raw 7, 27 C
28 04 1A 00 00 FA 00 00 00 01 01  raw 4, 26 C
```

- `raw_level` et `temperature_c` sont **mesurés** : octets 1 et 2 de la réponse.
- `percent` est **dérivé** : `(raw - lower) * round(100/(upper-lower))`, borné 0..100,
  formule des clients de référence.
- Le couple (lower, upper) **du DW-H5600 n'est pas confirmé** par une capture à
  batterie connue. `battery_decode()` garde donc la paire générique (9, 19) et
  peut retourner 0 % pour un `raw` de 6 ou 7 — c'est attendu, pas un bug.

## Known limits

- `raw_level` est un index de palier, pas un pourcentage : ne jamais l'afficher
  comme « 7 % ».
- `battery_percent_from_raw` existe pour les valeurs venues d'ailleurs
  (companion, `live.txt`) et applique la même formule.
- `battery_decode_full` refuse `payload[0] != 0x28` (−2) et `upper <= lower` (−3).

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_decoders.c` : raw/temp mesurés, mapping avec
      limites injectées, limites invalides, mauvaise commande

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » ne décodait rien : elle retournait la valeur
  brute en la présentant comme un pourcentage. Corrigé : `battery_decode_full`
  expose `raw_level`, `temperature_c` et `percent` séparément, avec la provenance
  de chacun.
- `battery_decode()` seul ne suffisait pas au companion ni à `live.txt`, qui
  reçoivent parfois un `raw` sans trame : ajout de `battery_percent_from_raw()`.
