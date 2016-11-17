#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pdc-transform/pdc-transform.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "weather.h"
#include "weather-layer.h"

typedef struct __attribute__((packed)) {
    GDrawCommandImage *weather_pdc;
    char buf[6];
    TextLayer *text_layer;
    EventHandle weather_event_handle;
    EventHandle settings_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    Data *data = layer_get_data(this);

    if (data->weather_pdc != NULL) {
        gdraw_command_image_draw(ctx, data->weather_pdc, GPoint(0, PBL_IF_DISPLAY_LARGE_ELSE(8, 6)));
    }
}

static void weather_handler(GenericWeatherInfo *info, GenericWeatherStatus status, void *context) {
    log_func();
    Data *data = layer_get_data(context);
#ifdef DEMO
    if (data->weather_pdc != NULL) {
        gdraw_command_image_destroy(data->weather_pdc);
    }
    data->weather_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY);
#ifdef PBL_DISPLAY_LARGE
    pdc_transform_scale_image(data->weather_pdc, 13);
#endif
    pdc_transform_recolor_image(data->weather_pdc, colors_get_background_color(), colors_get_foreground_color());
    snprintf(data->buf, sizeof(data->buf), "21°");
#else
    if (status == GenericWeatherStatusAvailable) {
        uint32_t resource_id;
        switch (info->condition) {
            case GenericWeatherConditionClearSky:
                resource_id = info->day ? RESOURCE_ID_WEATHER_CLEAR_DAY : RESOURCE_ID_WEATHER_CLEAR_NIGHT;
                break;
            case GenericWeatherConditionFewClouds:
                resource_id = info->day ? RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY : RESOURCE_ID_WEATHER_PARTLY_CLOUDY_NIGHT;
                break;
            case GenericWeatherConditionScatteredClouds:
            case GenericWeatherConditionBrokenClouds:
                resource_id = RESOURCE_ID_WEATHER_CLOUDY;
                break;
            case GenericWeatherConditionShowerRain:
            case GenericWeatherConditionRain:
                resource_id = RESOURCE_ID_WEATHER_RAIN;
                break;
            case GenericWeatherConditionThunderstorm:
                resource_id = RESOURCE_ID_WEATHER_THUNDERSTORM;
                break;
            case GenericWeatherConditionSnow:
                resource_id = RESOURCE_ID_WEATHER_SNOW;
                break;
            default:
                resource_id = RESOURCE_ID_WEATHER_GENERIC;
                break;
        }
        if (data->weather_pdc != NULL) {
            gdraw_command_image_destroy(data->weather_pdc);
        }
        data->weather_pdc = gdraw_command_image_create_with_resource(resource_id);
#ifdef PBL_DISPLAY_LARGE
        pdc_transform_scale_image(data->weather_pdc, 13);
#endif
        pdc_transform_recolor_image(data->weather_pdc, colors_get_background_color(), colors_get_foreground_color());
        int unit = atoi(enamel_get_WEATHER_UNIT());
        snprintf(data->buf, sizeof(data->buf), "%d°", unit == 1 ? info->temp_f : info->temp_c);
    } else if (status != GenericWeatherStatusPending) {
        snprintf(data->buf, sizeof(data->buf), "--");
    }
#endif
    layer_mark_dirty(context);
}

static void settings_handler(void *context) {
    log_func();
    Data *data = layer_get_data(context);
#ifndef DEMO
    GenericWeatherInfo *info = weather_peek();
    int unit = atoi(enamel_get_WEATHER_UNIT());
    snprintf(data->buf, sizeof(data->buf), "%d°", unit == 1 ? info->temp_f : info->temp_c);
#endif

    text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
    pdc_transform_recolor_image(data->weather_pdc, colors_get_background_color(), colors_get_foreground_color());
    layer_mark_dirty(context);
}

WeatherLayer *weather_layer_create(GRect frame) {
    log_func();
    WeatherLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);

    data->weather_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_WEATHER_GENERIC);
#ifdef PBL_DISPLAY_LARGE
    pdc_transform_scale_image(data->weather_pdc, 13);
#endif
    pdc_transform_recolor_image(data->weather_pdc, colors_get_background_color(), colors_get_foreground_color());

    GSize size = gdraw_command_image_get_bounds_size(data->weather_pdc);
    GRect rect = GRect(size.w + 3, 5, bounds.size.w - size.w - 3, bounds.size.h - 5);
    data->text_layer = text_layer_create(rect);
    text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_30, RESOURCE_ID_GILROY_LIGHT_22)));
    text_layer_set_background_color(data->text_layer, GColorClear);
    text_layer_set_text_alignment(data->text_layer, GTextAlignmentCenter);
    text_layer_set_text(data->text_layer, data->buf);
    layer_add_child(this, text_layer_get_layer(data->text_layer));

    weather_handler(weather_peek(), GenericWeatherStatusAvailable, this);
#ifndef DEMO
    data->weather_event_handle = events_weather_subscribe(weather_handler, this);
#endif

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void weather_layer_destroy(WeatherLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
#ifndef DEMO
    events_weather_unsubscribe(data->weather_event_handle);
#endif
    text_layer_destroy(data->text_layer);
    if (data->weather_pdc != NULL) {
        gdraw_command_image_destroy(data->weather_pdc);
    }
    layer_destroy(this);
}
