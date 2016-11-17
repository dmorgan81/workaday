#include "common.h"
#include "colors.h"

GColor colors_get_background_color(void) {
    log_func();
#ifdef PBL_COLOR
    return enamel_get_COLOR_BACKGROUND();
#else
    return enamel_get_COLOR_INVERT() ? GColorBlack : GColorWhite;
#endif
}

GColor colors_get_foreground_color(void) {
    log_func();
    return gcolor_legible_over(colors_get_background_color());
}
