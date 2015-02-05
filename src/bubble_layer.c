#include <pebble.h>
#include "bubble_layer.h"

#define MAX_BUBBLES 16
#define BUBBLE_TIMER_INTERVAL 100
#define WIGGLE_COUNT 16

typedef struct {
  GPoint origin;
  uint16_t size;
  uint16_t speed;
  uint16_t delayStartIntervals;
} Bubble;

static int16_t _wiggles[WIGGLE_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, -2, 2 };

static Bubble _bubbles[MAX_BUBBLES];
static AppTimer *_bubbleTimer = NULL;
static int16_t _lastUpdateMinute = -1;
static volatile bool _updateBubbles = false;
static uint16_t _bubbleStartIndex = 0;
static uint16_t _bubbleEndIndex = 0;

static void bubbleLayerUpdateProc(Layer *layer, GContext *ctx);
static void bubbleTimerCallback(void *callback_data);
static int16_t getBubbleWiggle(uint16_t size);

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
  
  data->lastUpdateMinute = minute;
  _lastUpdateMinute = minute;
}

void DestroyBubbleLayer(BubbleLayerData* data) {
  if (_bubbleTimer != NULL) {
    app_timer_cancel(_bubbleTimer);
    _bubbleTimer = NULL;
  }
  
  if (data != NULL) {
    if (data->layer != NULL) {
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    free(data);
  }
}

void AddBubble(BubbleLayerData* data, GPoint startOrigin, uint16_t size, uint16_t speed, uint16_t delayStart) {
  uint16_t start = _bubbleStartIndex;
  uint16_t end = _bubbleEndIndex;
  
  if (isBufferFull(start, end, MAX_BUBBLES)) {
    return;
  }

  _bubbles[end].origin = startOrigin;
  _bubbles[end].size = size;
  _bubbles[end].speed = speed;
  _bubbles[end].delayStartIntervals = delayStart;

  end++;
  if (end == MAX_BUBBLES) {
    end = 0;
  }
  
  _bubbleEndIndex = end;
  
  if (_bubbleTimer == NULL) {
    _bubbleTimer = app_timer_register(BUBBLE_TIMER_INTERVAL, (AppTimerCallback) bubbleTimerCallback, (void*) data);
  }
}

static void bubbleLayerUpdateProc(Layer *layer, GContext *ctx) {
  uint16_t start = _bubbleStartIndex;
  uint16_t end = _bubbleEndIndex;

  if (start == end || _lastUpdateMinute == -1) {
    return;
  }
  
  uint16_t index = start;
  bool updateBubbles = _updateBubbles;
  _updateBubbles = false;

  int16_t waterTop = WATER_TOP(_lastUpdateMinute);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  
  // Iterate through bubbles and move them by speed pixels. Remove any that have floated all
  // the way to the top.
  while (index != end) {
    if (_bubbles[index].delayStartIntervals > 0) {
      _bubbles[index].delayStartIntervals--;
      
    } else {
      if (updateBubbles) {
        _bubbles[index].origin.y -= _bubbles[index].speed;
        _bubbles[index].origin.x += getBubbleWiggle(_bubbles[index].size);
      }
        
      if (_bubbles[index].origin.y < waterTop) {
        // Bubble now has reached the top. Move the start if this bubble is at the head.
        // Otherwise it will have to wait until the bubble(s) before it hit the top.
        if (index == start) {
          start = index + 1;
          if (start == MAX_BUBBLES) {
            start = 0;
          }
        }
      } else {
        graphics_fill_circle(ctx, _bubbles[index].origin, _bubbles[index].size);
      }
    }
    
    index++;
    if (index == MAX_BUBBLES) {
      index = 0;
    }
  }
  
  _bubbleStartIndex = start;
}

static void bubbleTimerCallback(void *callback_data) {
  _bubbleTimer = NULL;
  BubbleLayerData *data = (BubbleLayerData*) callback_data;
  
  uint16_t start = _bubbleStartIndex;
  uint16_t end = _bubbleEndIndex;
  if (start != end) {
    _bubbleTimer = app_timer_register(BUBBLE_TIMER_INTERVAL, (AppTimerCallback) bubbleTimerCallback, (void*) data);
  }

  _updateBubbles = true;
  layer_mark_dirty(data->layer);  
}

static int16_t getBubbleWiggle(uint16_t size) {
  uint16_t index = rand() % WIGGLE_COUNT;
  return _wiggles[index];
}