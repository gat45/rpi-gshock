# gatt_map.h

**Path:** `casio/gatt_map.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* gatt_map - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Aucune table handle → usage n'est générée. Les quatre handles utiles sont mesurés (`0x0016` commande, `0x0018` notification, `0x0013` LifeLog, `0x0010` trames `09 1D`) et documentés, pas centralisés ici.

Tout appelant qui inclut cet en-tête n'obtient que `#pragma once`. Ce module ne
doit donc être présenté nulle part comme fonctionnel.

## Status

- [ ] Implemented
- [ ] Tested
- [x] Non écrit : aucune mesure ne le bloquait, le module n'a simplement
      jamais été codé.

## Errors

_none yet_

## Corrections

- 2026-11-19 : la mention « stub (Phase 0) » est conservée, mais l'état réel
  (non implémenté) et sa raison sont maintenant explicites. Les anciens
  documents de projet laissaient croire que ces modules participaient au FAP.
