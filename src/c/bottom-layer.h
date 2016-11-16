#pragma once
#include <pebble.h>

#define BOTTOM_LAYER_HEIGHT PBL_IF_DISPLAY_LARGE_ELSE(46, 34)

typedef Layer BottomLayer;

BottomLayer *bottom_layer_create(GRect frame);
void bottom_layer_destroy(BottomLayer *this);
