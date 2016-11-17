#pragma once
#include <pebble.h>

typedef Layer WeatherLayer;

WeatherLayer *weather_layer_create(GRect frame);
void weather_layer_destroy(WeatherLayer *this);
