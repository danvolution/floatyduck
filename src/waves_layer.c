#include <pebble.h>
#include "waves_layer.h"

// X position of the waves' left edge
static uint16_t _wavesCoordinateX[WAVE_COUNT] = { 4, 40, 76, 112 };

// Wave definition which is a group of horizontal GRects.
static GRect _waveRows[WAVE_HEIGHT] = {
  { {11, 0}, {6, 1} },
  { {6, 1}, {16, 1} },
  { {3, 2}, {22, 1} },
  { {0, 3}, {28, 1} }
};

static PropertyAnimation* _animation = NULL;

static void animationStoppedHandler(Animation *animation, bool finished, void *context);
static GRect offsetRect(GRect* rect, int16_t x, int16_t y);

WavesLayerData* CreateWavesLayer(Layer* relativeLayer, LayerRelation relation) {
  WavesLayerData* data = malloc(sizeof(WavesLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(WavesLayerData));
    
    // Set layer off screen initially
    data->layer = layer_create(GRect(0, -1 * WAVE_HEIGHT, SCREEN_WIDTH, WAVE_HEIGHT));
  
    for (int wave = 0; wave < WAVE_COUNT; wave++) {
      for (int waveRow = 0; waveRow < WAVE_HEIGHT; waveRow++) {
        data->childInverterLayers[wave * waveRow] = inverter_layer_create(offsetRect(&_waveRows[waveRow], _wavesCoordinateX[wave], 0));
        AddLayer(data->layer, (Layer*) data->childInverterLayers[wave * waveRow], CHILD);
      }
    }
  
    AddLayer(relativeLayer, data->layer, relation);
    data->lastUpdateMinute = -1;
  }
  
  return data;
}

void DrawWavesLayer(WavesLayerData* data, uint16_t hour, uint16_t minute) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  GRect newFrame = GRect(0, WATER_TOP(minute) - WAVE_HEIGHT, SCREEN_WIDTH, WAVE_HEIGHT);
  
  if (minute == 0 || firstDisplay) {
    layer_set_frame(data->layer, newFrame);

  } else if (_animation == NULL) {
    // Create the animation and schedule it.
    _animation = property_animation_create_layer_frame((Layer*) data->layer, NULL, &newFrame);
    animation_set_duration((Animation*) _animation, WATER_RISE_DURATION);
    animation_set_curve((Animation*) _animation, AnimationCurveLinear);
    animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
      .started = NULL,
      .stopped = (AnimationStoppedHandler) animationStoppedHandler,
    }, NULL);

    animation_schedule((Animation*) _animation);
  }
}

void DestroyWavesLayer(WavesLayerData* data) {
  if (data != NULL) {
    // Destroy wave children InverterLayer
    for (int waveIndex = 0; waveIndex < (WAVE_HEIGHT * WAVE_COUNT); waveIndex++) {
      if (data->childInverterLayers[waveIndex] != NULL) {
        inverter_layer_destroy(data->childInverterLayers[waveIndex]);
        data->childInverterLayers[waveIndex] = NULL;
      }
    }
    
    // Destroy waves Layer
    if (data->layer != NULL) {
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    free(data);
  }
}

// Offset a GRect by x and y coordinates. x and y may be negative.
// Returns: A new GRect offset by x and y.
static GRect offsetRect(GRect* rect, int16_t x, int16_t y) {
  return GRect(rect->origin.x + x, rect->origin.y + y, rect->size.w, rect->size.h);
}

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
}