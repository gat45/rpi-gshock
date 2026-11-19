# packet_rewriter.h

**Path:** `proxy/packet_rewriter.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* packet_rewriter - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Sans proxy, il n'y a aucun paquet à réécrire.

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
