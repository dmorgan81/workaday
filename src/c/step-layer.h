#pragma once
#include <pebble.h>

typedef Layer StepLayer;

StepLayer *step_layer_create(GRect frame);
void step_layer_destroy(StepLayer *this);
