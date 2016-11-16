#pragma once
#include <pebble.h>

typedef Layer TimeLayer;

TimeLayer *time_layer_create(GRect frame);
void time_layer_destroy(TimeLayer *this);
