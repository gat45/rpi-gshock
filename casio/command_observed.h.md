# command_observed.h

**Path:** `casio/command_observed.h`

## Content (2026-11-19)

En-tête **généré** — `tools/gen_command_observed.py`, ne pas éditer à la main.

```c
typedef struct {
    uint8_t id;
    uint16_t write_handle;
    uint16_t notify_handle;
    uint32_t requests;   /* frames sent by the central */
    uint32_t responses;  /* notifications from the watch */
    const char* sample;  /* most frequent response, first 80 hex chars */
} CasioObservedCommand;

static const CasioObservedCommand kObservedCommands[] = { ... };
static const uint32_t kObservedCommandCount;
```

## Provenance

Source unique : les 5 captures ATT du dépôt
`F:\freebuff\gshock\DW-H5600-EMU\analysis\btsnoop\`

```
btsnoop_hci-DW-H5600-1_att.csv
btsnoop_hci_DW-H5600-2_att.csv
btsnoop_hci_DW-H5600-3_att.csv
btsnoop_hci-DW-H5600-4_att.csv
btsnoop_hci_DW-H5600_att.csv
```

Chaque entrée est un comptage brut : combien de trames le central a envoyées,
combien la montre a répondu, sur quels handles, et la réponse la plus fréquente
en hexadécimal. Rien n'est interprété.

42 identifiants distincts sont présents, de `0x00` à la fin du catalogue.

## Known limits

- Ce fichier ne dit **pas** ce que fait une commande : `id`, compteurs et
  handles sont mesurés ; la sémantique ne l'est pas.
- `sample` est tronqué à 80 caractères hexadécimaux (40 octets).
- Un identifiant absent du tableau n'a jamais été vu sur le fil — ce n'est pas
  la preuve qu'il n'existe pas.
- `static const` dans un en-tête : inclus dans une seule unité de compilation
  par module ; plusieurs inclusions créeraient des copies (et des warnings
  `unused` si non référencées).
- Les captures ne contiennent pas de session `SET_TIME` : l'heure de la montre
  n'est pas couverte.

## Status

- [x] Generated — reproductible par `python tools/gen_command_observed.py`
- [x] Consumed — `ui/main_menu.c` (écran REVERSE) et `casio/command_catalog.c`
      (`cmd_observed`) ; les totaux affichés sur l'appareil viennent de ces
      compteurs

## Errors

_none yet_

## Corrections

- Les anciens documents annonçaient « 27 commands » : c'était la taille de la
  **politique** manuelle, pas des mesures. Le nombre mesuré est 42, et il est
  maintenant lu depuis ce fichier généré.
