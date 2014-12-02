#include <pebble.h>
#include "bubble_layer.h"

#define BUBBLE_CLUSTERS 5
#define BUBBLES_PER_CLUSTER 3
#define BEGIN_BUBBLES_MINUTE 55

typedef struct {
  GPoint origin;
  uint16_t size;
} Bubble;

static Bubble _bubbles[BUBBLE_CLUSTERS][BUBBLES_PER_CLUSTER] = {
  { {{38, 21}, 1}, {{43, 21}, 2}, {{35, 29}, 1} },
  { {{34, 41}, 1}, {{29, 43}, 2}, {{34, 58}, 1} },
  { {{28, 65}, 2}, {{28, 70}, 1}, {{32, 88}, 1} },
  { {{23, 101}, 1}, {{26, 103}, 1}, {{25, 114}, 1} },
  { {{23, 128}, 2}, {{24, 133}, 1}, {{21, 140}, 1} } 
};

static uint16_t _currentMinute = 0;
static AppTimer *_timer = NULL;

static void bubbleLayerUpdateProc(Layer *layer, GContext *ctx);
static void timerCallback(void *callback_data);

BubbleLayerData* CreateBubbleLayer(Layer* relativeLayer, LayerRelation relation) {
  BubbleLayerData* data = malloc(sizeof(BubbleLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(BubbleLayerData));
    data->layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    layer_set_update_proc(data->layer, bubbleLayerUpdateProc);
    AddLayer(relativeLayer, data->layer, relation);
    data->lastUpdateMinute = -1;
  }
  
  return data;
}

void DrawBubbleLayer(BubbleLayerData* data, uint16_t hour, uint16_t minute) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;

  if (minute < BEGIN_BUBBLES_MINUTE || firstDisplay) {
    _currentMinute = minute;
    layer_mark_dirty(data->layer);  
    return;
  }
  
  // Delay layer update by the water rise animation duration.
  data->nextMinute = minute;
  _timer = app_timer_register(WATER_RISE_DURATION, (AppTimerCallback) timerCallback, (void*) data);
}

void DestroyBubbleLayer(BubbleLayerData* data) {
  if (_timer != NULL) {
    app_timer_cancel(_timer);
    _timer = NULL;
  }
  
  if (data != NULL) {
    if (data->layer != NULL) {
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    free(data);
  }
}

static void bubbleLayerUpdateProc(Layer *layer, GContext *ctx) {
  if (_currentMinute < BEGIN_BUBBLES_MINUTE) {
    return;
  }
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  
  for (int minute = BEGIN_BUBBLES_MINUTE; minute <= _currentMinute; minute++) {
    for (int bubble = 0; bubble < BUBBLES_PER_CLUSTER; bubble++) {
      graphics_fill_circle(ctx, _bubbles[minute - BEGIN_BUBBLES_MINUTE][bubble].origin, 
                           _bubbles[minute - BEGIN_BUBBLES_MINUTE][bubble].size);
    }
  }
}

static void timerCallback(void *callback_data) {
  _timer = NULL;
  BubbleLayerData *data = (BubbleLayerData*) callback_data;
  _currentMinute = data->nextMinute;
  layer_mark_dirty(data->layer);  
}