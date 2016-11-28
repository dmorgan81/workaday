#include "common.h"
#include <pebble-events/pebble-events.h>
#ifndef PBL_PLATFORM_APLITE
#include <@smallstoneapps/sliding-text-layer/sliding-text-layer.h>
#endif
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "date-layer.h"

typedef struct __attribute__((packed)) {
    char buf_date[7];
#ifndef PBL_PLATFORM_APLITE
    char buf_wday[4];
    SlidingTextLayer *text_layer;
#else
    TextLayer *text_layer;
#endif
    EventHandle tick_timer_event_handle;
    EventHandle settings_event_handle;
#ifndef PBL_PLATFORM_APLITE
    EventHandle tap_event_handle;
    AppTimer *tap_timer;
#endif
} Data;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    strftime(data->buf_date, sizeof(data->buf_date), "%b %d", tick_time);
#ifndef PBL_PLATFORM_APLITE
    strftime(data->buf_wday, sizeof(data->buf_wday), "%a", tick_time);
#endif
    layer_mark_dirty(context);
}

#ifndef PBL_PLATFORM_APLITE
static void app_timer_callback(void *context) {
    log_func();
    Data *data = layer_get_data(context);
    data->tap_timer = NULL;
    sliding_text_layer_set_next_text(data->text_layer, data->buf_date);
    sliding_text_layer_animate_up(data->text_layer);
}

static void tap_handler(AccelAxisType axis, int32_t direction, void *context) {
    log_func();
    if (axis > 0) {
        Data *data = layer_get_data(context);
        if (data->tap_timer == NULL) {
            sliding_text_layer_set_next_text(data->text_layer, data->buf_wday);
            sliding_text_layer_animate_down(data->text_layer);
            data->tap_timer = app_timer_register(4000, app_timer_callback, context);
        }
    }
}
#endif

static void settings_handler(void *context) {
    log_func();
    Data *data = layer_get_data(context);
#ifndef PBL_PLATFORM_APLITE
    sliding_text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
#else
    text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
#endif
}

DateLayer *date_layer_create(GRect frame) {
    log_func();
    DateLayer *this = layer_create_with_data(frame, sizeof(Data));
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);
    uint8_t x = PBL_IF_DISPLAY_LARGE_ELSE(15, 10);

#ifndef PBL_PLATFORM_APLITE
    data->text_layer = sliding_text_layer_create(GRect(x, 1, bounds.size.w - x, bounds.size.h - 1));
    sliding_text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_25, RESOURCE_ID_GILROY_LIGHT_18)));
    sliding_text_layer_set_text(data->text_layer, data->buf_date);
    sliding_text_layer_set_duration(data->text_layer, 500);
    layer_add_child(this, data->text_layer);
#else
    data->text_layer = text_layer_create(GRect(x, 1, bounds.size.w - x, bounds.size.h - 1));
    text_layer_set_font(data->text_layer, lazy_fonts_get(RESOURCE_ID_GILROY_LIGHT_18));
    text_layer_set_text(data->text_layer, data->buf_date);
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(data->text_layer, GColorClear);
    layer_add_child(this, text_layer_get_layer(data->text_layer));
#endif

    time_t now = time(NULL);
    tick_handler(localtime(&now), DAY_UNIT, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(DAY_UNIT, tick_handler, this);

#ifndef PBL_PLATFORM_APLITE
    data->tap_event_handle = events_accel_tap_service_subscribe_context(tap_handler, this);
#endif

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void date_layer_destroy(DateLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
#ifndef PBL_PLATFORM_APLITE
    events_accel_tap_service_unsubscribe(data->tap_event_handle);
#endif
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
#ifndef PBL_PLATFORM_APLITE
    sliding_text_layer_destroy(data->text_layer);
#else
    text_layer_destroy(data->text_layer);
#endif
    layer_destroy(this);
}
