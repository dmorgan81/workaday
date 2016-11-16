#pragma once
#include <pebble.h>

typedef Layer ConnectionLayer;

ConnectionLayer *connection_layer_create(GRect frame);
void connection_layer_destroy(ConnectionLayer *this);
