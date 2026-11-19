# activity_decoder.h

**Path:** `data/activity_decoder.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* activity_decoder - stub (Phase 0) */
```

État réel (2026-11-19) : identique. L'en-tête ne déclare aucune fonction.

## État

**NON IMPLÉMENTÉ.** Aucun payload d'activité n'a été identifié dans les 5 captures. Écrire un décodeur reviendrait à inventer le format.

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
