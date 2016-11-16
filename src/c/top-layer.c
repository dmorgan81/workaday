#include <pebble.h>
#include "logging.h"
#include "enamel.h"
#include "colors.h"
#include "top-layer.h"

typedef struct __attribute__((packed)) {
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    PreferredContentSize content_size = preferred_content_size();
    uint8_t height = content_size == PreferredContentSizeMedium ? 28 : 38;

    graphics_context_set_stroke_color(ctx, colors_get_foreground_color());
    graphics_draw_line(ctx, GPoint(0, height), GPoint(bounds.size.w, height));
    graphics_draw_line(ctx, GPoint(bounds.size.w / 2, 0), GPoint(bounds.size.w / 2, height));
}

TopLayer *top_layer_create(GRect frame) {
    log_func();
    TopLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    return this;
}

void top_layer_destroy(TopLayer *this) {
    log_func();
    layer_destroy(this);
}