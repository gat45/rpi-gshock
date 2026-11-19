# response_tracker.h

**Path:** `control/response_tracker.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* response_tracker - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Il n'y a pas de réponse à suivre : le FAP n'émet aucune requête vers la montre.

Tout appelant qui inclut cet en-tête n'obtient que `#pragma once`. Ce module ne
doit donc être présenté nulle part comme fonctionnel.

## Status

- [ ] Implemented
- [ ] Tested
- [x] Impossible sous Momentum : `api_symbols.csv` n'exporte que le serveur
      GATT (pas de GAP scan, pas de client GATT).

## Errors

_none yet_

## Corrections

- 2026-11-19 : la mention « stub (Phase 0) » est conservée, mais l'état réel
  (non implémenté) et sa raison sont maintenant explicites. Les anciens
  documents de projet laissaient croire que ces modules participaient au FAP.
