# export_bin.h

**Path:** `storage/export_bin.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* export_bin - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Aucun export binaire n'est codé. Les exports réels sont CSV (`storage/export_csv.c`) et JSON (`storage/export_json.c`).

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
