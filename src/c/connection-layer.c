#include "common.h"
#include <pebble-events/pebble-events.h>
#include "colors.h"
#include "connection-layer.h"

static const GPathInfo PATH_INFO = {
    .num_points = 7,
#ifdef PBL_DISPLAY_LARGE
    .points = (GPoint []) {{0, 11}, {13, 24}, {6, 30}, {6, 5}, {13, 11}, {0, 24}, {6, 18}}
#else
    .points = (GPoint []) {{0, 8}, {8, 16}, {4, 20}, {4, 4}, {8, 8}, {0, 16}, {4, 12}}
#endif
};

typedef struct {
    GPath *gpath;
    EventHandle connection_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    if (quiet_time_is_active()) return;
    Data *data = layer_get_data(this);
    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    gpath_draw_outline(ctx, data->gpath);
}

static void connection_handler(bool connected, void *context) {
    log_func();
    layer_set_hidden(context, !connected);
}

ConnectionLayer *connection_layer_create(GRect frame) {
    log_func();
    ConnectionLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->gpath = gpath_create(&PATH_INFO);

    connection_handler(connection_service_peek_pebble_app_connection(), this);
    data->connection_event_handle = events_connection_service_subscribe_context((EventConnectionHandlers) {
        .pebble_app_connection_handler = connection_handler
    }, this);

    return this;
}

void connection_layer_destroy(ConnectionLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_connection_service_unsubscribe(data->connection_event_handle);
    gpath_destroy(data->gpath);
    layer_destroy(this);
}
