#include <pebble.h>
#include "santa_layer.h"

#define SANTA_IMAGE_WIDTH 142
#define SANTA_IMAGE_HEIGHT 29 
  
// Have santa fly upwards by PASS_OFFSET_Y y coordinates.
#define PASS_OFFSET_Y 28
  
// The highest Y coordinate Santa's fly-by can start.
#define TOP_PASS_COORDINATE_Y 24
  
// The lowest Y coordinate Santa's fly-by can start.
#define BOTTOM_PASS_COORDINATE_Y 76

typedef struct {
  uint32_t duration;
  uint32_t delay;
  uint32_t resourceId;
  GRect start;
  GRect end;
} SantaAnimation;

static PropertyAnimation *_animation = NULL;

static SantaAnimation* getSantaAnimation(uint16_t minute, bool runNow, bool firstDisplay);
static void runAnimation(SantaLayerData *data, SantaAnimation *animation);
static void animationStoppedHandler(Animation *animation, bool finished, void *context);

SantaLayerData* CreateSantaLayer(Layer *relativeLayer, LayerRelation relation) {
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

void DrawSantaLayer(SantaLayerData *data, uint16_t hour, uint16_t minute) {  
  // Exit if this minute has already been drawn.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  
  // Exit if animation already running
  if (_animation != NULL) {
    return;
  }
  
  // Check if there is an animation for the current minute. Force animation if watchface just loaded.
  SantaAnimation *santaAnimation = getSantaAnimation(minute, firstDisplay, firstDisplay);
  if (santaAnimation == NULL) {
    return;
  }

  runAnimation(data, santaAnimation);
  free(santaAnimation);
}

void DestroySantaLayer(SantaLayerData *data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->santa);
    free(data);
  }  
}

void HandleTapSantaLayer(SantaLayerData *data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if animation or rotation already running
  if (_animation != NULL) {
    return;
  }
  
  // Force animation on shake.
  SantaAnimation *santaAnimation = getSantaAnimation(minute, true, false);
  if (santaAnimation == NULL) {
    return;
  }

  runAnimation(data, santaAnimation);
  free(santaAnimation);
}

static void runAnimation(SantaLayerData *data, SantaAnimation *santaAnimation) {
  BitmapGroupSetBitmap(&data->santa, santaAnimation->resourceId);
  
  layer_set_frame((Layer*) data->santa.layer, santaAnimation->start);    
  layer_set_bounds((Layer*) data->santa.layer, GRect(0, 0, santaAnimation->start.size.w, santaAnimation->start.size.h));    
  
  // Create the animation and schedule it.
  _animation = property_animation_create_layer_frame((Layer*) data->santa.layer, NULL, &santaAnimation->end);
  animation_set_duration((Animation*) _animation, santaAnimation->duration);
  animation_set_delay((Animation*) _animation, santaAnimation->delay);
  animation_set_curve((Animation*) _animation, AnimationCurveLinear);
  animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
    .started = NULL,
    .stopped = (AnimationStoppedHandler) animationStoppedHandler,
  }, NULL);

  animation_schedule((Animation*) _animation);
}

static SantaAnimation* getSantaAnimation(uint16_t minute, bool runNow, bool firstDisplay) {
  // No animations past LAST_ANIMATION_MINUTE since the water level is too high at this point.
  if (minute > LAST_SANTA_ANIMATION_MINUTE) {
    return NULL;
  }
  
  // Create animation every 5 minutes or if displaying for the first time.
  if ((minute % 5 != 0) && (runNow == false)) {
    return NULL;
  }
  
  SantaAnimation *santaAnimation = malloc(sizeof(SantaAnimation));
  if (santaAnimation == NULL) {
    return NULL;
  }

  bool flyRight = (minute % 2 == 0);
  
  // Position Santa's fly-by in the fly zone between y coordinates BOTTOM_PASS_COORDINATE_Y and TOP_PASS_COORDINATE_Y
  int16_t coordinateY = BOTTOM_PASS_COORDINATE_Y - (BOTTOM_PASS_COORDINATE_Y - TOP_PASS_COORDINATE_Y) * minute / LAST_SANTA_ANIMATION_MINUTE;

  santaAnimation->resourceId = flyRight ? RESOURCE_ID_IMAGE_SANTA : RESOURCE_ID_IMAGE_SANTA_LEFT;
  santaAnimation->duration = SANTA_ANIMATION_DURATION;
  santaAnimation->delay = (firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0);
  santaAnimation->start = (GRect) { 
    .origin = { flyRight ? (0 - SANTA_IMAGE_WIDTH) : SCREEN_WIDTH, coordinateY }, 
    .size = { SANTA_IMAGE_WIDTH, SANTA_IMAGE_HEIGHT } 
  };
  
  santaAnimation->end = (GRect) { 
    .origin = { flyRight ? SCREEN_WIDTH : (0 - SANTA_IMAGE_WIDTH), (coordinateY - PASS_OFFSET_Y) }, 
    .size = { SANTA_IMAGE_WIDTH, SANTA_IMAGE_HEIGHT} 
  };
  
  return santaAnimation;
}

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
}
