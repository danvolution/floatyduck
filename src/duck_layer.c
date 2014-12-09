#include <pebble.h>
#include "duck_layer.h"

#define HORIZONTAL_POSITIONS 15
#define DIVE_POSITIONS 7
#define BEGIN_DIVE_MINUTE 53
#define SHARK_SCENE_HIDE_DUCK_MINUTE 52
#define PREVIOUS_COORD -999
#define OFF_SCREEN_LEFT_COORD -998
#define OFF_SCREEN_RIGHT_COORD -997
  
// Have duck fly in from above by FLY_IN_OFFSET_Y y coordinates.
#define FLY_IN_OFFSET_Y 40

#define DUCK_LANDING_OFFSET_Y 6
#define DUCK_FLY_OUT_OFFSET_Y 8
#define FLY_IN_DURATION 1500
#define FLY_OUT_DURATION 1500
#define FLY_OUT_IN_DELAY 1000
  
// Seconds past the minute that the fly-in animation should not be run.
// Otherwise the animation would spill over to the next minute.
#define FLY_IN_CUTOFF_SECOND 57
  
// Milliseconds between each rotation increment when duck is starting dive.
#define ROTATION_INCREMENT_DURATION 50

typedef enum { DISPLAY_NONE, DISPLAY_ANIMATION, DISPLAY_DIVE, DISPLAY_FLY_IN } DISPLAY_ACTION;

typedef struct {
  int32_t startAngle;  // Start angle in degrees
  int32_t endAngle;    // End angle in degrees
  int32_t increment;   // Degrees to rotate per increment
} RotationAnimation;

typedef struct {
  uint32_t resourceId;
  // startPoint and endPoint are where the bottom center of the bitmap
  // should line up. Allows for varying sizes of bitmaps.
  GPoint startPoint;
  GPoint endPoint;
  uint32_t duration;
  uint32_t delay;
  AnimationCurve animationCurve;
  AnimationStoppedHandler animationStoppedHandler;
  RotationAnimation rotation;
} DuckAnimation;

static int16_t _duckCoordinateX[HORIZONTAL_POSITIONS] = { 17, 25, 32, 40, 48, 56, 63, 71, 79, 86, 94, 102, 110, 117, 125 };

// Pixels to offset the duck in the positive Y direction to make the duck look like
// it is comfortably sitting on the wave.
static int16_t _waveOffsetY[HORIZONTAL_POSITIONS] = { 1, 1, 4, 3, 1, 1, 2, 4, 2, 1, 1, 3, 4, 2, 1 };

static PropertyAnimation *_animation = NULL;
static AppTimer *_rotationTimer = NULL;
static int32_t _rotationAmount = 0;
static int32_t _rotationIncrement = 0;
static AppTimer *_flyInTimer = NULL;
static AppTimer *_flyOutTimer = NULL;

static PropertyAnimation* runAnimation(DuckLayerData *data, DuckAnimation *duckAnimation, uint16_t minute);
static DuckAnimation* getAnimation(uint16_t minute, SCENE scene);
static DuckAnimation* getDiveAnimation(uint16_t minute);
static DuckAnimation* getFlyInAnimation(uint16_t minute);
static DuckAnimation* getFlyOutAnimation(uint16_t minute);
static DISPLAY_ACTION getDisplayAction(DuckLayerData *data, uint16_t minute, uint16_t second, bool firstDisplay);
static bool canDoFlyOut(DuckLayerData *data, uint16_t minute, uint16_t second, int16_t *flyInMinute);
static uint32_t getDuckResourceId(uint16_t minute, SCENE scene);
static bool isAnimationInProgress();
static void disableAnimations(DuckAnimation *duckAnimation);
static GPoint getDuckWavePoint(uint16_t minute);
static uint16_t getHorizontalPosition(uint16_t minute);
static GRect getFrameFromPoint(GPoint bottomCenter, int16_t width, int16_t height);
static bool isMovingRight(uint16_t minute);
static bool isSharkSceneControl(SCENE scene, uint16_t minute);
static void resolveCoordinateSubstitution(GPoint *point, uint16_t objectWidth, uint16_t minute);
static void moveAnimationStopped(Animation *animation, bool finished, void *context);
static void flyInAnimationStopped(Animation *animation, bool finished, void *context);
static void flyOutAnimationStopped(Animation *animation, bool finished, void *context);
static void flyInFinishedTimerCallback(void *callback_data);
static void flyOutFinishedTimerCallback(void *callback_data);
static void rotationTimerCallback(void *callback_data);
static uint32_t calcDegreeDiff(int32_t startDegree, int32_t endDegree, bool rotateCW);

static DuckAnimation _duckDiveAnimation[DIVE_POSITIONS] = {
  { 
    RESOURCE_ID_IMAGE_DUCK_LEFT, { PREVIOUS_COORD, PREVIOUS_COORD }, { 63, 35 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {0, 300, -5}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 59, 39 }, { 56, 55 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {10, 5, -1}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 56, 55 }, { 50, 65 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {5, 0, -1}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 50, 65 }, { 44, 75 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {0, 0, 0}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 44, 75 }, { 38, 100 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {0, 0, 0}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 38, 100 }, { 32, 135 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
   {0, 0, 0}
  },
  { 
    RESOURCE_ID_IMAGE_DUCK_DIVE, { 32, 135 }, { 28, 180 }, WATER_RISE_DURATION, 0, AnimationCurveLinear, moveAnimationStopped, 
    {0, 0, 0}
  }
};

DuckLayerData* CreateDuckLayer(Layer* relativeLayer, LayerRelation relation, SCENE scene) {
  DuckLayerData* data = malloc(sizeof(DuckLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(DuckLayerData));
    data->scene = scene;
    data->duck.bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DUCK);
    data->duck.resourceId = RESOURCE_ID_IMAGE_DUCK;
    data->duck.layer = rot_bitmap_layer_create(data->duck.bitmap);
    rot_bitmap_set_compositing_mode(data->duck.layer, GCompOpAnd);
    AddLayer(relativeLayer, (Layer*) data->duck.layer, relation);
    data->duck.angle = 0;
    data->hidden = false;
    data->lastUpdateMinute = -1;
    data->exited = false;
  }
  
  return data;
}

void DrawDuckLayer(DuckLayerData* data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  
  // Exit if animation or rotation already running
  if (isAnimationInProgress()) {
    return;
  }
  
  // Duck always comes back at minute 0 in shark scene. Reset exited flag.
  if (minute == 0) {
    data->exited = false;
  }
  
  DISPLAY_ACTION displayAction = getDisplayAction(data, minute, second, firstDisplay);
  DuckAnimation *duckAnimation = NULL;
  if (displayAction == DISPLAY_NONE) {
    // Hide layer and exit if not displaying content
    SetLayerHidden((Layer*) data->duck.layer, &data->hidden, true);
    
  } else if (displayAction == DISPLAY_ANIMATION) {
    duckAnimation = getAnimation(minute, data->scene);
    
  } else if (displayAction == DISPLAY_DIVE) {
    duckAnimation = getDiveAnimation(minute);
    
  } else if  (displayAction == DISPLAY_FLY_IN) {
    duckAnimation = getFlyInAnimation(minute);
    duckAnimation->delay = firstDisplay ? FIRST_DISPLAY_ANIMATION_DELAY : 0;
  } 
   
  if (duckAnimation == NULL) {
    return;
  }
  
  // If first display or minute zero and we're not doing the fly-in, don't do animations.
  if ((firstDisplay || minute == 0) && displayAction != DISPLAY_FLY_IN) {
    disableAnimations(duckAnimation);
  }
  
  _animation = runAnimation(data, duckAnimation, minute);
  free(duckAnimation);
  
  // The shark layer controls the duck visibility during the SHARK_SCENE_EAT_MINUTE minute.
  if (isSharkSceneControl(data->scene, minute) == false) {
    SetLayerHidden((Layer*) data->duck.layer, &data->hidden, false);
  }
}

void DestroyDuckLayer(DuckLayerData* data) {
  if (_rotationTimer != NULL) {
    app_timer_cancel(_rotationTimer);
    _rotationTimer = NULL;
  }
  
  if (_flyInTimer != NULL) {
    app_timer_cancel(_flyInTimer);
    _flyInTimer = NULL;
  }
  
  if (data != NULL) {    
    DestroyRotBitmapGroup(&data->duck);
    free(data);
  }
}

void SwitchSceneDuckLayer(DuckLayerData* data, SCENE scene) {
  data->scene = scene;
}

void HandleTapDuckLayer(DuckLayerData *data, uint16_t hour, uint16_t minute, uint16_t second) {
  // Exit if animation or rotation already running
  if (isAnimationInProgress()) {
    return;
  }
  
  int16_t flyInMinute = -1;
  if (canDoFlyOut(data, minute, second, &flyInMinute)) {
    DuckAnimation *duckAnimation = getFlyOutAnimation(minute);
    if (duckAnimation == NULL) {
      return;
    }
    
    // Set whether duck is coming back.
    data->exited = (flyInMinute == -1);
    if (flyInMinute >= 0) {
      data->flyInReturnMinute = flyInMinute;
    }
    
    _animation = runAnimation(data, duckAnimation, minute);
    free(duckAnimation);
  }
}

static PropertyAnimation* runAnimation(DuckLayerData *data, DuckAnimation *duckAnimation, uint16_t minute) {
  PropertyAnimation *animation = NULL;
  GRect rotLayerFrame;
  if (duckAnimation->resourceId != data->duck.resourceId) {
    rotLayerFrame = RotBitmapGroupChangeBitmap(&data->duck, duckAnimation->resourceId);
    
  } else {
    rotLayerFrame = layer_get_frame((Layer*) data->duck.layer);   
  }
  
  resolveCoordinateSubstitution(&duckAnimation->endPoint, data->duck.bitmap->bounds.size.w, minute);
  GRect endFrame = getFrameFromPoint(duckAnimation->endPoint, rotLayerFrame.size.w, rotLayerFrame.size.h);
  
  // Offset the frame by the buffer the RotBitmapLayer creates around the bitmap.
  endFrame.origin.y += ((rotLayerFrame.size.h - data->duck.bitmap->bounds.size.h) / 2);
  
  // No rotation animation, so check that duck is set to end angle
  if (duckAnimation->rotation.increment == 0) {
    if (data->duck.angle != duckAnimation->rotation.endAngle) {
      rot_bitmap_layer_set_angle(data->duck.layer, PEBBLE_ANGLE_FROM_DEGREE(duckAnimation->rotation.endAngle));
      data->duck.angle = duckAnimation->rotation.endAngle;
    }
  } else if (_rotationTimer == NULL) {
    if (data->duck.angle != duckAnimation->rotation.startAngle) {
      rot_bitmap_layer_set_angle(data->duck.layer, PEBBLE_ANGLE_FROM_DEGREE(duckAnimation->rotation.startAngle));
      data->duck.angle = duckAnimation->rotation.startAngle;
    }
    
    _rotationAmount = calcDegreeDiff(duckAnimation->rotation.startAngle, duckAnimation->rotation.endAngle, 
                                     (duckAnimation->rotation.increment > 0));
    
    _rotationIncrement = duckAnimation->rotation.increment;
    _rotationTimer = app_timer_register(ROTATION_INCREMENT_DURATION, (AppTimerCallback) rotationTimerCallback, (void*) data);
  }

  if (duckAnimation->duration == 0) {
    layer_set_frame((Layer*) data->duck.layer, endFrame);

  } else {
    resolveCoordinateSubstitution(&duckAnimation->startPoint, data->duck.bitmap->bounds.size.w, minute);
    GRect startFrame = getFrameFromPoint(duckAnimation->startPoint, rotLayerFrame.size.w, rotLayerFrame.size.h); 
    
    // Offset the frame by the buffer the RotBitmapLayer creates around the bitmap.
    startFrame.origin.y += ((rotLayerFrame.size.h - data->duck.bitmap->bounds.size.h) / 2);
    layer_set_frame((Layer*) data->duck.layer, startFrame);
    
    // Create the animation and schedule it.
    animation = property_animation_create_layer_frame((Layer*) data->duck.layer, NULL, &endFrame);
    animation_set_duration((Animation*) animation, duckAnimation->duration);
    animation_set_curve((Animation*) animation, duckAnimation->animationCurve);
    animation_set_delay((Animation*) animation, duckAnimation->delay);
    animation_set_handlers((Animation*) animation, (AnimationHandlers) {
      .started = NULL,
      .stopped = (AnimationStoppedHandler) duckAnimation->animationStoppedHandler,
    }, (void*) data);

    animation_schedule((Animation*) animation);
  }
  
  return animation;
}

static DuckAnimation* getAnimation(uint16_t minute, SCENE scene) {
  uint32_t duckResourceId = getDuckResourceId(minute, scene);
  if (duckResourceId == 0) {
    return NULL;
  }
  
  DuckAnimation *duckAnimation = malloc(sizeof(DuckAnimation));
  if (duckAnimation == NULL) {
    return NULL;
  }
  
  memset(duckAnimation, 0, sizeof(DuckAnimation));
  duckAnimation->resourceId = duckResourceId;
  duckAnimation->endPoint = getDuckWavePoint(minute);
  if (minute > 0) {
    duckAnimation->startPoint = GPoint(PREVIOUS_COORD, PREVIOUS_COORD);    
    duckAnimation->duration = WATER_RISE_DURATION;
    duckAnimation->animationCurve = AnimationCurveLinear;
    duckAnimation->animationStoppedHandler = moveAnimationStopped;
  }

  return duckAnimation;
}

static DuckAnimation* getDiveAnimation(uint16_t minute) {
  if (minute < BEGIN_DIVE_MINUTE) {
    return NULL;
  }
  
  DuckAnimation *duckAnimation = malloc(sizeof(DuckAnimation));
  if (duckAnimation == NULL) {
    return NULL;
  }
  
  memcpy(duckAnimation, &_duckDiveAnimation[minute - BEGIN_DIVE_MINUTE], sizeof(DuckAnimation));
  return duckAnimation;
}

static DuckAnimation* getFlyInAnimation(uint16_t minute) {
  DuckAnimation *duckAnimation = malloc(sizeof(DuckAnimation));
  if (duckAnimation == NULL) {
    return NULL;
  }
  
  memset(duckAnimation, 0, sizeof(DuckAnimation));
  duckAnimation->endPoint = getDuckWavePoint(minute);
  duckAnimation->startPoint.y = duckAnimation->endPoint.y - FLY_IN_OFFSET_Y;
  // Offset the landing bitmap so feet are in the water
  duckAnimation->endPoint.y += DUCK_LANDING_OFFSET_Y;
        
  if (isMovingRight(minute)) {
    duckAnimation->startPoint.x = OFF_SCREEN_LEFT_COORD;
    duckAnimation->resourceId = RESOURCE_ID_IMAGE_DUCK_LANDING;

  } else {
    duckAnimation->startPoint.x = OFF_SCREEN_RIGHT_COORD;          
    duckAnimation->resourceId = RESOURCE_ID_IMAGE_DUCK_LANDING_LEFT;
  }

  duckAnimation->duration = FLY_IN_DURATION;
  duckAnimation->animationCurve = AnimationCurveEaseOut;
  duckAnimation->animationStoppedHandler = flyInAnimationStopped;
  
  return duckAnimation;
}

static DuckAnimation* getFlyOutAnimation(uint16_t minute) {
  DuckAnimation *duckAnimation = malloc(sizeof(DuckAnimation));
  if (duckAnimation == NULL) {
    return NULL;
  }
  
  memset(duckAnimation, 0, sizeof(DuckAnimation));
  duckAnimation->startPoint = getDuckWavePoint(minute);
  duckAnimation->endPoint.y = duckAnimation->startPoint.y - FLY_IN_OFFSET_Y;
  // Offset the take off bitmap so feet are in the water
  duckAnimation->startPoint.y += DUCK_FLY_OUT_OFFSET_Y;
        
  if (isMovingRight(minute)) {
    duckAnimation->endPoint.x = OFF_SCREEN_RIGHT_COORD;
    duckAnimation->resourceId = RESOURCE_ID_IMAGE_DUCK_TAKING_OFF;

  } else {
    duckAnimation->endPoint.x = OFF_SCREEN_LEFT_COORD;          
    duckAnimation->resourceId = RESOURCE_ID_IMAGE_DUCK_TAKING_OFF_LEFT;
  }

  duckAnimation->duration = FLY_OUT_DURATION;
  duckAnimation->animationCurve = AnimationCurveEaseIn;
  duckAnimation->animationStoppedHandler = flyOutAnimationStopped;
  
  return duckAnimation;
}

static DISPLAY_ACTION getDisplayAction(DuckLayerData *data, uint16_t minute, uint16_t second, bool firstDisplay) {
  // Check if duck already exited (eaten or got away) due to shark or is past the shark eat duck minute.
  if (data->exited || (data->scene == FRIDAY13 && minute >= SHARK_SCENE_HIDE_DUCK_MINUTE)) {
    return DISPLAY_NONE;
  }
  
  if (data->scene == THANKSGIVING) {
    return DISPLAY_ANIMATION;
  }
  
  // Check for duck diving sequence
  if (minute >= BEGIN_DIVE_MINUTE) {
    return DISPLAY_DIVE;
  }
  
  // At first display or minute zero check for fly-in animation
  if (firstDisplay || minute == 0) {
    
    // Do the fly-in animation if the scene criteria are met and it's not too late in the
    // current minute to perform the animation.
    if ((isSharkSceneControl(data->scene, minute) == false) && (second <= FLY_IN_CUTOFF_SECOND)) {
      return DISPLAY_FLY_IN;
    }
  }
  
  return DISPLAY_ANIMATION;
}

static uint32_t getDuckResourceId(uint16_t minute, SCENE scene) {
  uint32_t resourceId = 0;
  
  switch (scene) {
    case FRIDAY13:
      if (minute >= SHARK_SCENE_HIDE_DUCK_MINUTE) {
        resourceId = 0;
        
      } else {
        resourceId = isMovingRight(minute) ? RESOURCE_ID_IMAGE_DUCK : RESOURCE_ID_IMAGE_DUCK_LEFT;
      }

      break;
    
    case THANKSGIVING:
      resourceId = isMovingRight(minute) ? RESOURCE_ID_IMAGE_TURKEY : RESOURCE_ID_IMAGE_TURKEY_LEFT;
      break;
    
    default:
      resourceId = isMovingRight(minute) ? RESOURCE_ID_IMAGE_DUCK : RESOURCE_ID_IMAGE_DUCK_LEFT;
      break;
  }
  
  return resourceId;
}

static bool canDoFlyOut(DuckLayerData *data, uint16_t minute, uint16_t second, int16_t *flyInMinute) {
  *flyInMinute = -1;
  
  // Check if duck already exited (eaten or got away) due to shark.
  if (data->exited) {
    return false;
  }
  
  // Turkeys don't fly (much)
  if (data->scene == THANKSGIVING) {
    return false;
  }
  
  // Calculate landing time
  uint32_t landingMilliSecond = (second * 1000) + FLY_OUT_DURATION + FLY_OUT_IN_DELAY + FLY_IN_DURATION;
   
  if (data->scene == FRIDAY13) {
    // If shark eat duck minute, fly duck out with no fly in.
    if (minute == SHARK_SCENE_EAT_MINUTE) {
      return true;
    }

    // If landing comes in shark eat duck minute, fly duck out with no fly in.
    if (landingMilliSecond >= 60000 && minute == (SHARK_SCENE_EAT_MINUTE - 1)) {
      return true;
    }
  } else if (minute >= BEGIN_DIVE_MINUTE) {
    // No flying while diving
    return false;
  }
  
  // Don't fly out if landing time falls in the dive sequence.
  if (landingMilliSecond >= 60000 && minute == (BEGIN_DIVE_MINUTE - 1)) {
    return false;
  }
  
  // Don't fly out if landing occurs when water is rising. Also covers the case
  // of the hour change when the duck already flies in at minute zero.
  if (landingMilliSecond >= 59000 && landingMilliSecond < 61000) {
    return false;
  }
  
  *flyInMinute = (landingMilliSecond >= 60000) ? (minute + 1) % 60: minute;
  return true;
}

static bool isSharkSceneControl(SCENE scene, uint16_t minute) {
  return (scene == FRIDAY13 && minute == SHARK_SCENE_EAT_MINUTE);
}

static bool isAnimationInProgress() {
  return (_animation != NULL || _rotationTimer != NULL || _flyInTimer != NULL || _flyOutTimer != NULL);
}

static void disableAnimations(DuckAnimation *duckAnimation) {
  duckAnimation->duration = 0;
  duckAnimation->rotation.increment = 0;
}

static GPoint getDuckWavePoint(uint16_t minute) {
  GPoint wavePoint;
  uint16_t horizontalPosition = getHorizontalPosition(minute);
  
  wavePoint.x = _duckCoordinateX[horizontalPosition];
  wavePoint.y = WATER_TOP(minute) - WAVE_HEIGHT + _waveOffsetY[horizontalPosition];
    
  return wavePoint;
}

static GRect getFrameFromPoint(GPoint bottomCenter, int16_t width, int16_t height) {
  return GRect(bottomCenter.x - (width / 2), bottomCenter.y - height, width, height);
}

static void resolveCoordinateSubstitution(GPoint *point, uint16_t objectWidth, uint16_t minute) {
  if (minute > 0 && (point->x == PREVIOUS_COORD || point->y == PREVIOUS_COORD)) {
    GPoint previousPoint = getDuckWavePoint(minute - 1);
    
    if (point->x == PREVIOUS_COORD) {
      point->x = previousPoint.x;
    }

    if (point->y == PREVIOUS_COORD) {
      point->y = previousPoint.y;
    }    
  }

  if (point->x == OFF_SCREEN_LEFT_COORD) {
    point->x = 0 - (objectWidth / 2) - 1;

  } else if (point->x == OFF_SCREEN_RIGHT_COORD) {
    point->x = SCREEN_WIDTH + (objectWidth / 2) + 1;
  }
}

static uint16_t getHorizontalPosition(uint16_t minute) {
  uint16_t position;
  if (isMovingRight(minute)) {
    position = minute % HORIZONTAL_POSITIONS;
    
  } else {
    position = HORIZONTAL_POSITIONS - (minute % HORIZONTAL_POSITIONS) - 1;
  }
  
  return position;
}

static bool isMovingRight(uint16_t minute) {
  if (minute < HORIZONTAL_POSITIONS || (minute >= (HORIZONTAL_POSITIONS * 2) && minute < (HORIZONTAL_POSITIONS * 3))) {
    return true;
  }
  
  return false;
}

static void moveAnimationStopped(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
}

static void flyInAnimationStopped(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
  
  if (finished) {
    _flyInTimer = app_timer_register(10, (AppTimerCallback) flyInFinishedTimerCallback, context);
  }
}

static void flyOutAnimationStopped(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
  
  if (finished) {
    _flyOutTimer = app_timer_register(10, (AppTimerCallback) flyOutFinishedTimerCallback, context);
  }
}

static void flyInFinishedTimerCallback(void *callback_data) {
  _flyInTimer = NULL;
  
  if (isAnimationInProgress()) {
    return;
  }
  
  DuckLayerData *data = (DuckLayerData*) callback_data;
  DuckAnimation *duckAnimation = getAnimation(data->lastUpdateMinute, data->scene);
  if (duckAnimation == NULL) {
    return;
  }

  disableAnimations(duckAnimation);
  _animation = runAnimation(data, duckAnimation, data->lastUpdateMinute);
  free(duckAnimation);
}

static void flyOutFinishedTimerCallback(void *callback_data) {
  _flyOutTimer = NULL;
  
  if (isAnimationInProgress()) {
    return;
  }

  DuckLayerData *data = (DuckLayerData*) callback_data;
  
  // Do not fly in if duck has already exited such as escaping the shark.
  if (data->exited) {
    return;
  }
  
  DuckAnimation *duckAnimation = getFlyInAnimation(data->flyInReturnMinute);
  if (duckAnimation == NULL) {
    return;
  }

  duckAnimation->delay = FLY_OUT_IN_DELAY;
  _animation = runAnimation(data, duckAnimation, data->flyInReturnMinute);
  free(duckAnimation);
}

static void rotationTimerCallback(void *callback_data) {
  _rotationTimer = NULL;
  DuckLayerData *data = (DuckLayerData*) callback_data;
  
  data->duck.angle += _rotationIncrement;
  if (data->duck.angle < 0) {
    data->duck.angle += 360;
    
  } else if (data->duck.angle > 360) {
    data->duck.angle -= 360;    
  }
  
  rot_bitmap_layer_set_angle(data->duck.layer, PEBBLE_ANGLE_FROM_DEGREE(data->duck.angle));
  _rotationAmount -= abs(_rotationIncrement);
  
  if (_rotationAmount > 0) {
    _rotationTimer = app_timer_register(ROTATION_INCREMENT_DURATION, (AppTimerCallback) rotationTimerCallback, (void*) data);
  }
}

static uint32_t calcDegreeDiff(int32_t startDegree, int32_t endDegree, bool rotateCW) {
  uint32_t diff = 0;
  
  if (endDegree > startDegree) {
    diff = rotateCW ? (endDegree - startDegree) : (360 - endDegree + startDegree);
    
  } else {
    diff = rotateCW ? (360 - startDegree + endDegree) : (startDegree - endDegree);
  }
  
  return diff;
}
