# export_json.h

**Path:** `storage/export_json.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* export_json - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
#pragma once
#include <furi.h>
#include <storage/storage.h>

/* Write <session_path>/export_decoded.json from the decoded session data.
 * Returns true when the file was written completely. */
bool export_json_create(Storage* storage, const char* session_path);
```

## Rôle

Fine enveloppe Flipper autour de `export_json_session()`
(`storage/export.h`), sur le même modèle `CasioSink` que `export_csv.c`.

La version historique prenait un `const char* json_content` fourni par
l'appelant : elle ne décodait rien et ne garantissait pas la validité du JSON.

## Sortie réelle (session 0013 de la carte, vérifiée sur l'appareil)

```json
{"schema":"casiolab.session/1",
 "provenance":{"battery_raw":"measured","temperature_c":"measured",
               "battery_pct":"derived","record":"absent",
               "current_time":"reference-layout-unverified"},
 "timestamp":"","watch_name":"CASIO DW-H5600","name_valid":true,
 "battery":{"raw":7,"percent":0,"temperature_c":27},
 "record":null,"frames":{"decoded":0,"rejected":0}}
```

`provenance` suit le contenu réel : `absent` quand la valeur n'est pas dans le
document, `derived` pour le pourcentage, `reference-layout-unverified` pour
l'heure `0x09`.

## Known limits

- Pas d'échappement JSON des chaînes ; sûr uniquement parce que `watch_name`
  est de l'ASCII imprimable filtré par `watch_name_decoder`.
- Le document est produit par une suite d'écritures : un sink qui échoue au
  milieu laisse un JSON tronqué. Le retour `false` le signale à l'UI.

## Status

- [x] Implemented
- [x] Tested — `tests/native/test_export.c` (schéma, provenance, `record:null`,
      équilibre des accolades, sink mort refusé) + vérification sur carte

## Errors

- La version historique acceptait n'importe quelle chaîne : le fichier exporté
  pouvait être du JSON invalide sans que rien ne le signale.

## Corrections

- Remplacée par un wrapper `CasioSink` sur `export_json_session()`.
- La provenance est maintenant calculée à partir des données, pas écrite en
  dur : elle annonçait `"record":"measured"` alors que `record` valait `null`.
