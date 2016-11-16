#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pdc-transform/pdc-transform.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "battery-layer.h"

static const uint8_t MARGIN_LEFT = 3;

typedef struct __attribute__((packed)) {
    GDrawCommandImage *battery_pdc;
    GDrawCommandImage *battery_charging_pdc;
    char buf[5];
    TextLayer *text_layer;
    BatteryChargeState state;
    EventHandle battery_state_event_handle;
    EventHandle settings_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);

    GSize size = gdraw_command_image_get_bounds_size(data->battery_pdc);
    GPoint offset = GPoint(bounds.size.w - size.w, PBL_IF_DISPLAY_LARGE_ELSE(1, 0));
    gdraw_command_image_draw(ctx, data->battery_pdc, offset);

    if (data->state.is_charging) {
        gdraw_command_image_draw(ctx, data->battery_charging_pdc, offset);
    } else {
        uint8_t percent = data->state.charge_percent > 0 ? data->state.charge_percent : 5;
        GColor color = colors_get_foreground_color();
#ifdef PBL_COLOR
        if (percent <= 10) {
            color = GColorRed;
        } else if (percent <= 20) {
            color = GColorYellow;
        }
#endif
        graphics_context_set_fill_color(ctx, color);

#ifdef PBL_DISPLAY_LARGE
        int w = 25 * percent / 100;
        graphics_fill_rect(ctx, GRect(offset.x + 3, offset.y + 10, w, 11), 0, GCornerNone);
#else
        int w = 18 * percent / 100;
        graphics_fill_rect(ctx, GRect(offset.x + 3, offset.y + 8, w, 8), 0, GCornerNone);
#endif
    }
}

static void battery_state_handler(BatteryChargeState state, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    snprintf(data->buf, sizeof(data->buf), "%d%%", state.charge_percent > 0 ? state.charge_percent : 5);
    memcpy(&data->state, &state, sizeof(BatteryChargeState));
    layer_mark_dirty(this);
}

static void settings_handler(void *this) {
    log_func();
    Data *data = layer_get_data(this);    
    pdc_transform_recolor_image(data->battery_pdc, colors_get_background_color(), colors_get_foreground_color());
    pdc_transform_recolor_image(data->battery_charging_pdc, colors_get_foreground_color(), colors_get_background_color());
    text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
    layer_mark_dirty(this);
}

BatteryLayer *battery_layer_create(GRect frame) {
    log_func();
    BatteryLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);

    data->battery_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY);
    data->battery_charging_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_BATTERY_CHARGING);

#ifdef PBL_DISPLAY_LARGE
    pdc_transform_scale_image(data->battery_pdc, 13);
    pdc_transform_scale_image(data->battery_charging_pdc, 13);
#endif

    GSize size = gdraw_command_image_get_bounds_size(data->battery_pdc);
    GRect rect = GRect(0, 0, bounds.size.w - size.w - 2, bounds.size.h);
    data->text_layer = text_layer_create(rect);
    text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_26, RESOURCE_ID_GILROY_LIGHT_18)));
    text_layer_set_background_color(data->text_layer, GColorClear);
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentRight);
    text_layer_set_text(data->text_layer, data->buf);
    layer_add_child(this, text_layer_get_layer(data->text_layer));

    battery_state_handler(battery_state_service_peek(), this);
    data->battery_state_event_handle = events_battery_state_service_subscribe_context(battery_state_handler, this);

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void battery_layer_destroy(BatteryLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    events_battery_state_service_unsubscribe(data->battery_state_event_handle);
    text_layer_destroy(data->text_layer);
    gdraw_command_image_destroy(data->battery_pdc);
    gdraw_command_image_destroy(data->battery_charging_pdc);
    layer_destroy(this);
}
