#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>

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
    uint32_t current_view;
} CasioLabApp;

CasioLabApp* casio_lab_app_alloc(void);
void casio_lab_app_free(CasioLabApp* app);