#ifdef PBL_HEALTH
#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pdc-transform/pdc-transform.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "sleep-layer.h"

typedef struct __attribute__((packed)) {
    GDrawCommandImage *sleep_pdc;
    char buf[6];
    TextLayer *text_layer;
    EventHandle health_event_handle;
    EventHandle settings_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);

    GSize size = gdraw_command_image_get_bounds_size(data->sleep_pdc);
    GPoint offset = GPoint(bounds.size.w - size.w, 5);
    gdraw_command_image_draw(ctx, data->sleep_pdc, offset);
}

static void health_event_handler(HealthEventType event, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    if (event == HealthEventSignificantUpdate) {
        health_event_handler(HealthEventSleepUpdate, this);
    } else if (event == HealthEventSleepUpdate) {
        time_t start = time_start_of_today();
        time_t end = time(NULL);

        HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricSleepSeconds, start, end);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue sleep = health_service_sum_today(HealthMetricSleepSeconds);
            int minutes = sleep / 60;
            int hours = minutes / 60;
            minutes %= 60;

            struct tm t = {
                .tm_hour = hours,
                .tm_min = minutes
            };
            strftime(data->buf, sizeof(data->buf), "%k:%M", &t);
        } else {
            snprintf(data->buf, sizeof(data->buf), "0:00");
        }
        layer_mark_dirty(this);
    }
}

static void settings_handler(void *this) {
    log_func();
    Data *data = layer_get_data(this);
    text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
    pdc_transform_recolor_image(data->sleep_pdc, colors_get_foreground_color(), colors_get_background_color());
}

SleepLayer *sleep_layer_create(GRect frame) {
    log_func();
    SleepLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);

    data->sleep_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_HEALTH_SLEEP);
#ifdef PBL_DISPLAY_LARGE
    pdc_transform_scale_image(data->sleep_pdc, 13);
#endif

    uint8_t y = PBL_IF_DISPLAY_LARGE_ELSE(7, 8);
    GSize size = gdraw_command_image_get_bounds_size(data->sleep_pdc);
    GRect rect = GRect(0, y, bounds.size.w - size.w - 2, bounds.size.h - y);
    data->text_layer = text_layer_create(rect);
    text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_25, RESOURCE_ID_GILROY_LIGHT_18)));
    text_layer_set_background_color(data->text_layer, GColorClear);
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentRight);
    text_layer_set_text(data->text_layer, data->buf);
    layer_add_child(this, text_layer_get_layer(data->text_layer));

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    health_event_handler(HealthEventSignificantUpdate, this);
    data->health_event_handle = events_health_service_events_subscribe(health_event_handler, this);

    return this;
}

void sleep_layer_destroy(SleepLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_health_service_events_unsubscribe(data->health_event_handle);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    text_layer_destroy(data->text_layer);
    gdraw_command_image_destroy(data->sleep_pdc);
    layer_destroy(this);
}

#endif
