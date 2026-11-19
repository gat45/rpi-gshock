# casiolab_test.h

**Path:** `tests/native/casiolab_test.h`

## Content (2026-11-19)

Micro-harnais d'assertions, sans dépendance ni système de build.

```c
extern unsigned g_checks;
extern unsigned g_failures;

#define CK(cond)                        /* condition booléenne */
#define CK_INT(actual, expected)        /* entiers, comparés en long */
#define CK_STR(actual, expected)        /* chaînes, NULL accepté */
#define CK_BYTES(actual, expected, len) /* memcmp */

void test_drsp(void);
void test_decoders(void);
void test_export(void);
void test_live(void);
```

## Rôle

Chaque macro incrémente `g_checks` et, en cas d'échec, `g_failures` **et**
affiche `FAIL fichier:ligne expression` — un test qui échoue ne s'arrête pas,
il s'accumule. `tests/native/main.c` lance les quatre suites dans l'ordre et
retourne `g_failures != 0`.

`CK_INT` compare en `long` pour éviter les warnings de comparaison
signé/non-signé sur `uint32_t` et `size_t`. `CK_STR` tolère un `NULL` côté
attendu comme côté réel et affiche `(null)` plutôt que de planter.

## Comment le lancer

```bash
gcc -std=c11 -O1 -Wall -Wextra -I . -o dist/native/t.exe \
  protocol/drsp.c protocol/sample_capture.c data/lifelog_record.c \
  data/watch_name_decoder.c data/battery_decoder.c data/time_decoder.c \
  data/live_state.c data/data_manager.c storage/export.c \
  tests/native/main.c tests/native/test_drsp.c tests/native/test_decoders.c \
  tests/native/test_export.c tests/native/test_live.c
./dist/native/t.exe
```

## Known limits

- Pas de timeout ni de détection de boucle infinie : un test bloqué bloque
  tout le binaire.
- Pas de désenregistrement dynamique : ajouter une suite impose d'éditer
  `main.c` et ce prototype.
- Aucune couverture des modules qui dépendent de l'API Flipper (`Storage`,
  GUI) ; ils ne sont pas compilables tels quels sur l'hôte.

## Status

- [x] Implemented
- [x] Tested — 186 checks, 0 échec sur les quatre suites

## Errors

_none yet_

## Corrections

_none yet_
