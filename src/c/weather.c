#include "common.h"
#include <pebble-events/pebble-events.h>
#include <pebble-generic-weather/pebble-generic-weather.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "geocode.h"
#include "weather.h"

static const uint32_t PERSIST_KEY_WEATHER_INFO = 2;
static const int32_t RESEND_TIMEOUT = SECONDS_PER_HOUR / 6; // 10 minutes

typedef struct {
    EventWeatherHandler handler;
    void *context;
} WeatherHandlerState;

typedef struct {
    GenericWeatherInfo *info;
    GenericWeatherStatus status;
} WeatherBundle;

static LinkedRoot *s_handler_list;
static EventHandle s_bluetooth_event_handle;
static AppTimer *s_timer;
static bool s_connected;
static EventHandle s_connected_event_handle;
static EventHandle s_health_event_handle;
static EventHandle s_settings_event_handle;
static bool s_sleeping;
static AppTimer *s_resend_timer;
static uint16_t s_interval;
static EventHandle s_geocode_event_handle;
static char s_location_name[32];

static bool each_weather_fetched(void *this, void *context) {
    log_func();
    WeatherHandlerState *state = (WeatherHandlerState *) this;
    WeatherBundle *bundle = (WeatherBundle *) context;
    state->handler(bundle->info, bundle->status, state->context);
    return true;
}

static void register_timer(uint32_t timeout);

static void generic_weather_fetch_callback(GenericWeatherInfo *info, GenericWeatherStatus status) {
    log_func();
    WeatherBundle bundle = {
        .info = info,
        .status = status
    };
    linked_list_foreach(s_handler_list, each_weather_fetched, &bundle);
    register_timer(s_interval);
    if (s_resend_timer) {
        app_timer_cancel(s_resend_timer);
        s_resend_timer = NULL;
    }
}

static void app_timer_callback(void *context) {
    log_func();
    s_timer = NULL;
    generic_weather_fetch(generic_weather_fetch_callback);
}

static void register_timer(uint32_t timeout) {
    log_func();
    if (s_timer) {
        app_timer_cancel(s_timer);
        s_timer = NULL;
    }
    if (!s_sleeping) s_timer = app_timer_register(timeout * 1000, app_timer_callback, NULL);
}

static void app_resend_timer_callback(void *context) {
    log_func();
    s_resend_timer = NULL;
    register_timer(1);
}

static void register_resend_timer(uint32_t timeout) {
    log_func();
    if (s_resend_timer) {
        app_timer_cancel(s_resend_timer);
        s_resend_timer = NULL;
    }
    s_resend_timer = app_timer_register(timeout * 1000, app_resend_timer_callback, NULL);
}

static void geocode_handler(GeocodeMapquestCoordinates *coordinates, GeocodeMapquestStatus status, void *context) {
    log_func();
    if (status == GeocodeMapquestStatusAvailable) {
        generic_weather_set_location((GenericWeatherCoordinates) {
            .latitude = coordinates->latitude,
            .longitude = coordinates->longitude
        });
    } else if (status != GeocodeMapquestStatusPending) {
        generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
    }
    if (status != GeocodeMapquestStatusPending) {
        register_timer(1);
    }
}

static void settings_handler(void *context) {
    log_func();
    generic_weather_set_api_key(enamel_get_WEATHER_KEY());
    generic_weather_set_provider(atoi(enamel_get_WEATHER_PROVIDER()));
    s_interval = atoi(enamel_get_WEATHER_INTERVAL()) * SECONDS_PER_MINUTE;

    if (enamel_get_USE_GPS()) {
        generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
        register_timer(1);
    } else if (strcmp(enamel_get_LOCATION_NAME(), s_location_name) != 0) {
        strncpy(s_location_name, enamel_get_LOCATION_NAME(), sizeof(s_location_name));
        geocode_fetch(s_location_name);
    }
}

static void health_event_handler(HealthEventType event, void *context) {
    log_func();
    if (event == HealthEventSignificantUpdate) {
        health_event_handler(HealthEventSleepUpdate, context);
    } else if (event == HealthEventSleepUpdate || (event == HealthEventMovementUpdate && s_sleeping)) {
        HealthActivityMask activities = health_service_peek_current_activities();
        s_sleeping = (activities & HealthActivitySleep) || (activities & HealthActivityRestfulSleep);
        if (!s_sleeping && s_connected) {
            GenericWeatherInfo *info = generic_weather_peek();
            time_t now = time(NULL);
            logd("%ld - %ld", now, info->timestamp);
            if (now - info->timestamp > s_interval) {
                register_timer(1);
            } else {
                logd("%ld", s_interval - (now - info->timestamp));
                register_timer(s_interval - (now - info->timestamp));
            }
        } else if (s_sleeping && s_timer) {
            app_timer_cancel(s_timer);
            s_timer = NULL;
        }
    }
}

static void outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    log_func();
    register_resend_timer(RESEND_TIMEOUT);
}

static void inbox_dropped(AppMessageResult reason, void *context) {
    log_func();
    register_resend_timer(RESEND_TIMEOUT);
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
    log_func();
    Tuple *tuple = dict_find(iterator, MESSAGE_KEY_APP_READY);
    if (tuple) {
        health_event_handler(HealthEventSleepUpdate, NULL);
    }
}

static void pebble_app_connection_handler(bool connected) {
    log_func();
    if (!connected && s_timer) {
        app_timer_cancel(s_timer);
        s_timer = NULL;
    } else if (s_connected != connected) {
        GenericWeatherInfo *info = generic_weather_peek();
        time_t now = time(NULL);
        if (now - info->timestamp > s_interval) {
            register_timer(1);
        } else {
            logd("%ld", s_interval - (now - info->timestamp));
            register_timer(s_interval - (now - info->timestamp));
        }
    }
    s_connected = connected;
}

void weather_init(void) {
    log_func();
    s_handler_list = linked_list_create_root();
    generic_weather_init();
    geocode_init();

    generic_weather_set_api_key(enamel_get_WEATHER_KEY());
    generic_weather_set_provider(atoi(enamel_get_WEATHER_PROVIDER()));
    generic_weather_set_feels_like(true);
    s_interval = atoi(enamel_get_WEATHER_INTERVAL()) * SECONDS_PER_MINUTE;

    strncpy(s_location_name, enamel_get_LOCATION_NAME(), sizeof(s_location_name));
    GeocodeMapquestCoordinates *coordinates = geocode_peek();
    if (enamel_get_USE_GPS() || coordinates == NULL || strlen(s_location_name) == 0) {
        generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
    } else {
        generic_weather_set_location((GenericWeatherCoordinates) {
            .latitude = coordinates->latitude,
            .longitude = coordinates->longitude
        });
    }
    s_geocode_event_handle = events_geocode_subscribe(geocode_handler, NULL);

    s_settings_event_handle = enamel_settings_received_subscribe(settings_handler, NULL);

    generic_weather_load(PERSIST_KEY_WEATHER_INFO);

    s_connected = connection_service_peek_pebble_app_connection();
    s_connected_event_handle = events_connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = pebble_app_connection_handler
    });

    s_health_event_handle = events_health_service_events_subscribe(health_event_handler, NULL);
    s_bluetooth_event_handle = events_app_message_subscribe_handlers((EventAppMessageHandlers) {
        .failed = outbox_failed,
        .received = inbox_received,
        .dropped = inbox_dropped
    }, NULL);
}

void weather_deinit(void) {
    log_func();
    events_app_message_unsubscribe(s_bluetooth_event_handle);
    events_health_service_events_unsubscribe(s_health_event_handle);
    events_connection_service_unsubscribe(s_connected_event_handle);
    enamel_settings_received_unsubscribe(s_settings_event_handle);
    events_geocode_unsubscribe(s_geocode_event_handle);

    generic_weather_save(PERSIST_KEY_WEATHER_INFO);
    generic_weather_deinit();

    geocode_deinit();
    free(s_handler_list);
}

GenericWeatherInfo *weather_peek(void) {
    log_func();
    return generic_weather_peek();
}

EventHandle events_weather_subscribe(EventWeatherHandler handler, void *context) {
    log_func();
    WeatherHandlerState *this = malloc(sizeof(WeatherHandlerState));
    this->handler = handler;
    this->context = context;
    linked_list_append(s_handler_list, this);

    return this;
}

void events_weather_unsubscribe(EventHandle handle) {
    log_func();

    int16_t index = linked_list_find(s_handler_list, handle);
    if (index == -1) {
        return;
    }

    free(linked_list_get(s_handler_list, index));
    linked_list_remove(s_handler_list, index);
}
