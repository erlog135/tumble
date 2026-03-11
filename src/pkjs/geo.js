/**
 * Fetches the device's current position once and passes it to callback.
 *
 * Callback signature: function(err, pos)
 *   pos: { lat, lon, altitude }  — altitude rounded to the nearest integer metre,
 *         or 0 when the platform does not provide it.
 */
function getPosition(callback) {
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      var alt = pos.coords.altitude;
      callback(null, {
        lat:      pos.coords.latitude,
        lon:      pos.coords.longitude,
        altitude: (alt !== null && alt !== undefined) ? Math.round(alt) : 0
      });
    },
    function(err) {
      console.log('Geolocation error (' + err.code + '): ' + err.message);
      callback(err, null);
    },
    { enableHighAccuracy: false, maximumAge: 300000, timeout: 15000 }
  );
}

module.exports = { getPosition: getPosition };
