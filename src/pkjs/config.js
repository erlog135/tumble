var TZ_OPTIONS = [
  { "label": "UTC-12 (BIT)", "value": "-720" },
  { "label": "UTC-11 (SST)", "value": "-660" },
  { "label": "UTC-10 (HST)", "value": "-600" },
  { "label": "UTC-9 (AKST)", "value": "-540" },
  { "label": "UTC-8 (PST)", "value": "-480" },
  { "label": "UTC-7 (MST/PDT)", "value": "-420" },
  { "label": "UTC-6 (CST/MDT)", "value": "-360" },
  { "label": "UTC-5 (EST/CDT)", "value": "-300" },
  { "label": "UTC-4 (AST/EDT)", "value": "-240" },
  { "label": "UTC-3 (BRT/ART)", "value": "-180" },
  { "label": "UTC-2 (GST)", "value": "-120" },
  { "label": "UTC-1 (CVT/AZOT)", "value": "-60" },
  { "label": "UTC+0 (UTC/GMT/WET)", "value": "0" },
  { "label": "UTC+1 (CET/WAT)", "value": "60" },
  { "label": "UTC+2 (EET/CAT/IST)", "value": "120" },
  { "label": "UTC+3 (MSK/EAT/AST)", "value": "180" },
  { "label": "UTC+3:30 (IRST)", "value": "210" },
  { "label": "UTC+4 (GST/GET)", "value": "240" },
  { "label": "UTC+4:30 (AFT)", "value": "270" },
  { "label": "UTC+5 (PKT/UZT)", "value": "300" },
  { "label": "UTC+5:30 (IST/SLT)", "value": "330" },
  { "label": "UTC+5:45 (NPT)", "value": "345" },
  { "label": "UTC+6 (BST/BTT)", "value": "360" },
  { "label": "UTC+6:30 (MMT/CCT)", "value": "390" },
  { "label": "UTC+7 (ICT/WIB)", "value": "420" },
  { "label": "UTC+8 (CST/SGT/AWST)", "value": "480" },
  { "label": "UTC+8:45 (ACWST)", "value": "525" },
  { "label": "UTC+9 (JST/KST)", "value": "540" },
  { "label": "UTC+9:30 (ACST)", "value": "570" },
  { "label": "UTC+10 (AEST/ChST)", "value": "600" },
  { "label": "UTC+10:30 (LHST)", "value": "630" },
  { "label": "UTC+11 (SBT/AEDT)", "value": "660" },
  { "label": "UTC+12 (NZST/FJST)", "value": "720" },
  { "label": "UTC+12:45 (CHAST)", "value": "765" },
  { "label": "UTC+13 (TOT/NZDT)", "value": "780" },
  { "label": "UTC+14 (LINT)", "value": "840" }
];

module.exports = [
  {
    "type": "heading",
    "defaultValue": "Tumble",
    "size": 1
  },
  {
    "type": "heading",
    "defaultValue": "Preferences",
    "size": 3
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance"
      },
      {
        "type": "toggle",
        "messageKey": "CFG_BLACK_BG",
        "label": "Black background",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "CFG_BLACK_MINIVIEW_BG",
        "label": "Black mini-view background",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Units"
      },
      {
        "type": "select",
        "messageKey": "CFG_UNIT_TEMP",
        "label": "Temperature",
        "defaultValue": "f",
        "options": [
          { "label": "°F", "value": "f" },
          { "label": "°C", "value": "c" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_UNIT_ALTITUDE",
        "label": "Distance",
        "defaultValue": "ft",
        "options": [
          { "label": "ft", "value": "ft" },
          { "label": "m", "value": "m" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_UNIT_PRESSURE",
        "label": "Air pressure",
        "defaultValue": "mb",
        "options": [
          { "label": "mb", "value": "mb" },
          { "label": "inHg", "value": "inhg" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "select",
        "messageKey": "CFG_WEATHER_REFRESH_INTERVAL",
        "label": "Weather refresh interval",
        "defaultValue": "30",
        "options": [
          { "label": "30 minutes", "value": "30" },
          { "label": "1 hour", "value": "60" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Time"
      },
      {
        "type": "select",
        "messageKey": "CFG_TZ_OFFSET",
        "label": "Custom time zone",
        "defaultValue": "0",
        "options": TZ_OPTIONS
      },
      {
        "type": "select",
        "messageKey": "CFG_SECONDS_OPTION",
        "label": "Seconds visibility",
        "defaultValue": "3",
        "options": [
          { "label": "Always on", "value": "0" },
          { "label": "On shake (30s)", "value": "1" },
          { "label": "On shake (15s)", "value": "2" },
          { "label": "Always off", "value": "3" }
        ]
      }
    ]
  },
  {
    "type": "heading",
    "defaultValue": "Complications",
    "size": 3
  },
  {
    "type": "section",
    "items": [
      {
        "type": "select",
        "messageKey": "CFG_GRAPH_OPTION",
        "label": "Graph",
        "defaultValue": "5",
        "options": [
          { "label": "Steps (24h)", "value": "0" },
          { "label": "Heart rate (4h)", "value": "1" },
          { "label": "Air pressure (24h)", "value": "2" },
          { "label": "Elevation (24h)", "value": "3" },
          { "label": "Battery (3d)", "value": "4" },
          { "label": "Temperature (24h)", "value": "5" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_MINIVIEW_OPTION",
        "label": "Mini-view",
        "defaultValue": "1",
        "options": [
          { "label": "Month & date", "value": "0" },
          { "label": "Day of week & date", "value": "1" },
          { "label": "Heart rate", "value": "2" },
          { "label": "Steps", "value": "3" },
          { "label": "Calories", "value": "4" },
          { "label": "Elevation", "value": "5" },
          { "label": "Air pressure", "value": "6" },
          { "label": "Sunrise / Sunset time", "value": "7" },
          { "label": "Current weather", "value": "8" },
          { "label": "Custom time zone", "value": "9" },
          { "label": "Current sun position", "value": "10" },
          { "label": "Current moon phase", "value": "11" },
          { "label": "Battery", "value": "12" },
          { "label": "Battery / Quiet Time / BT status", "value": "13" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_BOTTOM_LEFT_OPTION",
        "label": "Bottom left",
        "defaultValue": "8",
        "options": [
          { "label": "Month & date", "value": "0" },
          { "label": "Day of week & date", "value": "1" },
          { "label": "Steps", "value": "2" },
          { "label": "Heart rate", "value": "3" },
          { "label": "Calories", "value": "4" },
          { "label": "Elevation", "value": "5" },
          { "label": "Air pressure", "value": "6" },
          { "label": "Pressure trend", "value": "7" },
          { "label": "Sunrise / Sunset time", "value": "8" },
          { "label": "Temperature", "value": "9" },
          { "label": "Battery", "value": "10" },
          { "label": "Custom time zone", "value": "11" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_BOTTOM_RIGHT_OPTION",
        "label": "Bottom right",
        "defaultValue": "10",
        "options": [
          { "label": "Month & date", "value": "0" },
          { "label": "Day of week & date", "value": "1" },
          { "label": "Steps", "value": "2" },
          { "label": "Heart rate", "value": "3" },
          { "label": "Calories", "value": "4" },
          { "label": "Elevation", "value": "5" },
          { "label": "Air pressure", "value": "6" },
          { "label": "Pressure trend", "value": "7" },
          { "label": "Sunrise / Sunset time", "value": "8" },
          { "label": "Temperature", "value": "9" },
          { "label": "Battery", "value": "10" },
          { "label": "Custom time zone", "value": "11" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
