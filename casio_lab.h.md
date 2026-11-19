# casio_lab.h

**Path:** `casio_lab.h`

## Content

État initial (2026-11-17), conservé tel quel :

```c
typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget;
} CasioLabApp;
```

État réel (2026-11-19) :

```c
typedef enum {
    CasioLabViewSubmenu = 0,
    CasioLabViewMode,
    CasioLabViewCount,
} CasioLabView;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget;
    uint32_t current_view;   /* ajouté : nécessaire au retour BACK */
} CasioLabApp;

CasioLabApp* casio_lab_app_alloc(void);
void casio_lab_app_free(CasioLabApp* app);
```

## Rôle

Point d'entrée du FAP (`application.fam` → `casio_lab_app`). Deux vues
seulement : le sous-menu et un `Widget` réutilisé pour tous les écrans de
texte. Il n'y a pas de scène manager.

`current_view` existe parce que le callback de navigation doit savoir s'il est
sur un écran de mode (BACK → menu) ou sur le menu (BACK → quitter l'app).

## Known limits

- Une seule vue de mode : tous les écrans partagent le même `Widget`, donc
  aucune animation ni rafraîchissement en direct.
- `casio_lab_app_alloc()` ne vérifie pas `malloc` : une allocation nulle
  planterait. Acceptable sur Flipper (allocateur qui échoue = redémarrage),
  mais c'est une limite réelle.
- `view_dispatcher_run()` est bloquant jusqu'à la sortie de l'app.

## Status

- [x] Implemented
- [x] Tested on hardware — `ufbt launch` démarre l'app, `loader info` la voit
      en cours d'exécution, BACK depuis un mode revient au menu puis quitte

## Errors

- Circular include: casio_lab.h included ui/main_menu.h which included casio_lab.h. Resolved by removing the include from casio_lab.h.
- `on_exit` function name conflicted with stdlib's `on_exit` (int (*)(void (*)(int,void*), void*)). Caused implicit declaration / conflicting types.

## Corrections

- Removed `#include "ui/main_menu.h"` from casio_lab.h (main_menu.h still includes casio_lab.h).
- Renamed `on_exit` ��' `on_exit_scene` in main_menu.c.

Corrections du 2026-11-19 (historique conservé) :

- Le struct ci-dessus n'avait pas `current_view` : le BACK depuis un écran de
  mode sortait de l'application au lieu de revenir au menu. Champ ajouté, et
  `back_event_callback()` dans `casio_lab.c` l'utilise.
- Le statut « [ ] Implemented » était faux : l'app tourne depuis les premières
  sessions ; le statut est corrigé.
