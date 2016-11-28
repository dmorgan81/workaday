#pragma once
#include <pebble.h>

typedef Layer QuietTimeLayer;

QuietTimeLayer *quiet_time_layer_create(GRect frame);
void quiet_time_layer_destroy(QuietTimeLayer *this);
