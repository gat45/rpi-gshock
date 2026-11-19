# session_manager.h

**Path:** `storage/session_manager.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once

#include <furi.h>
#include <storage/storage.h>

/* Create a new session directory under /ext/casiolab/sessions/session_NNNN.
   Returns FuriString with the path, or NULL on failure. Caller frees. */
FuriString* session_create(Storage* storage);

/* List existing session directories. Returns count. */
uint32_t session_count(Storage* storage);
```

État réel (2026-11-19) : identique. Implémentation dans `session_manager.c`.

## Comportement vérifié sur l'appareil

- Crée `/ext/casiolab/sessions/session_NNNN/` avec un `metadata.json` de 33 o.
- La numérotation repart de `session_0001` quand le dossier est vide, puis
  s'incrémente ; les sessions 0001 à 0013 ont été produites par les essais
  réels (voir `docs/TEST_STRATEGY.md`).
- `session_count()` retourne le nombre de répertoires `session_*`.

## Known limits

- Aucune rotation ni purge : les sessions s'accumulent sur la carte, et il
  n'existe pas de commande CLI `remove` pour les nettoyer à distance.
- Le chemin est construit dans un buffer de 256 octets.

## Status

- [x] Implemented
- [x] Tested on hardware — `storage list /ext/casiolab/sessions` montre les
      répertoires créés, et les exports y sont lisibles

## Errors

- `storage_mkdir` implicit declaration: function doesn't exist in Flipper SDK.
- `storage_dir_exists` and `storage_dir_read` signatures checked against storage.h.

## Corrections

- Replaced `storage_mkdir` with `storage_common_mkdir` (FS_Error storage_common_mkdir(Storage*, const char*)).
- storage_dir_exists(Storage*, const char*) returns bool — used for session numbering.
