#pragma once
#include <pebble.h>

typedef Layer SleepLayer;

SleepLayer *sleep_layer_create(GRect frame);
void sleep_layer_destroy(SleepLayer *this);
