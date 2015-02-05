#include <pebble.h>
#include "heart_layer.h"

#define HEART_TIMER_INTERVAL 100

static AppTimer *_heartTimer = NULL;
static uint16_t _heartStartIndex = 0;
static uint16_t _heartEndIndex = 0;

static void heartTimerCallback(void *callback_data);

HeartLayerData* CreateHeartLayer(Layer* relativeLayer, LayerRelation relation) {
  HeartLayerData* data = malloc(sizeof(HeartLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(HeartLayerData));
    data->layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    AddLayer(relativeLayer, data->layer, relation);
    data->bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART);
  }
  
  return data;
}

void DrawHeartLayer(HeartLayerData* data, uint16_t hour, uint16_t minute) {
}

void DestroyHeartLayer(HeartLayerData* data) {
  if (_heartTimer != NULL) {
    app_timer_cancel(_heartTimer);
    _heartTimer = NULL;
  }
  
  if (data != NULL) {
    for (int heartIndex = 0; heartIndex < MAX_HEARTS; heartIndex++) {
      if (data->childHearts[heartIndex].animation != NULL) {
        if (animation_is_scheduled((Animation*) data->childHearts[heartIndex].animation)) {
          animation_unschedule((Animation*) data->childHearts[heartIndex].animation);
        }
        
        property_animation_destroy(data->childHearts[heartIndex].animation);
        data->childHearts[heartIndex].animation = NULL;
      }
      
      if (data->childHearts[heartIndex].bitmapLayer != NULL) {
        layer_remove_from_parent((Layer*) data->childHearts[heartIndex].bitmapLayer);
        bitmap_layer_destroy(data->childHearts[heartIndex].bitmapLayer);
        data->childHearts[heartIndex].bitmapLayer = NULL;
      }
    }
    
    if (data->layer != NULL) {
      layer_destroy(data->layer);
      data->layer = NULL;
    }
    
    if (data->bitmap != NULL) {
      gbitmap_destroy(data->bitmap);
      data->bitmap = NULL;
    }
    
    free(data);
  }
}

void AddHeart(HeartLayerData* data, GPoint startOrigin, GPoint endOrigin, uint16_t speed, uint16_t delayStart) {
  uint16_t start = _heartStartIndex;
  uint16_t end = _heartEndIndex;
  
  // Return if full
  if (isBufferFull(start, end, MAX_HEARTS)) {
    return;
  }
  
  GRect startFrame = GRect(startOrigin.x - (data->bitmap->bounds.size.w / 2), startOrigin.y - (data->bitmap->bounds.size.h / 2),
                           data->bitmap->bounds.size.w, data->bitmap->bounds.size.h);
  
  GRect stopFrame = GRect(endOrigin.x - (data->bitmap->bounds.size.w / 2), endOrigin.y - (data->bitmap->bounds.size.h / 2),
                          data->bitmap->bounds.size.w, data->bitmap->bounds.size.h);

  data->childHearts[end].bitmapLayer = bitmap_layer_create(startFrame);
  
  bitmap_layer_set_compositing_mode(data->childHearts[end].bitmapLayer, GCompOpAnd);
  bitmap_layer_set_bitmap(data->childHearts[end].bitmapLayer, data->bitmap);
  AddLayer(data->layer, (Layer*) data->childHearts[end].bitmapLayer, CHILD);
  
  // Create the animation and schedule it.
  data->childHearts[end].animation = property_animation_create_layer_frame((Layer*) data->childHearts[end].bitmapLayer, NULL, &stopFrame);
  animation_set_duration((Animation*) data->childHearts[end].animation, (startOrigin.y - endOrigin.y) * speed);
  animation_set_delay((Animation*) data->childHearts[end].animation, delayStart);
  animation_set_curve((Animation*) data->childHearts[end].animation, AnimationCurveLinear);
  animation_schedule((Animation*) data->childHearts[end].animation);
  
  end++;
  if (end == MAX_HEARTS) {
    end = 0;
  }
  
  _heartEndIndex = end;
  
  if (_heartTimer == NULL) {
    _heartTimer = app_timer_register(HEART_TIMER_INTERVAL, (AppTimerCallback) heartTimerCallback, (void*) data);
    MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "Heart timer started");
  }
}

static void heartTimerCallback(void *callback_data) {
  uint16_t start = _heartStartIndex;
  uint16_t end = _heartEndIndex;
  
  _heartTimer = NULL;

  // Return if empty
  if (start == end) {
    MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "Heart timer stopped");
    return;
  }
  
  HeartLayerData *data = (HeartLayerData*) callback_data;
  
  // Remove any hearts that have completed.
  uint16_t index = start;
  while (index != end) {
    if (data->childHearts[index].bitmapLayer != NULL) {
      if (data->childHearts[index].animation != NULL &&
          animation_is_scheduled((Animation*) data->childHearts[index].animation) == false) {
        
        property_animation_destroy(data->childHearts[index].animation);
        data->childHearts[index].animation = NULL;
        
        layer_remove_from_parent((Layer*) data->childHearts[index].bitmapLayer);
        bitmap_layer_destroy(data->childHearts[index].bitmapLayer);
        data->childHearts[index].bitmapLayer = NULL;
      }
    }
    
    if (data->childHearts[index].bitmapLayer == NULL &&
        data->childHearts[index].animation == NULL) {
    
      // Heart is complete. Move the start if this heart is at the head.
      // Otherwise it will have to wait until the heart(s) before it complete.
      if (index == start) {
        start = index + 1;
        if (start == MAX_HEARTS) {
          start = 0;
        }
      }
    }
    
    index++;
    if (index == MAX_HEARTS) {
      index = 0;
    }
  }
  
  _heartStartIndex = start;
  
  if (start != end) {
    _heartTimer = app_timer_register(HEART_TIMER_INTERVAL, (AppTimerCallback) heartTimerCallback, (void*) data);
    
  } else {
    MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "Heart timer stopped");    
  }
}