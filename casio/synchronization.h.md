# synchronization.h

**Path:** `casio/synchronization.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* synchronization - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** La synchronisation horaire n'est ni implémentée ni mesurée : le corpus ne contient aucune session `SET_TIME`.

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
