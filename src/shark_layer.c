#include <pebble.h>
#include "shark_layer.h"

// RESOURCE_ID_IMAGE_SHARK_LEFT image width
#define SHARK_LEFT_WIDTH 88
  
#define FIRST_SHARK_PASS_MINUTE 20
#define LAST_SHARK_PASS_MINUTE 50
  
// Have the shark pass under the duck by PASS_OFFSET_Y y coordinates.
#define PASS_OFFSET_Y 15
  
// Number of animations for eating the duck
#define SHARK_EAT_COUNT 13
  
// Index in _sharkEat at which the duck layer should be hidden.
#define EAT_ANIMATION_HIDE_DUCK_INDEX 6
  
// Control speed of eat animation. Units are milliseconds per coordinate X.
#define EAT_ANIMATION_SPEED_FACTOR 30
  
// Seconds past the eat animation minute that the animation should not be run.
// Otherwise the animation would spill over to the next minute.
#define ANIMATION_CUTOFF_SECOND 50

#define OFF_SCREEN_LEFT_COORD -998

typedef enum { SHARK_UNDEFINED, SHARK_PASS, SHARK_EAT } SharkAnimationType;

typedef struct {
  SharkAnimationType type;
  uint32_t duration;
  uint32_t delay;
  uint32_t resourceId;
  GPoint startPoint;
  GPoint endPoint;
} SharkAnimation;

static SharkAnimation _sharkEat[SHARK_EAT_COUNT] = {
  { SHARK_EAT, 46 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_LEFT, { SCREEN_WIDTH, 24 }, { 98, 10 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_1, { 98, 10 }, { 93, 8 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_2, { 93, 8 }, { 88, 6 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_3, { 88, 6 }, { 83, 4 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_4, { 83, 4 }, { 78, 2 } },
  { SHARK_EAT, 13 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_5, { 78, 2 }, { 65, 0 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_EAT_1, { 65, 0 }, { 60, 2 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_EAT_2, { 60, 2 }, { 55, 4 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_EAT_3, { 55, 4 }, { 50, 6 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_EAT_4, { 50, 6 }, { 45, 8 } }, 
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_2, { 45, 6 }, { 40, 8 } },
  { SHARK_EAT, 5 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_OPEN_1, { 40, 8 }, { 35, 10 } },
  { SHARK_EAT, 123 * EAT_ANIMATION_SPEED_FACTOR, 0, RESOURCE_ID_IMAGE_SHARK_LEFT, { 35, 10 }, { OFF_SCREEN_LEFT_COORD, 36 } }
};

static PropertyAnimation *_animation = NULL;
static SharkAnimationType _animationType = SHARK_UNDEFINED;
static uint16_t _eatAnimationIndex = 0;
static AppTimer *_eatTimer = NULL;
  
static void runAnimation(SharkLayerData* data, SharkAnimation* sharkAnimation);
static SharkAnimation* getSharkAnimation(uint16_t minute, uint16_t second, bool firstDisplay);
static void animationStoppedHandler(Animation *animation, bool finished, void *context);
static void eatTimerCallback(void *callback_data);
static void resolveCoordinateSubstitution(GPoint *point, uint16_t objectWidth);

SharkLayerData* CreateSharkLayer(Layer *relativeLayer, LayerRelation relation, DuckLayerData *duckData) {
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

void DrawSharkLayer(SharkLayerData *data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
    
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  
  SharkAnimation *sharkAnimation = getSharkAnimation(minute, second, firstDisplay);
  if (sharkAnimation != NULL && _animation == NULL) {
    if (sharkAnimation->type == SHARK_EAT) {
      _eatAnimationIndex = 0;
    }
	
    SetLayerHidden((Layer*) data->shark.layer, &data->hidden, false);
    runAnimation(data, sharkAnimation);
	
  } else if (_animation == NULL) {
    SetLayerHidden((Layer*) data->shark.layer, &data->hidden, true);
  }
  
  // The duck layer is showing if watchface was loaded between 53:50 and 53:59, so hide it if
  // animation isn't running.
  if (minute == SHARK_SCENE_EAT_MINUTE && _animation == NULL && data->duckData->hidden == false) {
      layer_set_hidden((Layer*) data->duckData->duck.layer, true);
      data->duckData->hidden = true;
  }
  
  if (sharkAnimation != NULL) {
    free(sharkAnimation);
  }
}

void DestroySharkLayer(SharkLayerData *data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->shark);
    free(data);
  }  
}

static void runAnimation(SharkLayerData *data, SharkAnimation *sharkAnimation) {
  BitmapGroupSetBitmap(&data->shark, sharkAnimation->resourceId);

  resolveCoordinateSubstitution(&sharkAnimation->startPoint, data->shark.bitmap->bounds.size.w);
  resolveCoordinateSubstitution(&sharkAnimation->endPoint, data->shark.bitmap->bounds.size.w);
  
  GRect startFrame = (GRect) { .origin = sharkAnimation->startPoint, .size = data->shark.bitmap->bounds.size };
  GRect stopFrame = (GRect) { .origin = sharkAnimation->endPoint, .size = data->shark.bitmap->bounds.size };
  
  layer_set_frame((Layer*) data->shark.layer, startFrame);    
  layer_set_bounds((Layer*) data->shark.layer, GRect(0, 0, startFrame.size.w, startFrame.size.h));

  // Create the animation and schedule it.
  _animation = property_animation_create_layer_frame((Layer*) data->shark.layer, NULL, &stopFrame);
  if (_animation != NULL) {
    animation_set_duration((Animation*) _animation, sharkAnimation->duration);
    animation_set_delay((Animation*) _animation, sharkAnimation->delay);
    animation_set_curve((Animation*) _animation, AnimationCurveLinear);
    animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
      .started = NULL,
      .stopped = (AnimationStoppedHandler) animationStoppedHandler,
    }, (void*) data);
    
    _animationType = sharkAnimation->type;
    animation_schedule((Animation*) _animation);
  }
}

static SharkAnimation* getSharkAnimation(uint16_t minute, uint16_t second, bool firstDisplay) {
  // Limit animation to between startMinute and ANIMATION_CUTOFF_SECOND seconds.
  if (minute == SHARK_SCENE_EAT_MINUTE) {
    SharkAnimation *sharkAnimation = NULL;
    if (second < ANIMATION_CUTOFF_SECOND) {
      sharkAnimation = malloc(sizeof(SharkAnimation));
      if (sharkAnimation != NULL) {
        memcpy(sharkAnimation, &_sharkEat[0], sizeof(SharkAnimation));
        sharkAnimation->delay = (firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0);
      }
    }

    return sharkAnimation;
  }
  
  if (minute < FIRST_SHARK_PASS_MINUTE || minute > LAST_SHARK_PASS_MINUTE) {
    return NULL;
  }
  
  // Create animation every 5 minutes or if displaying for the first time.
  if ((minute % 5 != 0) && (firstDisplay == false)) {
    return NULL;
  }
  
  // Don't do animation if the minute before the eat sequence would run over into the eat minute.
  if (minute == (SHARK_SCENE_EAT_MINUTE - 1) && second >= (58 - (SHARK_ANIMATION_DURATION / 1000))) {
    return NULL;
  }
  
  SharkAnimation *sharkAnimation = malloc(sizeof(SharkAnimation));
  if (sharkAnimation == NULL) {
    return NULL;
  }

  bool swimRight = (minute % 2 == 0);
  
  // Position shark PASS_OFFSET_Y below duck
  int16_t coordinateY = WATER_TOP(minute) + PASS_OFFSET_Y;
 
  sharkAnimation->type = SHARK_PASS;
  sharkAnimation->duration = SHARK_ANIMATION_DURATION;
  sharkAnimation->delay = (firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0);
  sharkAnimation->resourceId = swimRight ? RESOURCE_ID_IMAGE_SHARK : RESOURCE_ID_IMAGE_SHARK_LEFT;
  sharkAnimation->startPoint = swimRight ? (GPoint) { OFF_SCREEN_LEFT_COORD, coordinateY } : (GPoint) { SCREEN_WIDTH, coordinateY };
  sharkAnimation->endPoint = swimRight ? (GPoint) { SCREEN_WIDTH, coordinateY } : (GPoint) { OFF_SCREEN_LEFT_COORD, coordinateY };
  
  return sharkAnimation;
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
  
  if (_eatAnimationIndex == EAT_ANIMATION_HIDE_DUCK_INDEX) {
    data->duckData->exited = true;
  }
                 
  runAnimation(data, &_sharkEat[_eatAnimationIndex]);
}

static void resolveCoordinateSubstitution(GPoint *point, uint16_t objectWidth) {
  if (point->x == OFF_SCREEN_LEFT_COORD) {
    point->x = 0 - objectWidth;
  }
}
