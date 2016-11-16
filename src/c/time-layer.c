#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include "colors.h"
#include "time-layer.h"

typedef struct __attribute__((packed)) {
    char buf[6];
    FFont *font;
    EventHandle settings_event_handle;
    EventHandle tick_timer_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_unobstructed_bounds(this);
    FPoint center = FPointI(bounds.size.w / 2, bounds.size.h / 2 - PBL_IF_DISPLAY_LARGE_ELSE(7, 5));
    fixed_t width = INT_TO_FIXED(bounds.size.w);

    int16_t font_size = PBL_IF_DISPLAY_LARGE_ELSE(80, 60);

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    fixed_t str_width;
    do {
        fctx_set_text_em_height(&fctx, data->font, font_size--);
        str_width = fctx_string_width(&fctx, data->buf, data->font);
    } while (str_width > width);

    fctx_set_text_em_height(&fctx, data->font, ++font_size);
    fctx_set_color_bias(&fctx, 0);
    fctx_set_fill_color(&fctx, colors_get_foreground_color());
    fctx_set_offset(&fctx, center);

    fctx_begin_fill(&fctx);
    fctx_draw_string(&fctx, data->buf, data->font, GTextAlignmentCenter, FTextAnchorMiddle);
    fctx_end_fill(&fctx);

    fctx_deinit_context(&fctx);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    if (enamel_get_LEADING_ZERO()) {
        strftime(data->buf, sizeof(data->buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    } else {
        strftime(data->buf, sizeof(data->buf), clock_is_24h_style() ? "%k:%M" : "%l:%M", tick_time);
    }
    layer_mark_dirty(this);
}

static void settings_handler(void *this) {
    log_func();
    time_t now = time(NULL);
    tick_handler(localtime(&now), MINUTE_UNIT, this);
}

TimeLayer *time_layer_create(GRect frame) {
    log_func();
    TimeLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = ffont_create_from_resource(RESOURCE_ID_GILROY_LIGHT_FFONT);

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(MINUTE_UNIT, tick_handler, this);

    return this;
}

void time_layer_destroy(TimeLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    ffont_destroy(data->font);
    layer_destroy(this);
}
