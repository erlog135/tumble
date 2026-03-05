module.exports = [
  {
    "type": "heading",
    "defaultValue": "Tumble Settings",
    "size": 1
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "General"
      },
      {
        "type": "toggle",
        "messageKey": "CFG_BLACK_BG",
        "label": "Black Background",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "CFG_INVERT_MINIVIEW",
        "label": "Invert Miniview",
        "defaultValue": true
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
        "messageKey": "CFG_WEATHER_OPTION",
        "label": "Weather data provider",
        "defaultValue": "0",
        "options": [
          { "label": "Apple WeatherKit",  "value": "0" },
          { "label": "Open-Meteo", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "CFG_WEATHER_REFRESH_INTERVAL",
        "label": "Weather refresh interval",
        "defaultValue": "30",
        "options": [
          { "label": "30 minutes", "value": "30" },
          { "label": "1 hour",     "value": "60" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Miniview"
      },
      {
        "type": "select",
        "messageKey": "CFG_MINIVIEW_OPTION",
        "label": "Display",
        "defaultValue": "1",
        "options": [
          { "label": "Date (Month & Date)",          "value": "0"  },
          { "label": "Date (Day of Week & Date)",    "value": "1"  },
          { "label": "Heart rate",                   "value": "2"  },
          { "label": "Steps",                        "value": "3"  },
          { "label": "Calories",                     "value": "4"  },
          { "label": "Altitude",                     "value": "5"  },
          { "label": "Air pressure",                 "value": "6"  },
          { "label": "Sunrise / Sunset time",        "value": "7"  },
          { "label": "Current weather",              "value": "8"  },
          { "label": "Custom time zone",             "value": "9"  },
          { "label": "Current sun position",         "value": "10" },
          { "label": "Current moon phase",           "value": "11" },
          { "label": "Battery",                      "value": "12" },
          { "label": "Battery / Quiet Time / Phone status", "value": "13" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Seconds"
      },
      {
        "type": "select",
        "messageKey": "CFG_SECONDS_OPTION",
        "label": "Display",
        "defaultValue": "0",
        "options": [
          { "label": "Always on",          "value": "0" },
          { "label": "On shake (30s)",     "value": "1" },
          { "label": "On shake (15s)",     "value": "2" },
          { "label": "Always off",         "value": "3" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Graph"
      },
      {
        "type": "select",
        "messageKey": "CFG_GRAPH_OPTION",
        "label": "Display",
        "defaultValue": "0",
        "options": [
          { "label": "Steps",        "value": "0" },
          { "label": "Heart rate",   "value": "1" },
          { "label": "Air pressure", "value": "2" },
          { "label": "Altitude",     "value": "3" },
          { "label": "Battery",      "value": "4" },
          { "label": "Temperature",  "value": "5" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Bottom Left"
      },
      {
        "type": "select",
        "messageKey": "CFG_BOTTOM_LEFT_OPTION",
        "label": "Display",
        "defaultValue": "8",
        "options": [
          { "label": "Date (month & date)",       "value": "0" },
          { "label": "Date (day of week & date)", "value": "1" },
          { "label": "Steps",                     "value": "2" },
          { "label": "Heart rate",                "value": "3" },
          { "label": "Calories",                  "value": "4" },
          { "label": "Altitude",                  "value": "5" },
          { "label": "Air pressure",              "value": "6" },
          { "label": "Pressure trend",            "value": "7" },
          { "label": "Sunrise / Sunset time",     "value": "8" },
          { "label": "Temperature",               "value": "9" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Bottom Right"
      },
      {
        "type": "select",
        "messageKey": "CFG_BOTTOM_RIGHT_OPTION",
        "label": "Display",
        "defaultValue": "0",
        "options": [
          { "label": "Date (month & date)",       "value": "0" },
          { "label": "Date (day of week & date)", "value": "1" },
          { "label": "Steps",                     "value": "2" },
          { "label": "Heart rate",                "value": "3" },
          { "label": "Calories",                  "value": "4" },
          { "label": "Altitude",                  "value": "5" },
          { "label": "Air pressure",              "value": "6" },
          { "label": "Pressure trend",            "value": "7" },
          { "label": "Sunrise / Sunset time",     "value": "8" },
          { "label": "Temperature",               "value": "9" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
