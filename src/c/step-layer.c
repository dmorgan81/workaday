#ifdef PBL_HEALTH
#include "common.h"
#include <pebble-events/pebble-events.h>
#include <@smallstoneapps/sliding-text-layer/sliding-text-layer.h>
#include <pdc-transform/pdc-transform.h>
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "step-layer.h"

typedef struct __attribute__((packed)) {
    GDrawCommandImage *steps_pdc;
    char buf_steps[7];
    char buf_distance[8];
    SlidingTextLayer *text_layer;
    HealthValue goal;
    HealthValue steps;
    HealthValue typical;
    EventHandle health_event_handle;
    EventHandle tap_event_handle;
    EventHandle settings_event_handle;
    AppTimer *tap_timer;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);

    GSize size = gdraw_command_image_get_bounds_size(data->steps_pdc);
    GPoint offset = GPoint(bounds.size.w - size.w, PBL_IF_DISPLAY_LARGE_ELSE(3, 2));
    gdraw_command_image_draw(ctx, data->steps_pdc, offset);

    int h = PBL_IF_DISPLAY_LARGE_ELSE(8, 6);
    GRect rect = GRect(bounds.origin.x + 6, bounds.size.h - h - 2, bounds.size.w - 6, h);
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, colors_get_foreground_color());
    graphics_fill_rect(ctx, grect_crop(rect, -1), h + 1, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, rect, h, GCornersAll);
#else
    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    graphics_draw_round_rect(ctx, rect, h);
    graphics_context_set_stroke_color(ctx, colors_get_background_color());
    graphics_draw_round_rect(ctx, grect_crop(rect, 1), h);
#endif

    if (data->goal > 0 && data->steps > 0) {
        HealthValue steps = data->steps;
        HealthValue goal = data->goal < data->steps ? data->steps : data->goal;
        HealthValue typical = data->typical > goal ? goal : data->typical;

        uint8_t percent = (steps * 100) / goal;
        uint8_t w = rect.size.w * percent / 100;
        graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorDarkGray));
        GRect inside = GRect(rect.origin.x, rect.origin.y, w, h);
        graphics_fill_rect(ctx, inside, h, steps < typical ? GCornersLeft : GCornersAll);

        if (data->typical > 0) {
            graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, colors_get_foreground_color()));
            percent = (typical * 100) / goal;
            uint8_t t = rect.size.w * percent / 100;
            if (steps < typical) {
                int len = t - w;
                GRect typical = GRect(rect.origin.x + w, rect.origin.y, len < 2 ? 2 : len, h);
                graphics_fill_rect(ctx, typical, h, GCornersRight);
            } else {
                GRect typical = GRect(rect.origin.x + t, rect.origin.y, 2, h);
                graphics_fill_rect(ctx, typical, 0, GCornerNone);
            }
        }
    }
}

static void health_event_handler(HealthEventType event, void *context) {
    log_func();
    Data *data = layer_get_data(context);
#ifdef DEMO
    snprintf(data->buf_steps, sizeof(data->buf_steps), "4.5k");
    layer_mark_dirty(context);
    data->steps = PBL_IF_COLOR_ELSE(5, 6);
    data->goal = 10;
    data->typical = PBL_IF_COLOR_ELSE(6, 5);
#else
    if (event == HealthEventSignificantUpdate) {
        time_t start = time_start_of_today();
        time_t end = start + SECONDS_PER_DAY;
        HealthServiceAccessibilityMask mask = health_service_metric_averaged_accessible(HealthMetricStepCount, start, end, HealthServiceTimeScopeDailyWeekdayOrWeekend);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            data->goal = health_service_sum_averaged(HealthMetricStepCount, start, end, HealthServiceTimeScopeDailyWeekdayOrWeekend);
            logd("goal: %ld", data->goal);
            health_event_handler(HealthEventMovementUpdate, context);
        } else {
            data->goal = 0;
        }
        layer_mark_dirty(context);
    } else if (event == HealthEventMovementUpdate) {
        time_t start = time_start_of_today();
        time_t end = time(NULL);

        HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue steps = health_service_sum_today(HealthMetricStepCount);
            data->steps = steps;
            logd("steps: %ld", data->steps);
            if (steps < 1000) {
                snprintf(data->buf_steps, sizeof(data->buf_steps), "%ld", steps);
            } else {
                int t = steps / 1000;
                int h = steps / 100 % 10;
                snprintf(data->buf_steps, sizeof(data->buf_steps), "%d.%dk", t, h);
            }
        } else {
            snprintf(data->buf_steps, sizeof(data->buf_steps), "0");
            data->steps = 0;
        }

        mask = health_service_metric_averaged_accessible(HealthMetricStepCount, start, end, HealthServiceTimeScopeWeekly);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            data->typical = health_service_sum_averaged(HealthMetricStepCount, start, end, HealthServiceTimeScopeWeekly);
            logd("typical: %ld", data->typical);
        } else {
            data->typical = 0;
        }

        mask = health_service_metric_accessible(HealthMetricWalkedDistanceMeters, start, end);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue distance = health_service_sum_today(HealthMetricWalkedDistanceMeters);
            MeasurementSystem system = health_service_get_measurement_system_for_display(HealthMetricWalkedDistanceMeters);
            if (system != MeasurementSystemImperial) {
                if (distance < 100) {
                    snprintf(data->buf_distance, sizeof(data->buf_distance), "%ldm", distance);
                } else if (distance < 1000) {
                    distance /= 100;
                    snprintf(data->buf_distance, sizeof(data->buf_distance), ".%ldkm", distance);
                } else {
                    distance /= 1000;
                    snprintf(data->buf_distance, sizeof(data->buf_distance), "%ldkm", distance);
                }
            } else {
                int tenths = distance * 10 / 1609 % 10;
                int whole = distance / 1609;
                if (whole < 10) {
                    snprintf(data->buf_distance, sizeof(data->buf_distance), "%d.%dmi", whole, tenths);
                } else {
                    snprintf(data->buf_distance, sizeof(data->buf_distance), "%dmi", whole);
                }
            }
        } else {
            snprintf(data->buf_distance, sizeof(data->buf_distance), "0");
        }
        layer_mark_dirty(context);
    }
#endif
}

static void app_timer_callback(void *context) {
    log_func();
    Data *data = layer_get_data(context);
    data->tap_timer = NULL;
    sliding_text_layer_set_next_text(data->text_layer, data->buf_steps);
    sliding_text_layer_animate_up(data->text_layer);
}

static void tap_handler(AccelAxisType axis, int32_t direction, void *context) {
    log_func();
    if (axis > 0) {
        Data *data = layer_get_data(context);
        if (data->tap_timer == NULL) {
            sliding_text_layer_set_next_text(data->text_layer, data->buf_distance);
            sliding_text_layer_animate_down(data->text_layer);
            data->tap_timer = app_timer_register(4000, app_timer_callback, context);
        }
    }
}

static void settings_handler(void *context) {
    log_func();
    Data *data = layer_get_data(context);
    sliding_text_layer_set_text_color(data->text_layer, colors_get_foreground_color());
    pdc_transform_recolor_image(data->steps_pdc, colors_get_foreground_color(), colors_get_background_color());
}

StepLayer *step_layer_create(GRect frame) {
    log_func();
    StepLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);
    GRect bounds = layer_get_bounds(this);

    data->steps_pdc = gdraw_command_image_create_with_resource(RESOURCE_ID_HEALTH_STEPS);
#ifdef PBL_DISPLAY_LARGE
    pdc_transform_scale_image(data->steps_pdc, 13);
#endif

    GSize size = gdraw_command_image_get_bounds_size(data->steps_pdc);
    GRect rect = GRect(0, 3, bounds.size.w - size.w - 2, bounds.size.h - 3);
    data->text_layer = sliding_text_layer_create(rect);
    sliding_text_layer_set_font(data->text_layer, lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_26, RESOURCE_ID_GILROY_LIGHT_18)));
    sliding_text_layer_set_text_alignment(data->text_layer, GTextAlignmentRight);
    sliding_text_layer_set_text(data->text_layer, data->buf_steps);
    sliding_text_layer_set_duration(data->text_layer, 500);
    layer_add_child(this, data->text_layer);

    health_event_handler(HealthEventSignificantUpdate, this);
    data->health_event_handle = events_health_service_events_subscribe(health_event_handler, this);

    data->tap_event_handle = events_accel_tap_service_subscribe_context(tap_handler, this);

    settings_handler(this);
    data->settings_event_handle = enamel_settings_received_subscribe(settings_handler, this);

    return this;
}

void step_layer_destroy(StepLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    enamel_settings_received_unsubscribe(data->settings_event_handle);
    events_accel_tap_service_unsubscribe(data->tap_event_handle);
    events_health_service_events_unsubscribe(data->health_event_handle);
    sliding_text_layer_destroy(data->text_layer);
    gdraw_command_image_destroy(data->steps_pdc);
    layer_destroy(this);
}
#endif
