var SunCalc = require('suncalc');

/**
 * Maps SunCalc moon phase (0.0-1.0) to an integer 0-7:
 *   0 = New Moon
 *   1 = Waxing Crescent
 *   2 = First Quarter
 *   3 = Waxing Gibbous
 *   4 = Full Moon
 *   5 = Waning Gibbous
 *   6 = Last Quarter
 *   7 = Waning Crescent
 */
function moonPhaseIndex(phase) {
  return Math.round(phase * 8) % 8;
}

// lat/lon accepted for future use (e.g. getMoonPosition, moon rise/set times)
function fetch(lat, lon) {
  var illum = SunCalc.getMoonIllumination(new Date());
  var phaseIndex = moonPhaseIndex(illum.phase);

  Pebble.sendAppMessage(
    { 'WEATHER_MOON_PHASE': phaseIndex },
    function() {
      console.log('Lunar data sent: phase=' + phaseIndex);
    },
    function(e) {
      console.log('Lunar data send failed: ' + JSON.stringify(e));
    }
  );
}

module.exports = { fetch: fetch };
