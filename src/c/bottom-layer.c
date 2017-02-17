#include "common.h"
#include <pebble-events/pebble-events.h>
#include "colors.h"
#include "bottom-layer.h"
#include "weather-layer.h"
#ifdef PBL_HEALTH
#include "step-layer.h"
#endif

typedef struct __attribute__((packed)) {
    WeatherLayer *weather_layer;
#ifdef PBL_HEALTH
    StepLayer *step_layer;
#endif
#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
    EventHandle unobstructed_area_event_handle;
    Layer *window_layer;
#endif
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);

    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
#ifdef PBL_HEALTH
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, 0), GPoint(bounds.size.w / 2, bounds.size.h));
#endif
}

#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
static void unobstructed_area_change_handler(AnimationProgress progress, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_unobstructed_bounds(data->window_layer);

    GRect frame = layer_get_frame(this);
    frame.origin.y = bounds.size.h - BOTTOM_LAYER_HEIGHT;
    layer_set_frame(this, frame);
}

static void unobstructed_area_wil_change_handler(GRect final_unobstructed_screen_area, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    if (data->window_layer == NULL) {
        Window *window = layer_get_window(context);
        data->window_layer = window_get_root_layer(window);
    }
}
#endif

BottomLayer *bottom_layer_create(GRect frame) {
    log_func();
    BottomLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);
    uint8_t width = bounds.size.w / 2;

#ifdef PBL_HEALTH
    data->weather_layer = weather_layer_create(GRect(0, 0, width, bounds.size.h));
#else
    data->weather_layer = weather_layer_create(GRect(width / 2, 0, width, bounds.size.h));
#endif
    layer_add_child(this, data->weather_layer);

#ifdef PBL_HEALTH
    data->step_layer = step_layer_create(GRect(width, 0, width, bounds.size.h));
    layer_add_child(this, data->step_layer);
#endif

#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
    data->unobstructed_area_event_handle = events_unobstructed_area_service_subscribe((UnobstructedAreaHandlers) {
        .change = unobstructed_area_change_handler,
        .will_change = unobstructed_area_wil_change_handler
    }, this);
#endif

    return this;
}

void bottom_layer_destroy(BottomLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
    events_unobstructed_area_service_unsubscribe(data->unobstructed_area_event_handle);
#endif
#ifdef PBL_HEALTH
    step_layer_destroy(data->step_layer);
#endif
    weather_layer_destroy(data->weather_layer);
    layer_destroy(this);
}
