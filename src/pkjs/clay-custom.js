/**
 * Clay custom function (runs in config WebView). Strips health / heart-rate
 * complication options when the connected watch does not support them.
 * Mirrors @rebble/clay capabilityMap HEALTH + HR-capable platforms (no HR on basalt/chalk).
 */
module.exports = function () {
  var clayConfig = this;

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
    var wi = clayConfig.meta && clayConfig.meta.activeWatchInfo;
    if (!wi) return;

    function clayHealthMatches() {
      var healthPlatforms = { basalt: 1, chalk: 1, diorite: 1, emery: 1, flint: 1, gabbro: 1 };
      if (wi.platform === 'aplite') return false;
      if (!healthPlatforms[wi.platform]) return false;
      var fw = wi.firmware;
      if (!fw) return true;
      return fw.major > 3 || (fw.major === 3 && fw.minor >= 10);
    }

    function hasHeartRateSensor() {
      //TODO: what about when a diorite doesn't have a heart rate sensor?
      var hrPlatforms = { diorite: 1, emery: 1 };
      return !!hrPlatforms[wi.platform];
    }

    var hasHealth = clayHealthMatches();
    var hasHR = hasHeartRateSensor();

    function removeOptions(selectEl, removeSet) {
      if (!selectEl || !selectEl.options) return;
      var i = 0;
      while (i < selectEl.options.length) {
        var v = selectEl.options[i].value;
        if (removeSet[v]) {
          selectEl.remove(i);
        } else {
          i++;
        }
      }
    }

    function fireChange(selectEl) {
      if (!selectEl || !selectEl.dispatchEvent) return;
      try {
        selectEl.dispatchEvent(new Event('change', { bubbles: true }));
      } catch (e) {
        var ev = document.createEvent('HTMLEvents');
        ev.initEvent('change', true, false);
        selectEl.dispatchEvent(ev);
      }
    }

    function ensureValid(item, fallback) {
      var sel = item.$manipulatorTarget[0];
      if (!sel || !sel.options.length) return;
      var allowed = {};
      for (var j = 0; j < sel.options.length; j++) {
        allowed[sel.options[j].value] = true;
      }
      var cur = item.get();
      if (allowed[cur]) return;
      if (allowed[fallback]) {
        item.set(fallback);
      } else {
        item.set(sel.options[0].value);
      }
      fireChange(sel);
    }

    var removeGraph = {};
    var removeMiniview = {};
    var removeBottom = {};

    if (!hasHealth) {
      removeGraph = { '0': 1, '1': 1 };
      removeMiniview = { '2': 1, '3': 1, '4': 1, '13': 1 };
      removeBottom = { '2': 1, '3': 1, '4': 1 };
    } else if (!hasHR) {
      removeGraph = { '1': 1 };
      removeMiniview = { '2': 1 };
      removeBottom = { '3': 1 };
    }

    var tasks = [
      ['CFG_GRAPH_OPTION', removeGraph, '5'],
      ['CFG_MINIVIEW_OPTION', removeMiniview, '1'],
      ['CFG_BOTTOM_LEFT_OPTION', removeBottom, '8'],
      ['CFG_BOTTOM_RIGHT_OPTION', removeBottom, '10']
    ];

    for (var k = 0; k < tasks.length; k++) {
      var mk = tasks[k][0];
      var rs = tasks[k][1];
      var fb = tasks[k][2];
      if (!rs || !Object.keys(rs).length) continue;

      var item = clayConfig.getItemByMessageKey(mk);
      if (!item) continue;

      var sel = item.$manipulatorTarget[0];
      removeOptions(sel, rs);
      ensureValid(item, fb);
    }
  });
};
