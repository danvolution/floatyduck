#include <pebble.h>
#include "common.h"
#include "marker_layer.h"
#include "hour_layer.h"
#include "water_layer.h"
#include "waves_layer.h"
#include "duck_layer.h"
#include "shark_layer.h"
#include "santa_layer.h"
#include "message_layer.h"
#include "status_layer.h"
  
#ifdef RUN_TEST
#include "test_unit.h"
#endif

#define KEY_CURRENT_VERSION 0
#define KEY_INSTALLED_VERSION 1
#define KEY_HOUR_VIBRATE 2
#define KEY_HOUR_VIBRATE_START 3
#define KEY_HOUR_VIBRATE_END 4
#define KEY_BLUETOOTH_VIBRATE 5
#define KEY_SCENE_OVERRIDE 6
#define KEY_CLOCK_24_HOUR 7
#define KEY_SHARK_VIBRATE 8
#define KEY_SHARK_VIBRATE_START 9
#define KEY_SHARK_VIBRATE_END 10
#define KEY_REQUEST_SETUP_INFO 11
  
#define MESSAGE_SETTINGS_DURATION 1500
#define MESSAGE_BLUETOOTH_DURATION 5000

#define VIBES_SHORT_IGNORE_TAPS_TIME 2000

typedef struct {
  int32_t currentVersion;
  int32_t hourVibrate;
  int32_t hourVibrateStart;
  int32_t hourVibrateEnd;
  int32_t bluetoothVibrate;
  int32_t sceneOverride;
  int32_t sharkVibrate;
  int32_t sharkVibrateStart;
  int32_t sharkVibrateEnd;
} Settings;

static Window* _mainWindow = NULL;
static MarkerLayerData* _markerData = NULL;
static HourLayerData* _hourData = NULL;
static WaterLayerData* _waterData = NULL;
static WavesLayerData* _wavesData = NULL;
static DuckLayerData* _duckData = NULL;
static SharkLayerData* _sharkData = NULL;
static SantaLayerData* _santaData = NULL;
static MessageLayerData *_messageData = NULL;
static StatusLayerData *_statusData = NULL;

#ifdef RUN_TEST
static TestUnitData* _testUnitData = NULL;
#endif

static SCENE _scene;
static Settings _settings;
static AppTimer *_messageTimer = NULL;
static AppTimer *_sharkWarnTimer = NULL;
static AppTimer *_ignoreTapTimer = NULL;

// Message window strings
static const char *_settingsReceivedMsg = "Settings received!";
static const char *_bluetoothDisconnectMsg = "Bluetooth connection lost!";

static void init();
static void deinit();
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void timer_handler(struct tm *tick_time, TimeUnits units_changed);
static void tap_handler(AccelAxisType axis, int32_t direction);
static void bluetooth_service_handler(bool connected);
static void battery_service_handler(BatteryChargeState charge_state);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *values, void *context);
static void outbox_failed_callback(DictionaryIterator *failed, AppMessageResult reason, void *context);
static void loadSettings(Settings *settings);
static void saveSettings(Settings *settings);
static int32_t readPersistentInt(const uint32_t key, int32_t defaultValue);
static bool isHourInRange(int16_t hour, int16_t start, int16_t end);
static void sendSetupInfo();
static void showMessage(const char *text, uint32_t duration);
static void messageTimerCallback(void *callback_data);
static void sharkWarnTimerCallback(void *callback_data);
static void ignoreTapTimerCallback(void *callback_data);
static void updateApp(struct tm *tick_time);
static void drawWatchFace(struct tm *tick_time);
static void drawScene(SCENE scene, uint16_t hour, uint16_t minute, uint16_t second);
static SCENE getScene(struct tm *tick_time);
static void switchScene(SCENE scene);
static struct tm* getTime(struct tm *real_time);
static void vibrate();

int main(void) {
  init();
  app_event_loop();
  deinit();
}

static void init() {
  _scene = UNDEFINED_SCENE;
  srand(time(NULL));
  loadSettings(&_settings);
  
#ifdef RUN_TEST
  _testUnitData = CreateTestUnit();
#endif
    
  // Create main Window element and assign to pointer
  _mainWindow = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(_mainWindow, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(_mainWindow, true);
  
#ifdef RUN_TEST
  tick_timer_service_subscribe(SECOND_UNIT, timer_handler);
#else
  tick_timer_service_subscribe(MINUTE_UNIT, timer_handler);
#endif

  // Register for accelerometer tap events
  accel_tap_service_subscribe(&tap_handler);
  
  // Register bluetooth service
  bluetooth_connection_service_subscribe(bluetooth_service_handler);
  
  // Register battery service
  battery_state_service_subscribe(battery_service_handler);
  
  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  vibes_cancel();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
  animation_unschedule_all();
  
  if (_sharkWarnTimer != NULL) {
    app_timer_cancel(_sharkWarnTimer);
    _sharkWarnTimer = NULL;
  }
  
  if (_messageTimer != NULL) {
    app_timer_cancel(_messageTimer);
    _messageTimer = NULL;
  }
  
  if (_ignoreTapTimer != NULL) {
    app_timer_cancel(_ignoreTapTimer);
    _ignoreTapTimer = NULL;
  }
  
#ifdef RUN_TEST
  if (_testUnitData != NULL) {
    DestroyTestUnit(_testUnitData);
    _testUnitData = NULL;
  }
#endif

  if (_mainWindow != NULL) {
    window_destroy(_mainWindow);
    _mainWindow = NULL;
  }
}

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorWhite);
  
  // Fixed layers
  _markerData = CreateMarkerLayer(window_get_root_layer(_mainWindow), CHILD);
  _statusData = CreateStatusLayer(window_get_root_layer(_mainWindow), CHILD);
  _hourData = CreateHourLayer(window_get_root_layer(_mainWindow), CHILD);
  _waterData = CreateWaterLayer(window_get_root_layer(_mainWindow), CHILD);
  _wavesData = CreateWavesLayer(window_get_root_layer(_mainWindow), CHILD);
  
  // Initialize Bluetooth status
  bool connected = bluetooth_connection_service_peek();
  ShowBluetoothStatus(_statusData, !connected);
  UpdateBluetoothStatus(_statusData, connected);
  
  // Initialize battery status
  BatteryChargeState batteryState = battery_state_service_peek();
  ShowBatteryStatus(_statusData, (batteryState.is_charging || batteryState.is_plugged));
  UpdateBatteryStatus(_statusData, batteryState);
  
  updateApp(getTime(NULL));
}

static void main_window_unload(Window *window) {
  if (_messageData != NULL) {
    DestroyMessageLayer(_messageData);
    _messageData = NULL;
  }
  
  if (_santaData != NULL) {
    DestroySantaLayer(_santaData);    
    _santaData = NULL;
  }

  if (_sharkData != NULL) {
    DestroySharkLayer(_sharkData);    
    _sharkData = NULL;
  }
  
  if (_duckData != NULL) {
    DestroyDuckLayer(_duckData);
    _duckData = NULL;
  }
  
  DestroyWavesLayer(_wavesData);
  _wavesData = NULL;
  
  DestroyWaterLayer(_waterData);
  _waterData = NULL;
  
  DestroyHourLayer(_hourData);
  _hourData = NULL;
  
  DestroyStatusLayer(_statusData);
  _statusData = NULL;
  
  DestroyMarkerLayer(_markerData);
  _markerData = NULL;
}

static void timer_handler(struct tm *tick_time, TimeUnits units_changed) {
  struct tm *localNow = getTime(tick_time);
  updateApp(localNow);
  
#ifndef RUN_TEST
  // Check for hourly vibrate
  if ((units_changed & HOUR_UNIT) != 0 && _settings.hourVibrate == 1 &&
      isHourInRange(localNow->tm_hour, _settings.hourVibrateStart, _settings.hourVibrateEnd)) {
    
    vibrate();
  }
#endif
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  // If the disable tap timer is active then we know we're in a period that
  // taps should be ignored.
  if (_ignoreTapTimer != NULL) {
    return;
  }
  
  struct tm *localNow = getTime(NULL);
  uint16_t hour = localNow->tm_hour;
  uint16_t minute = localNow->tm_min;
  uint16_t second = localNow->tm_sec;

  if (_duckData != NULL) {
    HandleTapDuckLayer(_duckData, hour, minute, second);
  }

  if (_santaData != NULL) {
    HandleTapSantaLayer(_santaData, hour, minute, second);
  }

  if (_sharkData != NULL) {
    HandleTapSharkLayer(_sharkData, hour, minute, second);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *tuple = dict_read_first(iterator);
  
  // Check for 24-hour format request from phone.
  if (tuple != NULL && tuple->key == KEY_REQUEST_SETUP_INFO) {
    MY_APP_LOG(APP_LOG_LEVEL_INFO, "Setup info request");
    sendSetupInfo();
    return;
  }

  while (tuple != NULL) {
    switch (tuple->key) {
      case KEY_CURRENT_VERSION:
        _settings.currentVersion = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Current version %i", (int) _settings.currentVersion);
        break;
      
      case KEY_HOUR_VIBRATE:
        _settings.hourVibrate = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Hourly vibrate %i", (int) _settings.hourVibrate);
        break;
      
      case KEY_HOUR_VIBRATE_START:
        _settings.hourVibrateStart = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Hourly vibrate start %i", (int) _settings.hourVibrateStart);
        break;
      
      case KEY_HOUR_VIBRATE_END:
        _settings.hourVibrateEnd = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Hourly vibrate end %i", (int) _settings.hourVibrateEnd);
        break;
      
      case KEY_BLUETOOTH_VIBRATE:
        _settings.bluetoothVibrate = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Bluetooth vibrate %i", (int) _settings.bluetoothVibrate);
        break;
      
      case KEY_SCENE_OVERRIDE:
        _settings.sceneOverride = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Scene override %i", (int) _settings.sceneOverride);
        break;
      
      case KEY_SHARK_VIBRATE:
        _settings.sharkVibrate = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Shark vibrate %i", (int) _settings.sharkVibrate);
        break;
      
      case KEY_SHARK_VIBRATE_START:
        _settings.sharkVibrateStart = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Shark vibrate start %i", (int) _settings.sharkVibrateStart);
        break;
      
      case KEY_SHARK_VIBRATE_END:
        _settings.sharkVibrateEnd = tuple->value->int32;
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Shark vibrate end %i", (int) _settings.sharkVibrateEnd);
        break;
      
      default:
        MY_APP_LOG(APP_LOG_LEVEL_ERROR, "Key %i not recognized", (int) tuple->key);
        break;
    }

    tuple = dict_read_next(iterator);
  }
  
  saveSettings(&_settings);
  showMessage(_settingsReceivedMsg, MESSAGE_SETTINGS_DURATION);    
  updateApp(getTime(NULL));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
}

static void outbox_sent_callback(DictionaryIterator *values, void *context) {
  Tuple *tuple = dict_read_first(values);
  
  while (tuple != NULL) {
    switch (tuple->key) {
      case KEY_CLOCK_24_HOUR:
        // Record the most recently sent clock format
        persist_write_int(KEY_CLOCK_24_HOUR, tuple->value->int32);
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Successfully sent clock format %i to phone", (int) tuple->value->int32);
        break;
      
      case KEY_INSTALLED_VERSION:
        MY_APP_LOG(APP_LOG_LEVEL_INFO, "Successfully sent installed version %i to phone", (int) tuple->value->int32);
        break;
      
      default:
        MY_APP_LOG(APP_LOG_LEVEL_ERROR, "Key %i not recognized", (int) tuple->key);
        break;
    }

    tuple = dict_read_next(values);
  }
}

static void outbox_failed_callback(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "outbox_failed_callback");
}

static void bluetooth_service_handler(bool connected) {
  if (connected == false) {
    showMessage(_bluetoothDisconnectMsg, MESSAGE_BLUETOOTH_DURATION);
    if (_settings.bluetoothVibrate) {
      vibrate(); 
    }
  }
  
  ShowBluetoothStatus(_statusData, !connected);
  UpdateBluetoothStatus(_statusData, connected);
}

static void battery_service_handler(BatteryChargeState charge_state) {
  ShowBatteryStatus(_statusData, (charge_state.is_charging || charge_state.is_plugged));
  UpdateBatteryStatus(_statusData, charge_state);
}

static void loadSettings(Settings *settings) {
  settings->currentVersion = readPersistentInt(KEY_CURRENT_VERSION, 0);
  settings->hourVibrate = readPersistentInt(KEY_HOUR_VIBRATE, 0);
  settings->hourVibrateStart = readPersistentInt(KEY_HOUR_VIBRATE_START, 9);
  settings->hourVibrateEnd = readPersistentInt(KEY_HOUR_VIBRATE_END, 18);
  settings->bluetoothVibrate = readPersistentInt(KEY_BLUETOOTH_VIBRATE, 1);
  settings->sceneOverride = readPersistentInt(KEY_SCENE_OVERRIDE, UNDEFINED_SCENE);
  settings->sharkVibrate = readPersistentInt(KEY_SHARK_VIBRATE, 1);
  settings->sharkVibrateStart = readPersistentInt(KEY_SHARK_VIBRATE_START, 9);
  settings->sharkVibrateEnd = readPersistentInt(KEY_SHARK_VIBRATE_END, 18);
  
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "Load settings: currentVersion=%i", (int) settings->currentVersion);
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "Load settings: hourVibrate=%i, Start=%i, End=%i",
             (int) settings->hourVibrate, (int) settings->hourVibrateStart, (int) settings->hourVibrateEnd);
  
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "Load settings: bluetoothVibrate=%i, sceneOverride=%i",
             (int) settings->bluetoothVibrate, (int) settings->sceneOverride);
  
  MY_APP_LOG(APP_LOG_LEVEL_INFO, "Load settings: sharkVibrate=%i, Start=%i, End=%i",
             (int) settings->sharkVibrate, (int) settings->sharkVibrateStart, (int) settings->sharkVibrateEnd);
}

static int32_t readPersistentInt(const uint32_t key, int32_t defaultValue) {
  if (persist_exists(key)) {
    return persist_read_int(key);  
  }
  
  return defaultValue;
}

static bool isHourInRange(int16_t hour, int16_t start, int16_t end) {
  if (start == end) {
    return true;
    
  } else if ((end > start) && (hour >= start) && (hour < end)) {
    return true;
    
  } else if ((end < start) && ((hour >= start) || (hour < end))) {
    return true;
  }
  
  return false;
}

static void saveSettings(Settings *settings) {
  persist_write_int(KEY_CURRENT_VERSION, settings->currentVersion);
  persist_write_int(KEY_HOUR_VIBRATE, settings->hourVibrate);
  persist_write_int(KEY_HOUR_VIBRATE_START, settings->hourVibrateStart);
  persist_write_int(KEY_HOUR_VIBRATE_END, settings->hourVibrateEnd);
  persist_write_int(KEY_BLUETOOTH_VIBRATE, settings->bluetoothVibrate);
  persist_write_int(KEY_SCENE_OVERRIDE, settings->sceneOverride);
  persist_write_int(KEY_SHARK_VIBRATE, settings->sharkVibrate);
  persist_write_int(KEY_SHARK_VIBRATE_START, settings->sharkVibrateStart);
  persist_write_int(KEY_SHARK_VIBRATE_END, settings->sharkVibrateEnd);
}

static void sendSetupInfo() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }
  
  Tuplet clock24Hour = TupletInteger(KEY_CLOCK_24_HOUR, clock_is_24h_style() ? 1 : 0);
  dict_write_tuplet(iter, &clock24Hour);
  Tuplet installedVersion = TupletInteger(KEY_INSTALLED_VERSION, INSTALLED_VERSION);
  dict_write_tuplet(iter, &installedVersion);
  dict_write_end(iter);
  app_message_outbox_send();
}

static void sharkWarnTimerCallback(void *callback_data) {
  _sharkWarnTimer = NULL;
  vibrate();
}

static void setSharkWarnTimer(SCENE scene, struct tm *tick_time) {
  bool timerActive = (scene == FRIDAY13 && _settings.sharkVibrate == 1 &&
                      isHourInRange(tick_time->tm_hour, _settings.sharkVibrateStart, _settings.sharkVibrateEnd) &&
                      tick_time->tm_min == (SHARK_SCENE_EAT_MINUTE - 1) && tick_time->tm_sec <= SHARK_SCENE_WARN_SECOND);
  
  if (timerActive == false && _sharkWarnTimer != NULL) {
    app_timer_cancel(_sharkWarnTimer);
    _sharkWarnTimer = NULL;
    
  } else if (timerActive && _sharkWarnTimer == NULL) {
    // Set the shark warn timer at the minute before shark eat minute
    _sharkWarnTimer = app_timer_register((SHARK_SCENE_WARN_SECOND - tick_time->tm_sec) * 1000, (AppTimerCallback) sharkWarnTimerCallback, NULL);
  }
}

static void messageTimerCallback(void *callback_data) {
  _messageTimer = NULL;
  if (_messageData != NULL) {
    DestroyMessageLayer(_messageData);
    _messageData = NULL;
  }
}

static void ignoreTapTimerCallback(void *callback_data) {
  _ignoreTapTimer = NULL;
}

static void showMessage(const char *text, uint32_t duration) {
  if (_messageTimer != NULL) {
    if (app_timer_reschedule(_messageTimer, duration) == false) {
      _messageTimer = NULL;
    }
  } 
  
  if (_messageTimer == NULL) {
    _messageTimer = app_timer_register(duration, messageTimerCallback, NULL);
  }
  
  if (_messageData == NULL) {
    _messageData = CreateMessageLayer(window_get_root_layer(_mainWindow), CHILD);
  }
  
  DrawMessageLayer(_messageData, text);
}

static struct tm* getTime(struct tm *real_time) {
  struct tm *localTime;

#ifdef RUN_TEST
  time_t now = TestUnitGetTime(_testUnitData); 
  localTime = localtime(&now);
#else
  if (real_time != NULL) {
    localTime = real_time;
    
  } else {
    time_t now = time(NULL);
    localTime = localtime(&now);
  }
#endif
  
  return localTime;
}

static void vibrate() {
  // Vibrations can trigger the tap handler, so ignore taps for a period of time.
  _ignoreTapTimer = app_timer_register(VIBES_SHORT_IGNORE_TAPS_TIME, (AppTimerCallback) ignoreTapTimerCallback, NULL);
  vibes_short_pulse(); 
}

static void updateApp(struct tm *tick_time) {
  drawWatchFace(tick_time);
  
#ifndef RUN_TEST
  setSharkWarnTimer(_scene, tick_time);
#endif
}

static void drawWatchFace(struct tm *tick_time) {
  uint16_t hour = tick_time->tm_hour;
  uint16_t minute = tick_time->tm_min;
  uint16_t second = tick_time->tm_sec;
  
  SCENE scene = getScene(tick_time);
  if (scene != _scene) {
    switchScene(scene);
  }
  
  DrawMarkerLayer(_markerData, hour, minute);
  DrawStatusLayer(_statusData, hour, minute);
  DrawHourLayer(_hourData, hour, minute);
  DrawWaterLayer(_waterData, hour, minute);
  DrawWavesLayer(_wavesData, hour, minute);
  
  drawScene(scene, hour, minute, second);
}

static void drawScene(SCENE scene, uint16_t hour, uint16_t minute, uint16_t second) {
  if (_duckData != NULL) {
    DrawDuckLayer(_duckData, hour, minute, second);
  }
  
  if (_sharkData != NULL) {
    DrawSharkLayer(_sharkData, hour, minute, second);
  }
  
  if (_santaData != NULL) {
    DrawSantaLayer(_santaData, hour, minute);
  }
}

static void switchScene(SCENE scene) {
  bool duckLayer = false;
  bool sharkLayer = false;
  bool santaLayer = false;
  
  switch (scene) {
    case CHRISTMAS:
      duckLayer = true;
      santaLayer = true;
      break;
    
    case DUCK:
    case THANKSGIVING:
    case VALENTINES:
      duckLayer = true;
      break;
    
    case FRIDAY13:
      duckLayer = true;
      sharkLayer = true;
      break;
    
    default:
      break;
  }

  animation_unschedule_all();
  
  if (duckLayer == true) {
    if (_duckData == NULL) {
      _duckData = CreateDuckLayer((Layer*) _waterData->inverterLayer, BELOW_SIBLING, scene);
      
    } else {
      SwitchSceneDuckLayer(_duckData, scene);
    }
  } else if (duckLayer == false && _duckData != NULL) {
    DestroyDuckLayer(_duckData);
    _duckData = NULL;
  }
  
  if (sharkLayer == true && _sharkData == NULL) {
    _sharkData = CreateSharkLayer((Layer*) _waterData->inverterLayer, BELOW_SIBLING, _duckData);
    
  } else if (sharkLayer == false && _sharkData != NULL) {
    DestroySharkLayer(_sharkData);
    _sharkData = NULL;
  }
  
  if (santaLayer == true && _santaData == NULL) {
    _santaData = CreateSantaLayer((Layer*) _waterData->inverterLayer, BELOW_SIBLING);
    
  } else if (santaLayer == false && _santaData != NULL) {
    DestroySantaLayer(_santaData);
    _santaData = NULL;
  }
  
  _scene = scene;
}

static SCENE getScene(struct tm *tick_time) {
#ifndef RUN_TEST
  if (_settings.sceneOverride >= THANKSGIVING && _settings.sceneOverride <= VALENTINES) {
    return _settings.sceneOverride;
  }
#endif
  
  if (tick_time->tm_mon == 11 && tick_time->tm_mday == 25) {
    return CHRISTMAS;
    
  } else if (tick_time->tm_mon == 1 && tick_time->tm_mday == 14) {
    return VALENTINES;
    
  } else if (tick_time->tm_wday == 5 && tick_time->tm_mday == 13) {
    return FRIDAY13;
    
  } else if (tick_time->tm_mon == 10 && tick_time->tm_wday == 4 && (tick_time->tm_mday >= 22 && tick_time->tm_mday <= 28)) {
    return THANKSGIVING;
  }
  
  return DUCK;
}
