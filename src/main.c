//#define RUN_TEST true
  
#include <pebble.h>
#include "common.h"
#include "marker_layer.h"
#include "hour_layer.h"
#include "water_layer.h"
#include "waves_layer.h"
#include "duck_layer.h"
#include "bubble_layer.h"
#include "shark_layer.h"
#include "santa_layer.h"
  
#ifdef RUN_TEST
#include "test_unit.h"
#endif

static Window* _mainWindow = NULL;
static MarkerLayerData* _markerData = NULL;
static HourLayerData* _hourData = NULL;
static WaterLayerData* _waterData = NULL;
static WavesLayerData* _wavesData = NULL;
static DuckLayerData* _duckData = NULL;
static BubbleLayerData* _bubbleData = NULL;
static SharkLayerData* _sharkData = NULL;
static SantaLayerData* _santaData = NULL;

#ifdef RUN_TEST
static TestUnitData* _testUnitData = NULL;
#endif
  
SCENE _scene;

static void init();
static void deinit();
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void timer_handler(struct tm *tick_time, TimeUnits units_changed);
static void drawWatchFace();
static void drawScene(SCENE scene, uint16_t hour, uint16_t minute, uint16_t second);
static SCENE getScene(struct tm* now);
static void switchScene(SCENE scene);

int main(void) {
  init();
  app_event_loop();
  deinit();
}

static void init() {
  _scene = UNDEFINED;
  
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
}

static void deinit() {
  animation_unschedule_all();

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
  _hourData = CreateHourLayer(window_get_root_layer(_mainWindow), CHILD);
  _waterData = CreateWaterLayer(window_get_root_layer(_mainWindow), CHILD);
  _wavesData = CreateWavesLayer(window_get_root_layer(_mainWindow), CHILD);
  
  drawWatchFace();
}

static void main_window_unload(Window *window) {
  if (_santaData != NULL) {
    DestroySantaLayer(_santaData);    
    _santaData = NULL;
  }

  if (_sharkData != NULL) {
    DestroySharkLayer(_sharkData);    
    _sharkData = NULL;
  }

  if (_bubbleData != NULL) {
    DestroyBubbleLayer(_bubbleData);    
    _bubbleData = NULL;
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
  
  DestroyMarkerLayer(_markerData);
  _markerData = NULL;
}

static void timer_handler(struct tm *tick_time, TimeUnits units_changed) {
  drawWatchFace();
}

static void drawWatchFace() {
#ifdef RUN_TEST
  time_t now = TestUnitGetTime(_testUnitData); 
#else
  time_t now = time(NULL); 
#endif
  
  struct tm* localNow = localtime(&now);
  uint16_t hour = localNow->tm_hour;
  uint16_t minute = localNow->tm_min;
  uint16_t second = localNow->tm_sec;
  
  SCENE scene = getScene(localNow);
  if (scene != _scene) {
    switchScene(scene);
  }
  
  DrawMarkerLayer(_markerData, hour, minute);
  DrawHourLayer(_hourData, hour, minute);
  DrawWaterLayer(_waterData, hour, minute);
  DrawWavesLayer(_wavesData, hour, minute);
  
  drawScene(scene, hour, minute, second);
}

static void drawScene(SCENE scene, uint16_t hour, uint16_t minute, uint16_t second) {
  if (_duckData != NULL) {
    DrawDuckLayer(_duckData, hour, minute);
  }
  
  if (_bubbleData != NULL) {
    DrawBubbleLayer(_bubbleData, hour, minute);
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
  bool bubbleLayer = false;
  bool sharkLayer = false;
  bool santaLayer = false;
  
  switch (scene) {
    case CHRISTMAS:
      santaLayer = true;
      duckLayer = true;
      bubbleLayer = true;
      break;
    
    case DUCK:
      duckLayer = true;
      bubbleLayer = true;
      break;
    
    case FRIDAY13:
      duckLayer = true;
      sharkLayer = true;
      break;
    
    case THANKSGIVING:
      duckLayer = true;
      break;
    
    default:
      break;
  }

  animation_unschedule_all();
  
  if (duckLayer == true) {
    if (_duckData == NULL) {
      _duckData = CreateDuckLayer((Layer*) _waterData->inverterLayer, BELOW_SIBLING, scene);
      
    } else {
      SwitchDuckScene(_duckData, scene);
    }
  } else if (duckLayer == false && _duckData != NULL) {
    DestroyDuckLayer(_duckData);
    _duckData = NULL;
  }
  
  if (bubbleLayer == true && _bubbleData == NULL) {
    _bubbleData = CreateBubbleLayer((Layer*) _waterData->inverterLayer, BELOW_SIBLING);
    
  } else if (bubbleLayer == false && _bubbleData != NULL) {
    DestroyBubbleLayer(_bubbleData);
    _bubbleData = NULL;
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

static SCENE getScene(struct tm* now) {
  if (now->tm_mon == 11 && now->tm_mday == 25) {
    return CHRISTMAS;
    
  } else if (now->tm_wday == 5 && now->tm_mday == 13) {
    return FRIDAY13;
    
  } else if (now->tm_mon == 10 && now->tm_wday == 4 && (now->tm_mday >= 22 && now->tm_mday <= 28)) {
    return THANKSGIVING;
  }
  
  return DUCK;
}