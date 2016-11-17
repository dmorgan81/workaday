#pragma once
#include <pebble.h>

#define TOP_LAYER_HEIGHT PBL_IF_DISPLAY_LARGE_ELSE(38, 28)

typedef Layer TopLayer;

TopLayer *top_layer_create(GRect frame);
void top_layer_destroy(TopLayer *this);
