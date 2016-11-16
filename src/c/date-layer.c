#include "common.h"
#include <pebble-events/pebble-events.h>
#include <@smallstoneapps/sliding-text-layer/sliding-text-layer.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "date-layer.h"

typedef struct __attribute__((packed)) {
    char buf_date[7];
    char buf_wday[4];
    SlidingTextLayer *text_layer;
    EventHandle tick_timer_event_handle;
    EventHandle tap_event_handle;
    EventHandle settings_event_handle;
    AppTimer *tap_timer;
} Data;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    strftime(data->buf_date, sizeof(data->buf_date), "%b %d", tick_time);
    strftime(data->buf_wday, sizeof(data->buf_wday), "%a", tick_time);
    layer_mark_dirty(context);
}

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

static void settings_handler(void *context) {
    log_func();
    Data *data = layer_get_data(context);
    sliding_text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
}

DateLayer *date_layer_create(GRect frame) {
    log_func();
    DateLayer *this = layer_create_with_data(frame, sizeof(Data));
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);

    data->text_layer = sliding_text_layer_create(bounds);
    sliding_text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_26, RESOURCE_ID_GILROY_LIGHT_18)));
    sliding_text_layer_set_text_alignment(data->text_layer, GTextAlignmentRight);
    sliding_text_layer_set_text(data->text_layer, data->buf_date);
    sliding_text_layer_set_duration(data->text_layer, 500);
    layer_add_child(this, data->text_layer);

    time_t now = time(NULL);
    tick_handler(localtime(&now), DAY_UNIT, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(DAY_UNIT, tick_handler, this);

    data->tap_event_handle = events_accel_tap_service_subscribe_context(tap_handler, this);

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void date_layer_destroy(DateLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    events_accel_tap_service_unsubscribe(data->tap_event_handle);
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    sliding_text_layer_destroy(data->text_layer);
    layer_destroy(this);
}
