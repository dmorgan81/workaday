#include "common.h"
#include "colors.h"
#include "top-layer.h"
#ifndef PBL_PLATFORM_APLITE
#include "quiet-time-layer.h"
#endif
#include "connection-layer.h"
#include "date-layer.h"
#include "battery-layer.h"

typedef struct __attribute__((packed)) {
#ifndef PBL_PLATFORM_APLITE
    QuietTimeLayer *quiet_time_layer;
#endif
    ConnectionLayer *connection_layer;
    DateLayer *date_layer;
    BatteryLayer *battery_layer;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);

    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    graphics_draw_line(ctx, GPoint(0, TOP_LAYER_HEIGHT - 1), GPoint(bounds.size.w, TOP_LAYER_HEIGHT - 1));
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, 0), GPoint(bounds.size.w / 2, TOP_LAYER_HEIGHT));
}

TopLayer *top_layer_create(GRect frame) {
    log_func();
    TopLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);
    uint8_t width = bounds.size.w / 2;

#ifndef PBL_PLATFORM_APLITE
    data->quiet_time_layer = quiet_time_layer_create(GRect(0, 0, PBL_IF_DISPLAY_LARGE_ELSE(14, 10), TOP_LAYER_HEIGHT));
    layer_add_child(this, data->quiet_time_layer);
#endif

    data->connection_layer = connection_layer_create(GRect(0, 0, width, TOP_LAYER_HEIGHT));
    layer_add_child(this, data->connection_layer);

    data->date_layer = date_layer_create(GRect(0, 0, width - 4, TOP_LAYER_HEIGHT));
    layer_add_child(this, data->date_layer);

    data->battery_layer = battery_layer_create(GRect(width + 4, 0, width - 4, TOP_LAYER_HEIGHT));
    layer_add_child(this, data->battery_layer);

    return this;
}

void top_layer_destroy(TopLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    battery_layer_destroy(data->battery_layer);
    date_layer_destroy(data->date_layer);
    connection_layer_destroy(data->connection_layer);
#ifndef PBL_PLATFORM_APLITE
    quiet_time_layer_destroy(data->quiet_time_layer);
#endif
    layer_destroy(this);
}
