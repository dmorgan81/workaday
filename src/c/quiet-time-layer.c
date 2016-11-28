#ifndef PBL_PLATFORM_APLITE
#include "common.h"
#include <lazy-fonts/lazy-fonts.h>
#include "colors.h"
#include "quiet-time-layer.h"

typedef struct {
    GFont font;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    if (!quiet_time_is_active()) return;

    Data *data = layer_get_data(this);
    graphics_context_set_text_color(ctx, colors_get_foreground_color());
    graphics_draw_text(ctx, "Q T", data->font, layer_get_bounds(this), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

QuietTimeLayer *quiet_time_layer_create(GRect frame) {
    log_func();
    QuietTimeLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);
    data->font = lazy_fonts_get(PBL_IF_DISPLAY_LARGE_ELSE(RESOURCE_ID_GILROY_LIGHT_14, RESOURCE_ID_GILROY_LIGHT_10));
    return this;
}

void quiet_time_layer_destroy(QuietTimeLayer *this) {
    log_func();
    layer_destroy(this);
}


#endif