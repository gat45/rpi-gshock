#include "casio_lab.h"
#include "ui/main_menu.h"

#include <furi.h>

#define TAG "CasioLab"

static bool back_event_callback(void* context) {
    CasioLabApp* app = context;
    if(app->current_view == CasioLabViewMode) {
        view_dispatcher_switch_to_view(app->view_dispatcher, CasioLabViewSubmenu);
        app->current_view = CasioLabViewSubmenu;
        return true; /* handled: back goes to the menu, not out of the app */
    }
    return false; /* exit the app from the menu */
}

CasioLabApp* casio_lab_app_alloc(void) {
    CasioLabApp* app = malloc(sizeof(CasioLabApp));
    memset(app, 0, sizeof(CasioLabApp));

    app->gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, back_event_callback);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, CasioLabViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, CasioLabViewMode, widget_get_view(app->widget));

    casio_lab_menu_setup(app);

    return app;
}

void casio_lab_app_free(CasioLabApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, CasioLabViewMode);
    view_dispatcher_remove_view(app->view_dispatcher, CasioLabViewSubmenu);
    widget_free(app->widget);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    app->gui = NULL;
    free(app);
}

int32_t casio_lab_app(void* args) {
    UNUSED(args);
    CasioLabApp* app = casio_lab_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    casio_lab_app_free(app);
    return 0;
}