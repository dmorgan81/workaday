#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"

#include "time-layer.h"
#include "top-layer.h"
#include "bottom-layer.h"

static Window *s_window;
static TimeLayer *s_time_layer;
static TopLayer *s_top_layer;
static BottomLayer *s_bottom_layer;

static EventHandle s_settings_event_handle;

static void settings_handler(void *context) {
    log_func();
    window_set_background_color(s_window, colors_get_background_color());
    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());
}

static void window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    s_time_layer = time_layer_create(bounds);
    layer_add_child(root_layer, s_time_layer);

    s_top_layer = top_layer_create(bounds);
    layer_add_child(root_layer, s_top_layer);

    s_bottom_layer = bottom_layer_create(GRect(0, bounds.size.h - BOTTOM_LAYER_HEIGHT, bounds.size.w, BOTTOM_LAYER_HEIGHT));
    layer_add_child(root_layer, s_bottom_layer);

    settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(settings_handler, NULL);
}

static void window_unload(Window *window) {
    log_func();
    enamel_settings_received_unsubscribe(s_settings_event_handle);

    bottom_layer_destroy(s_bottom_layer);
    top_layer_destroy(s_top_layer);
    time_layer_destroy(s_time_layer);
}

static void init(void) {
    log_func();
    enamel_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });
    lazy_fonts_init();

#ifdef PBL_HEALTH
    connection_vibes_enable_health(true);
    hourly_vibes_enable_health(true);
#endif

    events_app_message_open();

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}

static void deinit(void) {
    log_func();
    window_destroy(s_window);

    lazy_fonts_deinit();
    hourly_vibes_deinit();
    connection_vibes_deinit();
    enamel_deinit();
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
