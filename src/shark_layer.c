#include <pebble.h>
#include "shark_layer.h"

// RESOURCE_ID_IMAGE_SHARK_LEFT image width
#define SHARK_WIDTH 79
  
// Number of full screen shark passes
#define SHARK_PASS_COUNT 3
  
// Number of animations for eating the duck
#define SHARK_EAT_COUNT 13
  
// Index in _sharkEat at which the duck layer should be hidden.
#define EAT_ANIMATION_HIDE_DUCK_INDEX 6
  
// Control speed of eat animation. Units are milliseconds per coordinate X.
#define EAT_ANIMATION_SPEED_FACTOR 30
  
// Seconds past the eat animation minute that the animation should not be run.
// Otherwise the animation would spill over to the next minute.
#define ANIMATION_CUTOFF_SECOND 45

typedef enum { SHARK_UNDEFINED, SHARK_PASS, SHARK_EAT } SharkAnimationType;

typedef struct {
  SharkAnimationType type;
  uint16_t startMinute;
  uint32_t duration;
  uint32_t resourceId;
  GPoint startPoint;
  GPoint endPoint;
} SharkAnimation;

static SharkAnimation _sharkEat[SHARK_EAT_COUNT] = {
  { SHARK_EAT, SHARK_SCENE_EAT_MINUTE, 37 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_LEFT, { SCREEN_WIDTH, 24 }, { 107, 10 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_1, { 107, 10 }, { 102, 8 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_2, { 102, 8 }, { 97, 6 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_3, { 97, 6 }, { 92, 4 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_4, { 92, 4 }, { 87, 2 } },
  { SHARK_EAT, 0, 13 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_5, { 87, 2 }, { 74, 0 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_EAT_1, { 65, 0 }, { 60, 2 } }, // Image size change by +9 px
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_EAT_2, { 60, 2 }, { 55, 4 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_EAT_3, { 55, 4 }, { 50, 6 } },
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_EAT_4, { 50, 6 }, { 45, 8 } }, 
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_2, { 54, 6 }, { 49, 8 } }, // Image size change by -9 px
  { SHARK_EAT, 0, 5 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_OPEN_1, { 49, 8 }, { 44, 10 } },
  { SHARK_EAT, 0, 123 * EAT_ANIMATION_SPEED_FACTOR, RESOURCE_ID_IMAGE_SHARK_LEFT, { 44, 10 }, { (0 - SHARK_WIDTH), 36 } }
};

static SharkAnimation _sharkPass[SHARK_PASS_COUNT] = {
  { SHARK_PASS, 19, 8000, RESOURCE_ID_IMAGE_SHARK, { (0 - SHARK_WIDTH), 129 }, { SCREEN_WIDTH, 129 } },
  { SHARK_PASS, 30, 8000, RESOURCE_ID_IMAGE_SHARK_LEFT, { SCREEN_WIDTH, 99 }, { (0 - SHARK_WIDTH), 99 } },
  { SHARK_PASS, 41, 8000, RESOURCE_ID_IMAGE_SHARK, { (0 - SHARK_WIDTH), 69 }, { SCREEN_WIDTH, 69 } }
};

static PropertyAnimation* _animation = NULL;
static SharkAnimationType _animationType = SHARK_UNDEFINED;
static uint16_t _eatAnimationIndex = 0;
static AppTimer *_eatTimer = NULL;
  
static void runAnimation(SharkLayerData* data, SharkAnimation* sharkAnimation);
static SharkAnimation* getSharkAnimation(uint16_t minute, uint16_t second);
static void animationStoppedHandler(Animation *animation, bool finished, void *context);
static void eatTimerCallback(void *callback_data);

SharkLayerData* CreateSharkLayer(Layer* relativeLayer, LayerRelation relation, DuckLayerData* duckData) {
  SharkLayerData* data = malloc(sizeof(SharkLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(SharkLayerData));
    data->shark.layer = bitmap_layer_create(GRect(0, -5, 5, 5));
    bitmap_layer_set_compositing_mode(data->shark.layer, GCompOpAnd);
    AddLayer(relativeLayer, (Layer*) data->shark.layer, relation);    
    data->hidden = false;
    data->lastUpdateMinute = -1;
    data->duckData = duckData;
  }
  
  return data;
}

void DrawSharkLayer(SharkLayerData* data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
    
  data->lastUpdateMinute = minute;
  
  SharkAnimation* sharkAnimation = getSharkAnimation(minute, second);
  if (sharkAnimation != NULL && _animation == NULL) {
    if (sharkAnimation->type == SHARK_EAT) {
      _eatAnimationIndex = 0;
    }
	
    SetLayerHidden((Layer*) data->shark.layer, &data->hidden, false);
    runAnimation(data, sharkAnimation);
	
  } else if (_animation == NULL) {
    SetLayerHidden((Layer*) data->shark.layer, &data->hidden, true);
  }
  
  // The duck layer is showing if watchface was loaded between 53:45 and 53:59, so hide it if
  // animation isn't running.
  if (minute == SHARK_SCENE_EAT_MINUTE && _animation == NULL && data->duckData->hidden == false) {
      layer_set_hidden((Layer*) data->duckData->duck.layer, true);
      data->duckData->hidden = true;
  }
}

void DestroySharkLayer(SharkLayerData* data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->shark);
    free(data);
  }  
}

static void runAnimation(SharkLayerData* data, SharkAnimation* sharkAnimation) {
  BitmapGroupSetBitmap(&data->shark, sharkAnimation->resourceId);

  GRect startFrame = (GRect) { .origin = sharkAnimation->startPoint, .size = data->shark.bitmap->bounds.size };
  GRect stopFrame = (GRect) { .origin = sharkAnimation->endPoint, .size = data->shark.bitmap->bounds.size };
  
  layer_set_frame((Layer*) data->shark.layer, startFrame);    
  layer_set_bounds((Layer*) data->shark.layer, GRect(0, 0, startFrame.size.w, startFrame.size.h));
  
  // Create the animation and schedule it.
  _animation = property_animation_create_layer_frame((Layer*) data->shark.layer, NULL, &stopFrame);

  if (_animation != NULL) {
    animation_set_duration((Animation*) _animation, sharkAnimation->duration);
    animation_set_curve((Animation*) _animation, AnimationCurveLinear);
    animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
      .started = NULL,
      .stopped = (AnimationStoppedHandler) animationStoppedHandler,
    }, (void*) data);
    _animationType = sharkAnimation->type;  
    animation_schedule((Animation*) _animation);
  }
}

static SharkAnimation* getSharkAnimation(uint16_t minute, uint16_t second) {
  for (int index = 0; index < SHARK_PASS_COUNT; index++) {
    if (minute == _sharkPass[index].startMinute) {
      return &_sharkPass[index];
    }
  }
  
  // Limit animation to between startMinute and ANIMATION_CUTOFF_SECOND seconds,
  // currently 53:00 to 53:44.
  if (minute == _sharkEat[0].startMinute && second < ANIMATION_CUTOFF_SECOND) {
    return &_sharkEat[0];
  }
  
  return NULL;
}

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
  
  if (finished && _animationType == SHARK_EAT && (_eatAnimationIndex + 1) < SHARK_EAT_COUNT) {
    // Work-around to crash in the Pebble animation_schedule() function in runAnimation().
    // Schedule a timer to run the next animation.
    _eatAnimationIndex++;
    _eatTimer = app_timer_register(10, (AppTimerCallback) eatTimerCallback, context);
  }
}

static void eatTimerCallback(void *callback_data) {
  _eatTimer = NULL;
  SharkLayerData* data = (SharkLayerData*) callback_data;

  // The shark layer is responsible for showing/hiding the duck layer in minute SHARK_SCENE_EAT_MINUTE.
  SetLayerHidden((Layer*) data->duckData->duck.layer, &data->duckData->hidden, 
                 (_eatAnimationIndex >= EAT_ANIMATION_HIDE_DUCK_INDEX));
                 
  runAnimation(data, &_sharkEat[_eatAnimationIndex]);
}