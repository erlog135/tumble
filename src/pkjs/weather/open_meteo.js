var BASE_URL    = 'https://api.open-meteo.com/v1/forecast';
var CACHE_KEY   = 'weather-cache';

/** Converts Celsius to Fahrenheit, rounded to the nearest integer. */
function celsiusToFahrenheit(c) {
  return Math.round(c * 9 / 5 + 32);
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
    sendWeather(cached.tempF, cached.pressure, cached.condition);
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

    var current   = data.current;
    var tempF     = celsiusToFahrenheit(current.temperature_2m);
    var pressure  = Math.round(current.surface_pressure);
    var condition = current.weather_code;

    localStorage.setItem(CACHE_KEY, JSON.stringify({
      tempF:     tempF,
      pressure:  pressure,
      condition: condition,
      timestamp: Date.now()
    }));

    sendWeather(tempF, pressure, condition);
  };
  xhr.onerror = function() {
    console.log('Weather: network error fetching ' + url);
  };
  xhr.send();
}

module.exports = { fetch: fetch };
