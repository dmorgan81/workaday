#include <pebble.h>
#include "logging.h"
#include "enamel.h"
#include "colors.h"
#include "bottom-layer.h"

typedef struct __attribute__((packed)) {
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_unobstructed_bounds(this);
    PreferredContentSize content_size = preferred_content_size();
    uint8_t y = bounds.size.h - (content_size == PreferredContentSizeMedium ? 34 : 46);

    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    graphics_draw_line(ctx, GPoint(0, y), GPoint(bounds.size.w, y));
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, y), GPoint(bounds.size.w / 2, bounds.size.h));
}

BottomLayer *bottom_layer_create(GRect frame) {
    log_func();
    BottomLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    return this;
}

void bottom_layer_destroy(BottomLayer *this) {
    log_func();
    layer_destroy(this);
}
