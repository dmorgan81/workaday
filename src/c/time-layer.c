#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include "colors.h"
#include "time-layer.h"

typedef struct __attribute__((packed)) {
    char buf[9];
    FFont *font;
    EventHandle settings_event_handle;
    EventHandle tick_timer_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_unobstructed_bounds(this);
    FPoint center = FPointI(bounds.size.w / 2, bounds.size.h / 2 - PBL_IF_DISPLAY_LARGE_ELSE(7, 5));

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    int16_t font_size = PBL_IF_DISPLAY_LARGE_ELSE(80, 60);
    fixed_t str_width;
    fixed_t width = INT_TO_FIXED(bounds.size.w);

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
#ifndef DEMO
    char s[6];
    if (enamel_get_LEADING_ZERO()) {
        snprintf(s, sizeof(s), clock_is_24h_style() ? "%%H:%%M" : "%%I:%%M");
    } else {
        snprintf(s, sizeof(s), clock_is_24h_style() ? "%%k:%%M" : "%%l:%%M");
    }
    char t[10];
    snprintf(t, sizeof(t), "%s%s", s, enamel_get_SHOW_SECONDS() ? ":%S" : "");
    strftime(data->buf, sizeof(data->buf), t, tick_time);
#else
    snprintf(data->buf, sizeof(data->buf), "12:34");
#endif
    layer_mark_dirty(this);
}

static void settings_handler(void *this) {
    log_func();
    Data *data = layer_get_data(this);

    if (data->tick_timer_event_handle) events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);

    time_t now = time(NULL);
    TimeUnits units = enamel_get_SHOW_SECONDS() ? SECOND_UNIT : MINUTE_UNIT;
    tick_handler(localtime(&now), units, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(units, tick_handler, this);
}

TimeLayer *time_layer_create(GRect frame) {
    log_func();
    TimeLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = ffont_create_from_resource(RESOURCE_ID_GILROY_LIGHT_FFONT);

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void time_layer_destroy(TimeLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    if (data->tick_timer_event_handle) events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    ffont_destroy(data->font);
    layer_destroy(this);
}
