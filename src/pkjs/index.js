var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var geo = require('./geo');
var solar = require('./solar');
var lunar = require('./lunar');

new Clay(clayConfig);

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready');
  geo.getPosition(function(err, pos) {
    if (err) {
      console.log('Location unavailable, skipping solar and lunar updates.');
      return;
    }
    solar.fetch(pos.lat, pos.lon);
    lunar.fetch(pos.lat, pos.lon);
  });
});
