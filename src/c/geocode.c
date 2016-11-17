#include <pebble-events/pebble-events.h>
#include <pebble-geocode-mapquest/pebble-geocode-mapquest.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "logging.h"
#include "geocode.h"

static const uint32_t PERSIST_KEY_GEOCODE_COORDINATES = 3;

#ifndef GEOCODE_API_KEY
#pragma message("GEOCODE_API_KEY not defined")
#define GEOCODE_API_KEY ""
#endif

typedef struct {
    EventGeocodeHandler handler;
    void *context;
} GeocodeHandlerState;

typedef struct {
    GeocodeMapquestCoordinates *coordinates;
    GeocodeMapquestStatus status;
} GeocodeBundle;

static LinkedRoot *s_handler_list;

static bool each_geocode_fetched(void *this, void *context) {
    log_func();
    GeocodeHandlerState *state = (GeocodeHandlerState *) this;
    GeocodeBundle *bundle = (GeocodeBundle *) context;
    state->handler(bundle->coordinates, bundle->status, state->context);
    return true;
}

static void geocode_fetch_callback(GeocodeMapquestCoordinates *coordinates, GeocodeMapquestStatus status) {
    log_func();
    GeocodeBundle bundle = {
        .coordinates = coordinates,
        .status = status
    };
    linked_list_foreach(s_handler_list, each_geocode_fetched, &bundle);
}

void geocode_init(void) {
    log_func();
    s_handler_list = linked_list_create_root();
    geocode_mapquest_init();
    geocode_mapquest_set_api_key(GEOCODE_API_KEY);

    geocode_mapquest_load(PERSIST_KEY_GEOCODE_COORDINATES);
}

void geocode_fetch(const char *location) {
    log_func();
    geocode_mapquest_fetch(location, geocode_fetch_callback);
}

void geocode_deinit(void) {
    log_func();
    geocode_mapquest_save(PERSIST_KEY_GEOCODE_COORDINATES);
    geocode_mapquest_deinit();

    free(s_handler_list);
}

GeocodeMapquestCoordinates *geocode_peek(void) {
    log_func();
    return geocode_mapquest_peek();
}

EventHandle events_geocode_subscribe(EventGeocodeHandler handler, void *context) {
    log_func();
    GeocodeHandlerState *this = malloc(sizeof(GeocodeHandlerState));
    this->handler = handler;
    this->context = context;
    linked_list_append(s_handler_list, this);

    return this;
}

void events_geocode_unsubscribe(EventHandle handle) {
    log_func();

    int16_t index = linked_list_find(s_handler_list, handle);
    if (index == -1) {
        return;
    }

    free(linked_list_get(s_handler_list, index));
    linked_list_remove(s_handler_list, index);
}
