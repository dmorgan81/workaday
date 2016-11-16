#pragma once
#include <pebble.h>

typedef Layer TopLayer;

TopLayer *top_layer_create(GRect frame);
void top_layer_destroy(TopLayer *this);
