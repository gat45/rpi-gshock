# data_manager.h

**Path:** `data/data_manager.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* data_manager - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
#define CASIO_NAME_MAX 24

typedef struct {
    int battery_pct;      /* derived, -1 = unknown */
    int battery_raw;      /* measured, -1 = unknown */
    int temperature_c;    /* measured */

    int year, month, day, hour, minute, second;
    int time_valid;       /* reference layout, NOT measured */

    char watch_name[CASIO_NAME_MAX];
    int name_valid;

    CasioLifeLogRecord last_record;
    int record_valid;

    uint32_t frames_decoded;
    uint32_t frames_rejected;
} CasioData;

CasioData* data_manager_get(void);
void data_update_battery(const uint8_t* payload, size_t len);  /* 0x28 */
void data_update_time(const uint8_t* payload, size_t len);     /* 0x09 */
void data_update_name(const uint8_t* payload, size_t len);     /* 0x23 */
void data_update_record(const CasioLifeLogRecord* rec);        /* type 0x00 */
int  data_apply_live(const CasioLiveState* live);
int  data_update_frame(const uint8_t* payload, size_t len);
void data_reset(void);
```

## Rôle

État décodé unique de l'application, avec la provenance de chaque champ :

| Champ | Provenance |
|---|---|
| `battery_raw`, `temperature_c` | mesurés (captures btsnoop) |
| `battery_pct` | dérivé (formule de référence + limites) |
| `watch_name` | mesuré (`0x23`) |
| `last_record` | mesuré (record `0x00`) |
| `year..second` (`time_valid`) | **non vérifié** : format de référence `0x09`, jamais capturé sur DW-H5600 |

`data_update_frame()` dispatche sur l'octet de commande (`0x28`, `0x23`,
`0x09`) et compte les trames refusées ; une commande inconnue incrémente
`frames_rejected` et retourne −1.

`data_apply_live()` fusionne un document `live.txt` : seuls les champs présents
sont écrits, ce qui permet au companion d'envoyer un nom sans écraser une
mesure déjà décodée.

## Known limits

- Les compteurs `frames_decoded` / `frames_rejected` comptent des **trames**,
  pas des sessions ; ils repartent à zéro sur `data_reset()`.
- `data_apply_live()` ne remet pas `frames_*` à zéro : c'est l'appelant qui
  décide (l'UI fait `data_reset()` avant, pour que le live soit un instantané
  autoritaire et ne mélange pas un record d'échantillon).
- Instance globale unique, non thread-safe (l'UI est mono-thread).

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_decoders.c` (dispatch, compteurs, reset) et
      `tests/native/test_live.c` (fusion live, champs absents non écrasés)

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » n'avait aucun champ ni aucune fonction :
  l'UI et l'export lisaient des valeurs inexistantes. Corrigé par l'état
  complet ci-dessus.
- `data_apply_live()` a d'abord été conçu sans remise à zéro : un record
  d'échantillon restait affiché après un passage en live. Corrigé côté UI :
  un document live valide est un instantané, précédé de `data_reset()`.
