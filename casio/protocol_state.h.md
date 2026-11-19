# protocol_state.h

**Path:** `casio/protocol_state.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once
/* protocol_state - stub (Phase 0) */
```

État réel (2026-11-19) :

```c
typedef enum {
    PROTO_STATE_DISCONNECTED = 0,
    PROTO_STATE_CONNECTING,
    PROTO_STATE_CONNECTED,
    PROTO_STATE_DISCOVERING,
    PROTO_STATE_READY,
    PROTO_STATE_ERROR,
} ProtocolState;

const char* proto_state_name(ProtocolState s);
ProtocolState proto_state_get(void);
void proto_state_set(ProtocolState s);
```

## Rôle

Une seule variable globale, lue par les écrans REVERSE et MONITOR. C'est un
affichage, pas une machine à états : aucune transition n'est imposée, aucune
horloge, aucun événement.

Sur Momentum le FAP ne peut pas être central, donc l'état reste
`Disconnected` en pratique. C'est exact et non trompeur : il n'y a pas de
session BLE à représenter. Le companion hôte a son propre état, hors FAP.

## Known limits

- Pas thread-safe (variable globale). L'UI est mono-thread.
- `proto_state_name()` retourne `"Unknown"` pour une valeur hors énumération
  plutôt que de lire hors bornes.
- Aucune persistance : l'état repart à `DISCONNECTED` à chaque lancement.

## Status

- [x] Implemented
- [ ] Tested — aucune assertion native ; le seul usage est un affichage
      (`proto_state_name(proto_state_get())`) visible sur l'appareil

## Errors

_none yet_

## Corrections

- La version « stub (Phase 0) » laissait `proto_state_get()` et
  `proto_state_name()` non définis alors que `ui/main_menu.c` les appelait :
  l'édition de liens aurait échoué. Corrigé par `protocol_state.c`.
