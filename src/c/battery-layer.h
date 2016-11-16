#pragma once
#include <pebble.h>

typedef Layer BatteryLayer;

BatteryLayer *battery_layer_create(GRect frame);
void battery_layer_destroy(BatteryLayer *this);
