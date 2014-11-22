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
  { {{36, 21}, 1}, {{41, 21}, 2}, {{33, 29}, 1} },
  { {{34, 41}, 1}, {{29, 43}, 2}, {{34, 58}, 1} },
  { {{28, 65}, 2}, {{28, 70}, 1}, {{32, 88}, 1} },
  { {{23, 101}, 1}, {{26, 103}, 1}, {{25, 114}, 1} },
  { {{23, 128}, 2}, {{24, 133}, 1}, {{21, 140}, 1} } 
};

static uint16_t _currentMinute = 0;

static void bubbleLayerUpdateProc(Layer *layer, GContext *ctx);

BubbleLayerData* CreateBubbleLayer(Layer* relativeLayer, LayerRelation relation) {
  BubbleLayerData* data = malloc(sizeof(BubbleLayerData));
  if (data != NULL) {
    data->layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    layer_set_update_proc(data->layer, bubbleLayerUpdateProc);
    AddLayer(relativeLayer, data->layer, relation);
  }
  
  return data;
}

void DrawBubbleLayer(BubbleLayerData* data, uint16_t hour, uint16_t minute) {
  _currentMinute = minute;
  layer_mark_dirty(data->layer);
}

void DestroyBubbleLayer(BubbleLayerData* data) {
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
