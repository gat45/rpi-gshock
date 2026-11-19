# main_menu.h

**Path:** `ui/main_menu.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
#pragma once

#include "../casio_lab.h"

/* Initialize submenu and show it. Returns nothing. */
void casio_lab_menu_setup(CasioLabApp* app);
```

État réel (2026-11-19) : identique. Implémentation dans `ui/main_menu.c`
(9,8 Ko).

## Ce que fait réellement l'écran

Menu à 8 entrées : `CasioLab v0.2`, `REVERSE`, `DATA`, `CONTROL`, `MONITOR`,
`PROXY`, `SAMPLE`, `STORAGE`.

- **REVERSE** : compte les trames mesurées (`kObservedCommands`) et affiche
  l'état de protocole.
- **DATA** : tente `load_live()` (`/ext/casiolab/live.txt`), puis, si le
  document est absent ou invalide, se replie sur `load_sample()`.
- **CONTROL** : affiche la politique de sécurité (`cmd_count()`) ; n'exécute
  rien.
- **MONITOR** : état de protocole + compteurs de trames décodées/refusées.
- **PROXY** : écran d'information ; le proxy n'est pas implémenté (le FAP ne
  peut pas être central).
- **SAMPLE** : `load_sample()` — décode les 3 chunks réels embarqués.
- **STORAGE** : crée une session et exporte CSV/JSON, en préférant le document
  live au sample.

`load_live()` fait `data_reset()` avant `data_apply_live()` : un document live
valide est un instantané autoritaire, il ne doit pas hériter du record du
sample.

## Known limits

- `SAMPLE` et `STORAGE` écrivent dans l'état global `data_manager` : après un
  `SAMPLE`, un `STORAGE` sans live ré-exporte le sample. C'est voulu (le sample
  est la seule source de valeurs réelles sur l'appareil) mais ce n'est pas un
  instantané de la montre.
- Les textes sont construits dans des buffers de 320/512 octets.
- Le buffer de l'écran STORAGE liste les fichiers d'une session, pas l'arbre
  complet des sessions.

## Status

- [x] Implemented
- [x] Tested on hardware — menu navigable, BACK revient au menu, DATA et
      STORAGE ont produit les sessions `session_0001..0013` et affiché le
      repli sample (`record` présent, `frames: 6`)

## Errors

- `GuiButtonTypeBack` undeclared: enum only has GuiButtonTypeLeft/Center/Right.
- `on_exit` name conflict with stdlib.
- Scene manager handlers caused unused warnings and circular includes.
- `widget_add_button_element` with NULL callback compiled but button did nothing.

## Corrections

- Removed button element; use hardware BACK via view_dispatcher navigation callback.
- Renamed on_exit ��' on_exit_scene.
- Removed scene manager; use direct submenu + widget views.
- Back navigation handled in casio_lab.c back_event_callback.

Corrections du 2026-11-19 (au-dessus, historique conservé) :

- Le menu est passé de 6 à 8 entrées ; « 6 modes » dans les anciens documents
  était périmé.
- DATA et STORAGE lisent maintenant `/ext/casiolab/live.txt` avant de se
  replier sur le sample ; l'ancienne version n'affichait que le sample.
- L'écran STORAGE affiche `ok` / `fail` selon le retour réel des exports, au
  lieu de supposer que l'écriture a réussi.
