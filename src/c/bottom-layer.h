#pragma once
#include <pebble.h>

typedef Layer BottomLayer;

BottomLayer *bottom_layer_create(GRect frame);
void bottom_layer_destroy(BottomLayer *this);
