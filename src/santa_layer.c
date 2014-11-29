#include <pebble.h>
#include "santa_layer.h"

#define SANTA_PASS_COUNT 7
// Have santa fly upwards by PASS_OFFSET_Y y coordinates.
#define PASS_OFFSET_Y 28
#define SANTA_ANIMATION_DURATION 10000

typedef struct {
  uint16_t startMinute;
  int16_t coordinateY;
  bool leftToRight;
} SantaPass;

static SantaPass _santaPass[SANTA_PASS_COUNT] = {
  { 0, 76, true},
  { 5, 68, false},
  { 10, 60, true},
  { 15, 52, false},
  { 20, 44, true},
  { 25, 36, false},
  { 30, 28, true}
};

static PropertyAnimation* _animation = NULL;

static SantaPass* getSantaPass(uint16_t minute);
static GRect getSantaStartFrame(SantaPass* pass, int16_t imageWidth, int16_t imageHeight);
static GRect getSantaStopFrame(SantaPass* pass, int16_t imageWidth, int16_t imageHeight);
static void animationStoppedHandler(Animation *animation, bool finished, void *context);

SantaLayerData* CreateSantaLayer(Layer* relativeLayer, LayerRelation relation) {
  SantaLayerData* data = malloc(sizeof(SantaLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(SantaLayerData));
    data->santa.layer = bitmap_layer_create(GRect(0, -5, 5, 5));
    bitmap_layer_set_compositing_mode(data->santa.layer, GCompOpAnd);
    AddLayer(relativeLayer, (Layer*) data->santa.layer, relation);    
    data->lastUpdateMinute = -1;
  }
  
  return data;
}

void DrawSantaLayer(SantaLayerData* data, uint16_t hour, uint16_t minute) {  
  // Exit if this minute has already been drawn.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  data->lastUpdateMinute = minute;
  
  if (_animation != NULL) {
    return;
  }
  
  SantaPass *pass = getSantaPass(minute);
  if (pass == NULL) {
    return;
  }

  uint32_t santaResourceId = pass->leftToRight ? RESOURCE_ID_IMAGE_SANTA : RESOURCE_ID_IMAGE_SANTA_LEFT;
  if (data->santa.resourceId != santaResourceId) {
    if (data->santa.bitmap != NULL) {
      gbitmap_destroy(data->santa.bitmap);
      data->santa.bitmap = NULL;
      data->santa.resourceId = 0;
    }
    
    data->santa.bitmap = gbitmap_create_with_resource(santaResourceId);
    data->santa.resourceId = santaResourceId;
    bitmap_layer_set_bitmap(data->santa.layer, data->santa.bitmap);
  }

  GRect startFrame = getSantaStartFrame(pass, data->santa.bitmap->bounds.size.w, data->santa.bitmap->bounds.size.h);
  GRect stopFrame = getSantaStopFrame(pass, data->santa.bitmap->bounds.size.w, data->santa.bitmap->bounds.size.h);
  
  layer_set_frame((Layer*) data->santa.layer, startFrame);    
  layer_set_bounds((Layer*) data->santa.layer, GRect(0, 0, startFrame.size.w, startFrame.size.h));    
  
  // Create the animation and schedule it.
  _animation = property_animation_create_layer_frame((Layer*) data->santa.layer, NULL, &stopFrame);
  animation_set_duration((Animation*) _animation, SANTA_ANIMATION_DURATION);
  animation_set_curve((Animation*) _animation, AnimationCurveLinear);
  animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
    .started = NULL,
    .stopped = (AnimationStoppedHandler) animationStoppedHandler,
  }, NULL);

  animation_schedule((Animation*) _animation);
}

void DestroySantaLayer(SantaLayerData* data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->santa);
    free(data);
  }  
}

static SantaPass* getSantaPass(uint16_t minute) {
  for (int passIndex = 0; passIndex < SANTA_PASS_COUNT; passIndex++) {
    if (minute == _santaPass[passIndex].startMinute) {
      return &_santaPass[passIndex];
    }
  }
  
  return NULL;
}

static GRect getSantaStartFrame(SantaPass* pass, int16_t imageWidth, int16_t imageHeight) {
  int16_t x = pass->leftToRight ? (0 - imageWidth) : SCREEN_WIDTH;
  
  return (GRect) { .origin = { x, pass->coordinateY }, .size = { imageWidth, imageHeight} };
}

static GRect getSantaStopFrame(SantaPass* pass, int16_t imageWidth, int16_t imageHeight) {
  int16_t x = pass->leftToRight ? SCREEN_WIDTH : (0 - imageWidth);
  
  return (GRect) { .origin = { x, (pass->coordinateY - PASS_OFFSET_Y) }, .size = { imageWidth, imageHeight} };
}

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
}
