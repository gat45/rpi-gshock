# export_csv.h

**Path:** `storage/export_csv.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* export_csv - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
#pragma once
#include <furi.h>
#include <storage/storage.h>

/* Write <session_path>/export_packets.csv from the decoded session data.
 * Returns true when the file was written completely. */
bool export_csv_create(Storage* storage, const char* session_path);
```

## Rôle

Fine enveloppe Flipper autour de `export_csv_session()` (`storage/export.h`) :
elle ouvre `File`, branche un `CasioSink` dessus, et laisse la sérialisation au
code testable sur hôte.

Première version (conservée pour mémoire) : elle copiait `packets.csv` octet
par octet vers `export_packets.csv`, donc exportait les trames brutes et pas les
valeurs décodées.

## Sortie réelle (session 0001 de la carte, vérifiée sur l'appareil)

```
timestamp,watch_name,battery_raw,battery_pct,temperature_c,record_time,value_a,value_b,sequence,frames_decoded,frames_rejected
2026-08-29T22:33:00Z,CASIO DW-H5600,7,0,27,2026-08-29 22:33,62892,0,1391,6,0
```

Avec un document `live.txt` sans record : `record_time` et les trois métriques
sont vides, `frames_*` valent 0 — c'est honnête, pas un bug.

## Known limits

- Pas d'échappement CSV : sûr uniquement parce que le nom décodé est de
  l'ASCII imprimable (le décodeur s'arrête sinon).
- Le chemin est construit dans un buffer de 256 octets (`snprintf`).

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_export.c` (format et contenu) +
      vérification sur carte (`storage read`)

## Errors

- La version historique compilait et écrivait un fichier, mais son contenu
  n'était qu'une copie de `packets.csv` : aucune valeur décodée.

## Corrections

- Remplacée par un wrapper `CasioSink` sur `export_csv_session()` ; la
  sérialisation est désormais couverte par les tests natifs.
- La signature est passée de `void` à `bool` : l'UI affiche `ok` / `fail`
  au lieu de supposer que l'écriture a réussi.
