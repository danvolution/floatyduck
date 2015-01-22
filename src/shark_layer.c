#include <pebble.h>
#include "shark_layer.h"

#define FIRST_SHARK_PASS_MINUTE 20

#define SHARK_LEFT_WIDTH 88
  
// Have the shark pass under the duck by PASS_OFFSET_Y y coordinates.
#define PASS_OFFSET_Y 15
#define PASS_POST_EAT_OFFSET_Y -7
  
// Control speed of eat animation. Units are milliseconds per coordinate X.
#define EAT_ANIMATION_SPEED_FACTOR 30

#define OFF_SCREEN_LEFT_COORD -998

typedef enum { SHARK_UNDEFINED, SHARK_PASS, SHARK_EAT } SharkAnimationType;

typedef enum { EAT_INITIAL, EAT_LEFT, EAT_OPEN_1, EAT_OPEN_2, EAT_OPEN_3, EAT_OPEN_4, EAT_OPEN_5, 
               EAT_1, EAT_2, EAT_3, EAT_4, EAT_FINISHED } EatState;

typedef struct {
  EatState state;
  GPoint endPoint;
  bool up;   // Moving up
} EatStateMachine;

typedef struct {
  SharkAnimationType type;
  uint32_t duration;
  uint32_t delay;
  uint32_t resourceId;
  GPoint startPoint;
  GPoint endPoint;
} SharkAnimation;

/*
static const uint32_t _jawsMusic[] = { 
  500,   50,   250,   2200,   
  500,   50,   250,   1200,   
  500,   50,   250,   250,   
  500,   50,   250,   250,   
  175,   75,   175,   75,   
  175,   75,   175,   75,
  175,   75,   175,   75,   
  175,   75,   175
};
*/

static PropertyAnimation *_animation = NULL;
static SharkAnimationType _animationType = SHARK_UNDEFINED;
static AppTimer *_eatTimer = NULL;
static EatStateMachine _eatState;

static void runAnimation(SharkLayerData* data, SharkAnimation* sharkAnimation);
static SharkAnimation* getSharkAnimation(SharkLayerData *data, uint16_t minute, uint16_t second, bool runNow, bool firstDisplay);
static SharkAnimation* getEatAnimation(SharkLayerData *data);
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
  
  SharkAnimation *sharkAnimation = getSharkAnimation(data, minute, second, firstDisplay, firstDisplay);
  if (sharkAnimation != NULL && _animation == NULL) {
    runAnimation(data, sharkAnimation);
  }
  
  // The duck layer is showing if watchface was loaded between 51:50 and 51:59, so hide it if
  // animation isn't running.
  if (minute == SHARK_SCENE_EAT_MINUTE && _animation == NULL && data->duckData->hidden == false) {
    SetLayerHidden((Layer*) data->duckData->duck.layer, &data->duckData->hidden, true);
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

void HandleTapSharkLayer(SharkLayerData *data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if animation or rotation already running
  if (_animation != NULL) {
    return;
  }
  
  // Force animation on shake.
  SharkAnimation *sharkAnimation = getSharkAnimation(data, minute, second, true, false);
  if (sharkAnimation == NULL) {
    return;
  }

  runAnimation(data, sharkAnimation);
  free(sharkAnimation);
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

static SharkAnimation* getSharkAnimation(SharkLayerData *data, uint16_t minute, uint16_t second, bool runNow, bool firstDisplay) {
  if (minute == SHARK_SCENE_EAT_MINUTE) {
    SharkAnimation *sharkAnimation = NULL;
    
    // Don't let eat animation run over into next minute.
    if (second < (59 - ((SCREEN_WIDTH + SHARK_LEFT_WIDTH) * EAT_ANIMATION_SPEED_FACTOR / 1000))) {
      memset(&_eatState, 0, sizeof(EatStateMachine));
      _eatState.state = EAT_INITIAL;
      
      sharkAnimation = getEatAnimation(data);
      if (sharkAnimation != NULL) {
        sharkAnimation->delay = (firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0);
      }
    }

    return sharkAnimation;
  }
  
  if (minute < FIRST_SHARK_PASS_MINUTE) {
    return NULL;
  }
  
  // Create animation every 5 minutes, if displaying for the first time, or on shake.
  if ((minute % 5 != 0) && (runNow == false)) {
    return NULL;
  }
  
  // Don't do animation if it would run over into the eat minute.
  if (minute == (SHARK_SCENE_EAT_MINUTE - 1) && second >= (58 - (SHARK_ANIMATION_DURATION / 1000))) {
    return NULL;
  }
  
  // Don't do animation if it would run over into the next hour.
  if (minute == 59 && second >= (58 - (SHARK_ANIMATION_DURATION / 1000))) {
    return NULL;
  }
  
  SharkAnimation *sharkAnimation = malloc(sizeof(SharkAnimation));
  if (sharkAnimation == NULL) {
    return NULL;
  }

  bool swimRight = (minute % 2 == 0);
  
  // Position shark PASS_OFFSET_Y below duck
  int16_t coordinateY = WATER_TOP(minute) + ((minute > SHARK_SCENE_EAT_MINUTE) ? PASS_POST_EAT_OFFSET_Y : PASS_OFFSET_Y);
 
  sharkAnimation->type = SHARK_PASS;
  sharkAnimation->duration = SHARK_ANIMATION_DURATION;
  sharkAnimation->delay = (firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0);
  sharkAnimation->resourceId = swimRight ? RESOURCE_ID_IMAGE_SHARK : RESOURCE_ID_IMAGE_SHARK_LEFT;
  sharkAnimation->startPoint = swimRight ? (GPoint) { OFF_SCREEN_LEFT_COORD, coordinateY } : (GPoint) { SCREEN_WIDTH, coordinateY };
  sharkAnimation->endPoint = swimRight ? (GPoint) { SCREEN_WIDTH, coordinateY } : (GPoint) { OFF_SCREEN_LEFT_COORD, coordinateY };
  
  return sharkAnimation;
}

static SharkAnimation* getEatAnimation(SharkLayerData *data) {
  SharkAnimation *sharkAnimation = malloc(sizeof(SharkAnimation));
  if (sharkAnimation == NULL) {
    return NULL;
  }
  
  memset(sharkAnimation, 0, sizeof(SharkAnimation));
  EatStateMachine newState;
  memset(&newState, 0, sizeof(EatStateMachine));
  
  uint32_t resourceId = 0;
  switch (_eatState.state) {
    case EAT_INITIAL:
      newState.up = (data->duckData->exited == false);
      if (newState.up) {
        newState.endPoint = GPoint(98, 10);
        
      } else {
        // Duck has already flown away. Swim the whole width of the screen.
        newState.endPoint = GPoint(0 - SHARK_LEFT_WIDTH, 36);
      }
    
      newState.state = EAT_LEFT;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (SCREEN_WIDTH - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                           RESOURCE_ID_IMAGE_SHARK_LEFT, { SCREEN_WIDTH, 24 }, newState.endPoint };
    
      break;
    
    case EAT_LEFT:
      if (_eatState.up) {
        newState.up = (data->duckData->exited == false);
        if (newState.up) {
          newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y - 2);
          
        } else {
          newState.endPoint = GPoint(0 - SHARK_LEFT_WIDTH, 36);
        }

        resourceId = newState.up ? RESOURCE_ID_IMAGE_SHARK_OPEN_1 : RESOURCE_ID_IMAGE_SHARK_LEFT;
        newState.state = newState.up ? EAT_OPEN_1 : EAT_LEFT;
        *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                             resourceId, _eatState.endPoint, newState.endPoint };
        
      } else {
        newState.state = EAT_FINISHED;
      }
    
      break;
    
    case EAT_OPEN_1:
      newState.up = (data->duckData->exited == false) && _eatState.up;
      if (newState.up) {
        newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y - 2);

      } else {
        newState.endPoint = GPoint(0 - SHARK_LEFT_WIDTH, 36);
      }
    
      resourceId = newState.up ? RESOURCE_ID_IMAGE_SHARK_OPEN_2 : RESOURCE_ID_IMAGE_SHARK_LEFT;
      newState.state = newState.up ? EAT_OPEN_2 : EAT_LEFT;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          resourceId, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_OPEN_2:
      newState.up = (data->duckData->exited == false) && _eatState.up;
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + (newState.up ? -2 : 2));
      resourceId = newState.up ? RESOURCE_ID_IMAGE_SHARK_OPEN_3 : RESOURCE_ID_IMAGE_SHARK_OPEN_1;
      newState.state = newState.up ? EAT_OPEN_3 : EAT_OPEN_1;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          resourceId, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_OPEN_3:
      newState.up = (data->duckData->exited == false) && _eatState.up;
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + (newState.up ? -2 : 2));
      resourceId = newState.up ? RESOURCE_ID_IMAGE_SHARK_OPEN_4 : RESOURCE_ID_IMAGE_SHARK_OPEN_2;
      newState.state = newState.up ? EAT_OPEN_4 : EAT_OPEN_2;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          resourceId, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_OPEN_4:
      newState.up = (data->duckData->exited == false) && _eatState.up;
      if (newState.up) {
        newState.endPoint = GPoint(_eatState.endPoint.x - 13, _eatState.endPoint.y - 2);

      } else {
        newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      }
    
      resourceId = newState.up ? RESOURCE_ID_IMAGE_SHARK_OPEN_5 : RESOURCE_ID_IMAGE_SHARK_OPEN_3;
      newState.state = newState.up ? EAT_OPEN_5 : EAT_OPEN_3;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          resourceId, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_OPEN_5:
      newState.up = false;
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      resourceId = data->duckData->exited ? RESOURCE_ID_IMAGE_SHARK_OPEN_4 : RESOURCE_ID_IMAGE_SHARK_EAT_1;
      newState.state = data->duckData->exited ? EAT_OPEN_4 : EAT_1;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          resourceId, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_1:
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      newState.state = EAT_2;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          RESOURCE_ID_IMAGE_SHARK_EAT_2, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_2:
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      newState.state = EAT_3;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          RESOURCE_ID_IMAGE_SHARK_EAT_3, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_3:
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      newState.state = EAT_4;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          RESOURCE_ID_IMAGE_SHARK_EAT_4, _eatState.endPoint, newState.endPoint };
        
      break;
    
    case EAT_4:
      newState.endPoint = GPoint(_eatState.endPoint.x - 5, _eatState.endPoint.y + 2);
      newState.state = EAT_OPEN_2;
      *sharkAnimation = (SharkAnimation) { SHARK_EAT, (_eatState.endPoint.x - newState.endPoint.x) * EAT_ANIMATION_SPEED_FACTOR, 0, 
                                          RESOURCE_ID_IMAGE_SHARK_OPEN_2, _eatState.endPoint, newState.endPoint };
        
      break;
    
    default:
      newState.state = EAT_FINISHED;
      break;
  }
  
  memcpy(&_eatState, &newState, sizeof(EatStateMachine));
  if (newState.state == EAT_FINISHED && sharkAnimation != NULL) {
    free(sharkAnimation);
    sharkAnimation = NULL;
  }
  
  return sharkAnimation;
}

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
  
  if (finished && _animationType == SHARK_EAT) {
    // Work-around to crash in the Pebble animation_schedule() function in runAnimation().
    // Schedule a timer to run the next animation.
    _eatTimer = app_timer_register(10, (AppTimerCallback) eatTimerCallback, context);
  }
}

static void eatTimerCallback(void *callback_data) {
  _eatTimer = NULL;
  SharkLayerData* data = (SharkLayerData*) callback_data;

  SharkAnimation *animation = getEatAnimation(data);
  
  if (animation == NULL) {
    return;
  }
  
  if (_eatState.state == EAT_1) {
    // The duck has now been eaten.
    data->duckData->exited = true;
    // The shark layer is responsible for showing/hiding the duck layer in minute SHARK_SCENE_EAT_MINUTE.
    SetLayerHidden((Layer*) data->duckData->duck.layer, &data->duckData->hidden, true);
  }
                 
  runAnimation(data, animation);
  free(animation);
}

static void resolveCoordinateSubstitution(GPoint *point, uint16_t objectWidth) {
  if (point->x == OFF_SCREEN_LEFT_COORD) {
    point->x = 0 - objectWidth;
  }
}
