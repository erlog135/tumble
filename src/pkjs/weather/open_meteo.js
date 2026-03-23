var solar    = require('../solar');
var BASE_URL = 'https://api.open-meteo.com/v1/forecast';
var CACHE_KEY = 'weather-cache';

/** Converts Celsius to Fahrenheit, rounded to the nearest integer. */
function celsiusToFahrenheit(c) {
  return Math.round(c * 9 / 5 + 32);
}

/**
 * Compact condition codes sent to the watch (must match WeatherCondition enum in weather_provider.c).
 *   0  UNKNOWN
 *   1  CLEAR
 *   2  PARTLY_CLOUDY        (WMO 1–2)
 *   3  CLOUDY               (WMO 3 overcast)
 *   4  FOGGY
 *   5  RAINY                (drizzle, rain, rain showers)
 *   6  SNOWY_RAINY          (freezing drizzle / freezing rain)
 *   7  SNOWY                (snow fall, snow grains, snow showers)
 *   8  STORMY               (thunderstorm ± hail)
 *   9  CLEAR_NIGHT
 *  10  PARTLY_CLOUDY_NIGHT  (WMO 1–2 at night)
 */
var WEATHER_COND_UNKNOWN             = 0;
var WEATHER_COND_CLEAR               = 1;
var WEATHER_COND_PARTLY_CLOUDY       = 2;
var WEATHER_COND_CLOUDY              = 3;
var WEATHER_COND_FOGGY               = 4;
var WEATHER_COND_RAINY               = 5;
var WEATHER_COND_SNOWY_RAINY         = 6;
var WEATHER_COND_SNOWY               = 7;
var WEATHER_COND_STORMY              = 8;
var WEATHER_COND_CLEAR_NIGHT         = 9;
var WEATHER_COND_PARTLY_CLOUDY_NIGHT = 10;

/** Maps an Open-Meteo WMO weather_code + current day/night to a watch condition enum value. */
function wmoToCondition(code, isDay) {
  if (code === 0)                                return isDay ? WEATHER_COND_CLEAR               : WEATHER_COND_CLEAR_NIGHT;
  if (code === 1 || code === 2)                  return isDay ? WEATHER_COND_PARTLY_CLOUDY       : WEATHER_COND_PARTLY_CLOUDY_NIGHT;
  if (code === 3)                                return WEATHER_COND_CLOUDY;
  if (code === 45 || code === 48)                return WEATHER_COND_FOGGY;
  if (code === 51 || code === 53 || code === 55) return WEATHER_COND_RAINY;
  if (code === 56 || code === 57)                return WEATHER_COND_SNOWY_RAINY;
  if (code === 61 || code === 63 || code === 65) return WEATHER_COND_RAINY;
  if (code === 66 || code === 67)                return WEATHER_COND_SNOWY_RAINY;
  if (code >= 71  && code <= 77)                 return WEATHER_COND_SNOWY;
  if (code === 80 || code === 81 || code === 82) return WEATHER_COND_RAINY;
  if (code === 85 || code === 86)                return WEATHER_COND_SNOWY;
  if (code === 95 || code === 96 || code === 99) return WEATHER_COND_STORMY;
  return WEATHER_COND_UNKNOWN;
}

function sendWeather(tempF, pressure, condition) {
  Pebble.sendAppMessage(
    {
      'WEATHER_TEMPERATURE': tempF,
      'WEATHER_PRESSURE':    pressure,
      'WEATHER_CONDITION':   condition
    },
    function() {
      console.log('Weather sent: temp=' + tempF + '°F' +
        ' pressure=' + pressure + 'hPa' +
        ' condition=' + condition);
    },
    function(e) {
      console.log('Weather send failed: ' + JSON.stringify(e));
    }
  );
}

function fetch(lat, lon) {
  var settings       = JSON.parse(localStorage.getItem('clay-settings')) || {};
  var intervalMs     = (parseInt(settings['CFG_WEATHER_REFRESH_INTERVAL'], 10) || 30) * 60 * 1000;

  var cached = JSON.parse(localStorage.getItem(CACHE_KEY) || 'null');
  if (cached && (Date.now() - cached.timestamp) < intervalMs) {
    console.log('Weather: sending cached data (age=' +
      Math.round((Date.now() - cached.timestamp) / 1000) + 's)');
    // Re-encode with current day/night so the icon stays accurate across
    // a sunrise or sunset during a long cache interval.
    sendWeather(cached.tempF, cached.pressure,
      wmoToCondition(cached.weatherCode, solar.isDaytime(lat, lon)));
    return;
  }

  var url = BASE_URL +
    '?latitude='  + lat +
    '&longitude=' + lon +
    '&current=temperature_2m,surface_pressure,weather_code';

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.onload = function() {
    if (xhr.status !== 200) {
      console.log('Weather: HTTP error ' + xhr.status);
      return;
    }
    var data;
    try {
      data = JSON.parse(xhr.responseText);
    } catch (e) {
      console.log('Weather: failed to parse response: ' + e);
      return;
    }

    var current     = data.current;
    var tempF       = celsiusToFahrenheit(current.temperature_2m);
    var pressure    = Math.round(current.surface_pressure);
    var weatherCode = current.weather_code;

    localStorage.setItem(CACHE_KEY, JSON.stringify({
      tempF:       tempF,
      pressure:    pressure,
      weatherCode: weatherCode,
      timestamp:   Date.now()
    }));

    sendWeather(tempF, pressure, wmoToCondition(weatherCode, solar.isDaytime(lat, lon)));
  };
  xhr.onerror = function() {
    console.log('Weather: network error fetching ' + url);
  };
  xhr.send();
}

module.exports = { fetch: fetch };
