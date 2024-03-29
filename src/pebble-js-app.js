var CONSOLE_LOG = false;
var _showConfiguration = false;

Pebble.addEventListener("ready",
  function(e) {
    consoleLog("Event listener - ready");
    _showConfiguration = false;
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    var message;
    
    consoleLog("Event listener - appmessage");

    if (typeof(e.payload.KEY_INSTALLED_VERSION) !== "undefined") {
      localStorage.setItem("installedVersion", parseInt(e.payload.KEY_INSTALLED_VERSION));  
      message = "Installed version is " + e.payload.KEY_INSTALLED_VERSION;
      consoleLog(message);
    }

    if (typeof(e.payload.KEY_CLOCK_24_HOUR) !== "undefined") {
      localStorage.setItem("clock24Hour", parseInt(e.payload.KEY_CLOCK_24_HOUR));  
      message = "24-hour is " + ((e.payload.KEY_CLOCK_24_HOUR == 1) ? "on" : "off");
      consoleLog(message);
      
      if (_showConfiguration === true) {
        _showConfiguration = false;
        showSettings();
      }
    }
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    consoleLog("Event listener - showConfiguration");
    
    // Request setup info (clock format and installed version) from watchface.
    var dictionary = {
      "KEY_REQUEST_SETUP_INFO" : 0
    };

    _showConfiguration = true;  
    Pebble.sendAppMessage(dictionary,
                          function(e) {
                            consoleLog("Setup info request successfully sent to Pebble");
                          },
                          function(e) {
                            // Fetching setup info failed, so just show configuration page with default/saved values.
                            _showConfiguration = false;
                            consoleLog("Error sending setup info request to Pebble");
                            showSettings();
                          }
    );
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    consoleLog("Event listener - webviewclosed");
    
    if (typeof(e.response) === "undefined" || e.response == "CANCELLED") {
      consoleLog("Settings canceled by user");
      
    } else {
      var configuration = JSON.parse(decodeURIComponent(e.response));
      consoleLog("Configuration window returned: " + JSON.stringify(configuration));
      
      saveSettings(configuration);

      var dictionary = {
        "KEY_CURRENT_VERSION" : parseInt(configuration.currentVersion),
        "KEY_HOUR_VIBRATE" : parseInt(configuration.hourVibrate),
        "KEY_HOUR_VIBRATE_START" : parseInt(configuration.hourVibrateStart),
        "KEY_HOUR_VIBRATE_END" : parseInt(configuration.hourVibrateEnd),
        "KEY_BLUETOOTH_VIBRATE" : parseInt(configuration.bluetoothVibrate),
        "KEY_SCENE_OVERRIDE" : parseInt(configuration.sceneOverride),
        "KEY_SHARK_VIBRATE" : parseInt(configuration.sharkVibrate),
        "KEY_SHARK_VIBRATE_START" : parseInt(configuration.sharkVibrateStart),
        "KEY_SHARK_VIBRATE_END" : parseInt(configuration.sharkVibrateEnd)
      };
  
      Pebble.sendAppMessage(dictionary,
        function(e) {
          consoleLog("Settings successfully sent to Pebble");
        },
        function(e) {
          consoleLog("Error sending settings to Pebble");
        }
      );
    }
  }
);

function formatUrlVariables() {
  var installedVersion = getLocalInt("installedVersion", 0);
  var hourVibrate = getLocalInt("hourVibrate", 0);
  var hourVibrateStart = getLocalInt("hourVibrateStart", 9);
  var hourVibrateEnd = getLocalInt("hourVibrateEnd", 18);
  var bluetoothVibrate = getLocalInt("bluetoothVibrate", 1);
  var sceneOverride = getLocalInt("sceneOverride", 0);
  var clock24Hour = getLocalInt("clock24Hour", 0);
  var sharkVibrate = getLocalInt("sharkVibrate", 1);
  var sharkVibrateStart = getLocalInt("sharkVibrateStart", 9);
  var sharkVibrateEnd = getLocalInt("sharkVibrateEnd", 18);
	
  return ("installedVersion=" + installedVersion + "&hourVibrate=" + hourVibrate + 
          "&hourVibrateStart=" + hourVibrateStart + "&hourVibrateEnd=" + hourVibrateEnd + 
          "&bluetoothVibrate=" + bluetoothVibrate + "&sceneOverride=" + sceneOverride + 
          "&clock24Hour=" + clock24Hour + "&sharkVibrate=" + sharkVibrate + 
          "&sharkVibrateStart=" + sharkVibrateStart + "&sharkVibrateEnd=" + sharkVibrateEnd);
}

function saveSettings(settings) {
  localStorage.setItem("currentVersion", parseInt(settings.currentVersion));  
  localStorage.setItem("hourVibrate", parseInt(settings.hourVibrate));  
  localStorage.setItem("hourVibrateStart", parseInt(settings.hourVibrateStart));  
  localStorage.setItem("hourVibrateEnd", parseInt(settings.hourVibrateEnd));  
  localStorage.setItem("bluetoothVibrate", parseInt(settings.bluetoothVibrate)); 
  localStorage.setItem("sceneOverride", parseInt(settings.sceneOverride)); 
  localStorage.setItem("sharkVibrate", parseInt(settings.sharkVibrate));  
  localStorage.setItem("sharkVibrateStart", parseInt(settings.sharkVibrateStart));  
  localStorage.setItem("sharkVibrateEnd", parseInt(settings.sharkVibrateEnd));  
}

function showSettings() {
  Pebble.openURL("http://www.sherbeck.com/pebble/floatyduck17.html?" + formatUrlVariables());
}

function getLocalInt(name, defaultValue) {
  var localValue = parseInt(localStorage.getItem(name));
	if (isNaN(localValue)) {
		localValue = defaultValue;
	}
  
  return localValue;
}

function consoleLog(message) {
  if (CONSOLE_LOG) {
    console.log(message);
  }
}