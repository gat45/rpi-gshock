# unknown_decoder.h

**Path:** `data/unknown_decoder.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* unknown_decoder - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Par construction : les commandes sans sémantique connue restent listées avec leurs compteurs dans `command_observed.h`, sans interprétation.

Tout appelant qui inclut cet en-tête n'obtient que `#pragma once`. Ce module ne
doit donc être présenté nulle part comme fonctionnel.

## Status

- [ ] Implemented
- [ ] Tested
- [x] Bloqué par l'absence de mesure : aucune capture ne relie ces octets à
      des valeurs.

## Errors

_none yet_

## Corrections

- 2026-11-19 : la mention « stub (Phase 0) » est conservée, mais l'état réel
  (non implémenté) et sa raison sont maintenant explicites. Les anciens
  documents de projet laissaient croire que ces modules participaient au FAP.
