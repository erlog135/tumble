var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clayCustom = require('./clay-custom');
var geo = require('./geo');
var solar = require('./solar');
var lunar = require('./lunar');
var weather = require('./weather/open_meteo');

var clay = new Clay(clayConfig, clayCustom);

function fetchSolarLunar() {
  geo.getPosition(function(err, pos) {
    if (err) {
      console.log('Solar/lunar: location unavailable, skipping update.');
      return;
    }
    solar.fetch(pos.lat, pos.lon);
    lunar.fetch(pos.lat, pos.lon);
  });
}

function fetchWeather() {
  geo.getPosition(function(err, pos) {
    if (err) {
      console.log('Weather: location unavailable, skipping refresh.');
      return;
    }
    weather.fetch(pos.lat, pos.lon);
  });
}

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.REQUEST_DATA !== undefined) {
    fetchWeather();
    fetchSolarLunar();
  }
});

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready');
  Pebble.sendAppMessage({ 'JS_READY': 1 });
});
