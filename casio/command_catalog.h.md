# command_catalog.h

**Path:** `casio/command_catalog.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* command_catalog - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
typedef enum {
    CMD_SAFETY_SAFE = 0,   /* read-only request, no watch state change */
    CMD_SAFETY_CAUTION,    /* may change watch state */
    CMD_SAFETY_DANGEROUS,  /* modifies settings, alarms or time */
    CMD_SAFETY_BLOCKED,    /* never auto-execute */
} CmdSafety;

typedef struct {
    uint8_t id;
    const char* name;
    CmdSafety safety;
    const char* description;
} CmdEntry;

const CmdEntry* cmd_lookup(uint8_t id);
uint32_t cmd_count(void);
const CasioObservedCommand* cmd_observed(uint8_t id);
uint32_t cmd_observed_count(void);
const char* cmd_safety_name(CmdSafety safety);
```

## Deux tables, deux natures

L'en-tête insiste sur la séparation, et c'est le point important :

1. **Ce qui a été mesuré** — `kObservedCommands[]` dans `casio/command_observed.h`,
   généré par `tools/gen_command_observed.py` depuis les 5 captures btsnoop.
   42 identifiants, avec le nombre de trames envoyées, le nombre de réponses,
   les handles et un échantillon réel.
2. **Ce qui est sûr à faire** — `command_catalog.c`, écrit à la main, 27 entrées.
   C'est une **politique**, pas une mesure : une commande `DANGEROUS` peut être
   parfaitement valide, elle n'est simplement pas exécutable sans confirmation.

`cmd_lookup()` retourne `NULL` pour une commande non classée, et
`cmd_observed()` retourne `NULL` pour une commande jamais vue sur le fil. Ces
deux `NULL` veulent dire des choses différentes et ne doivent pas être
confondus par l'appelant.

## Known limits

- La politique ne couvre que 27 des 42 identifiants mesurés. Les 15 autres
  n'ont pas de classification : `cmd_lookup()` les refuse par construction.
- Aucune exécution n'est possible depuis le Flipper (pas de GATT client) :
  cette table sert à documenter et à armer le companion, pas à écrire.

## Status

- [x] Implemented
- [ ] Tested — aucune assertion native sur la politique ; l'UI affiche
      `cmd_count()` et `cmd_observed_count()` sur l'appareil, ce qui vérifie
      seulement que les tables se chargent

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » ne contenait que le `#pragma once` : l'UI
  affichait « 27 commands » alors qu'aucune table n'existait. Corrigé : la
  politique manuelle est dans `command_catalog.c` et les mesures sont dans un
  en-tête généré, séparés.
