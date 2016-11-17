module.exports = function(minified) {
    var clayConfig = this;
    var _ = minified._;
    var $ = minified.$;
    var HTML = minified.HTML;

    var providers = {
        "0": "owm",
        "1": "wu",
        "2": "forecast"
    };

    function configureWeather() {
        var weatherProvider = clayConfig.getItemByMessageKey('WEATHER_PROVIDER');
        var weatherKey = clayConfig.getItemByMessageKey('WEATHER_KEY');
        var masterKeyEmail = clayConfig.getItemById('masterKeyEmail');
        var masterKeyPin = clayConfig.getItemById('masterKeyPin');
        var masterKeyButton = clayConfig.getItemById('masterKeyButton');
        var masterKeyText = clayConfig.getItemById('masterKeyText');

        masterKeyText.hide();

        var masterKey;

        masterKeyButton.on('click', function() {
            var email = masterKeyEmail.get();
            var pin = masterKeyPin.get();
            if ((!masterKey || !masterKey.success) && email && pin) {
                var url = _.format('https://pmkey.xyz/search/?email={{email}}&pin={{pin}}', { email : email, pin : pin });
                $.request('get', url).then(function(txt, xhr) {
                    masterKey = JSON.parse(txt);
                    if (masterKey.success && masterKey.keys.weather) {
                        var weather = masterKey.keys.weather;
                        var provider = providers[weatherProvider.get()];
                        weatherKey.set(weather[provider]);
                        masterKeyText.set('Success');
                        masterKeyText.show();
                    } else {
                        masterKeyEmail.disable();
                        masterKeyPin.disable();
                        masterKeyButton.disable();
                        masterKeyText.set(masterKey.error);
                        masterKeyText.show();
                    }
                }).error(function(status, txt, xhr) {
                    masterKeyEmail.disable();
                    masterKeyPin.disable();
                    masterKeyButton.disable();
                    masterKeyText.set(status + ': ' + txt);
                    masterKeyText.show();
                });
            } else if (masterKey && masterKey.success && masterKey.keys.weather) {
                var weather = masterKey.keys.weather;
                var provider = providers[weatherProvider.get()];
                if (provider) weatherKey.set(weather[provider]);
            }
        });

        weatherProvider.on('change', function() {
            if (masterKey) {
                var weather = masterKey.keys.weather;
                var provider = providers[weatherProvider.get()];
                if (provider) weatherKey.set(weather[provider]);
            }
        });
    }

    function configureLocation() {
        var gpsToggle = clayConfig.getItemByMessageKey('USE_GPS');
        var locationInput = clayConfig.getItemByMessageKey('LOCATION_NAME');
        if (gpsToggle.get()) {
            locationInput.hide();
        }
        gpsToggle.on('change', function() {
            if (gpsToggle.get()) {
                locationInput.hide();
            } else {
                locationInput.show();
            }
        });
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        configureWeather();
        configureLocation();
    });
}