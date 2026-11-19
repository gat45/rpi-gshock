# sample_data.h

**Path:** `protocol/sample_data.h`

## Content (2026-11-19)

En-tête **généré** — `tools/gen_sample_capture.py`, ne pas éditer à la main.

```c
typedef struct {
    const uint8_t* data;
    uint16_t len;
} CasioSampleFrame;

static const uint8_t kSampleFlowChunk0[];   /* 0x05 + payload + CRC16-KERMIT */
static const uint8_t kSampleFlowChunk1[];
static const uint8_t kSampleFlowChunk2[];
static const CasioSampleFrame kSampleFlowChunks[];

#define CASIO_SAMPLE_NAME_COUNT 2u
static const CasioSampleFrame kSampleNames[];

#define CASIO_SAMPLE_CONDITION_COUNT 3u
static const CasioSampleFrame kSampleConditions[];
```

## Provenance

Octets réels extraits des captures btsnoop du DW-H5600, sans réécriture :

- 3 chunks du flux LifeLog sur `0x0013` (305 octets de fil au total) ;
- 2 réponses de nom `0x23` sur `0x0018` ;
- 3 réponses de condition `0x28` sur `0x0018` (raw 6/26 °C, 6/27 °C,
  7/27 °C).

## Ce à quoi ça sert

Sur Momentum le FAP ne peut pas être central. Ces octets sont la seule preuve
matérielle que les décodeurs tournent sur autre chose que des données
inventées : `protocol/sample_capture.c` les passe dans `drsp_*` et les
décodeurs, `tests/native/test_decoders.c` vérifie les mêmes nombres sur l'hôte.

## Known limits

- Le flux embarqué s'arrête après 1 record `0x00` + 1 enregistrement `0x65`
  (124 octets de payload) ; la désynchronisation réelle à l'offset 124 est
  incluse et le resync est compté (`resync_bytes`).
- Les 3 réponses `0x28` ne couvrent qu'une petite plage de batteries ; les
  limites `(lower, upper)` du DW-H5600 restent non confirmées.
- Aucune trame d'heure `0x09` réelle : ce décodeur n'est pas couvert ici.
- Chaque tableau est `static const` : l'inclure dans plusieurs unités de
  compilation duplique les données.

## Status

- [x] Generated — reproductible par `python tools/gen_sample_capture.py`
- [x] Tested — `tests/native/test_drsp.c` vérifie le nombre de chunks,
      d'octets, de records, et `tests/native/test_decoders.c` les valeurs
      décodées ; même chemin exécuté sur l'appareil via SAMPLE

## Errors

_none yet_

## Corrections

_none yet_
