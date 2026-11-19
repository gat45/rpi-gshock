# history_db.h

**Path:** `storage/history_db.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* history_db - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
void history_append_packet(Storage* storage, const char* session_path,
                          const char* timestamp, const char* direction,
                          const char* uuid, uint16_t handle,
                          const uint8_t* payload, size_t len,
                          const char* event_type);

void history_write_decoded(Storage* storage, const char* session_path,
                          const char* json_entry);

uint32_t history_session_count(Storage* storage);
```

## Rôle réel

- `history_append_packet()` écrit une ligne dans `<session>/packets.csv` :
  ouverture en `FSOM_OPEN_APPEND` et, si l'ouverture échoue, création avec la
  ligne d'en-tête. Le payload est écrit en hexadécimal.
- `history_write_decoded()` écrit `<session>/decoded.json` en
  `FSOM_CREATE_ALWAYS` (remplace).
- `history_session_count()` compte les sous-répertoires de
  `/ext/casiolab/sessions`.

Ce module n'est **pas** appelé par l'UI actuelle : l'export passe par
`storage/export_csv.c` et `storage/export_json.c`, qui sérialisent des valeurs
décodées. `history_db` est le chemin « trames brutes », conservé pour un futur
enregistreur de session.

## Known limits

- Aucune écriture atomique : une coupure au milieu d'une ligne laisse un CSV
  tronqué. Il n'y a pas de fichier temporaire ni de renommage.
- Une ligne est construite dans un buffer de 512 octets ; un payload très long
  est silencieusement tronqué (la boucle s'arrête avant la fin). Le champ `len`
  écrit reste la longueur **demandée**, pas la longueur écrite.
- `history_write_decoded()` ne valide pas le JSON reçu.
- `history_session_count()` compte tous les répertoires, sans filtre `session_*`.
- Pas de test natif : le module dépend de l'API Flipper `Storage`, non
  compilable sur l'hôte sans doublure.

## Status

- [x] Implemented
- [ ] Tested — jamais exercé sur l'appareil ni en test natif

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » ne déclarait aucune fonction ; le fichier
  `history_db.c` qui les définit a été ajouté, mais l'en-tête était resté à
  l'état de coquille vide. Corrigé ci-dessus.
