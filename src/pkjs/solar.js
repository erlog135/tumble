var SunCalc = require('suncalc');

/**
 * Packs sunrise and sunset into a single int32:
 *   upper 16 bits: sunrise minutes since midnight (0-1440)
 *   lower 16 bits: sunset  minutes since midnight (0-1440)
 */
function packSunTimes(sunriseDate, sunsetDate) {
  var sunriseMinutes = sunriseDate.getHours() * 60 + sunriseDate.getMinutes();
  var sunsetMinutes  = sunsetDate.getHours()  * 60 + sunsetDate.getMinutes();
  return ((sunriseMinutes & 0xFFFF) << 16) | (sunsetMinutes & 0xFFFF);
}

function fetch(lat, lon) {
  var now = new Date();
  var times = SunCalc.getTimes(now, lat, lon);
  var packed = packSunTimes(times.sunrise, times.sunset);

  Pebble.sendAppMessage(
    { 'WEATHER_SUN_RISE_SET': packed },
    function() {
      console.log('Solar data sent: sunrise=' +
        times.sunrise.getHours() + ':' + times.sunrise.getMinutes() +
        ' sunset=' +
        times.sunset.getHours()  + ':' + times.sunset.getMinutes());
    },
    function(e) {
      console.log('Solar data send failed: ' + JSON.stringify(e));
    }
  );
}

module.exports = { fetch: fetch };
