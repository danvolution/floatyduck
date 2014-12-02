#include <pebble.h>
#include "duck_layer.h"

#define HORIZONTAL_POSITIONS 15
#define DIVE_POSITIONS 7
#define BEGIN_DIVE_MINUTE 53
#define SHARK_SCENE_HIDE_DUCK_MINUTE 52
// Milliseconds between each rotation increment when duck is starting dive.
#define ROTATION_INCREMENT_DURATION 40

typedef struct {
  int32_t startAngle;
  int32_t endAngle;
} RotationAnimation;

static int16_t _duckCoordinateX[HORIZONTAL_POSITIONS] = { 17, 25, 32, 40, 48, 56, 63, 71, 79, 86, 94, 102, 110, 117, 125 };
static int16_t _duckDiveCoordinateX[DIVE_POSITIONS] = { 63, 56, 50, 44, 38, 32, 28 };
static int16_t _duckDiveCoordinateY[DIVE_POSITIONS] = { 35, 55, 65, 75, 100, 135, 180 };

// Pixels to offset the duck in the positive Y direction to make the duck look like
// it is comfortably sitting on the wave.
static int16_t _waveOffsetY[HORIZONTAL_POSITIONS] = { 1, 1, 4, 3, 1, 1, 2, 4, 2, 1, 1, 3, 4, 2, 1 };

static PropertyAnimation *_animation = NULL;
static AppTimer *_rotationTimer = NULL;
static int32_t _targetRotationAngle = 0;

static GRect getDuckFrame(uint16_t minute, int16_t width, int16_t height, SCENE scene);
static int16_t getDuckCoordinateX(uint16_t minute, SCENE scene);
static int16_t getDuckCoordinateY(uint16_t minute, int16_t imageHeight, SCENE scene);
static int16_t getWaveOffsetY(uint16_t minute);
static uint32_t getDuckResourceId(uint16_t minute, SCENE scene);
static uint16_t getHorizontalPosition(uint16_t minute);
static bool isMovingRight(uint16_t minute);
static void animationStoppedHandler(Animation *animation, bool finished, void *context);
static RotationAnimation getRotationAnimation(uint16_t minute, SCENE scene);
static void rotationTimerCallback(void *callback_data);

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
  }
  
  return data;
}

void DrawDuckLayer(DuckLayerData* data, uint16_t hour, uint16_t minute) {
  // Exit if this minute has already been handled.
  if (data->lastUpdateMinute == minute) {
    return;
  }
  
  // Remember whether first time called.
  bool firstDisplay = (data->lastUpdateMinute == -1); 
  data->lastUpdateMinute = minute;
  
  // Get duck resource for this minute. Hide layer and exit if no resource (i.e. zero).
  uint32_t duckResourceId = getDuckResourceId(minute, data->scene);
  if (duckResourceId == 0) {
    SetLayerHidden((Layer*) data->duck.layer, &data->hidden, true);
    return;
  }
  
  GRect rotLayerFrame;
  if (duckResourceId != data->duck.resourceId) {
    rotLayerFrame = RotBitmapGroupChangeBitmap(&data->duck, duckResourceId);
    
    // To be pixel perfect the current frame should be calculated with the new image and
    // passed into property_animation_create_layer_frame() as the from_frame. However, since
    // the images are almost the same size, the small offset created by the recentering of
    // the image on the RotBitmapLayer is imperceptible.
    
  } else {
    rotLayerFrame = layer_get_frame((Layer*) data->duck.layer);   
  }
  
  GRect newFrame = getDuckFrame(minute, rotLayerFrame.size.w, rotLayerFrame.size.h, data->scene);
  
  // Offset the new frame by the buffer the RotBitmapLayer creates around the bitmap.
  newFrame.origin.y += ((rotLayerFrame.size.h - data->duck.bitmap->bounds.size.h) / 2);
  
  RotationAnimation rotation = getRotationAnimation(minute, data->scene);
  if (data->duck.angle != rotation.startAngle) {
    rot_bitmap_layer_set_angle(data->duck.layer, rotation.startAngle);
    data->duck.angle = rotation.startAngle;
  }
  
  if (minute == 0 || firstDisplay) {
    layer_set_frame((Layer*) data->duck.layer, newFrame);

  } else if (_animation == NULL) {
    // Create the animation and schedule it.
    _animation = property_animation_create_layer_frame((Layer*) data->duck.layer, NULL, &newFrame);
    animation_set_duration((Animation*) _animation, WATER_RISE_DURATION);
    animation_set_curve((Animation*) _animation, AnimationCurveLinear);
    animation_set_handlers((Animation*) _animation, (AnimationHandlers) {
      .started = NULL,
      .stopped = (AnimationStoppedHandler) animationStoppedHandler,
    }, NULL);

    animation_schedule((Animation*) _animation);
    
    if (rotation.startAngle != rotation.endAngle  && _rotationTimer == NULL) {
      _targetRotationAngle = rotation.endAngle;
      _rotationTimer = app_timer_register(ROTATION_INCREMENT_DURATION, (AppTimerCallback) rotationTimerCallback, (void*) data);
    }
  }
  
  // The shark layer controls the duck visibility during the SHARK_SCENE_EAT_MINUTE minute.
  bool sharkSceneControl = (data->scene == FRIDAY13 && minute == SHARK_SCENE_EAT_MINUTE);
  if (sharkSceneControl == false) {
    SetLayerHidden((Layer*) data->duck.layer, &data->hidden, false);
  }
}

void DestroyDuckLayer(DuckLayerData* data) {
  if (_rotationTimer != NULL) {
    app_timer_cancel(_rotationTimer);
    _rotationTimer = NULL;
  }
  
  if (data != NULL) {    
    DestroyRotBitmapGroup(&data->duck);
    free(data);
  }
}

void SwitchDuckScene(DuckLayerData* data, SCENE scene) {
  data->scene = scene;
}

static int16_t getWaveOffsetY(uint16_t minute) {
  uint16_t position = getHorizontalPosition(minute);
  return _waveOffsetY[position];
}

static int16_t getDuckCoordinateX(uint16_t minute, SCENE scene) {
  int16_t xCoordinate = 0;
  uint16_t position = getHorizontalPosition(minute);
  
  switch (scene) {
    case FRIDAY13:
    case THANKSGIVING:
      xCoordinate = _duckCoordinateX[position];
      break;
    
    default:
      if (minute < BEGIN_DIVE_MINUTE) {
        xCoordinate = _duckCoordinateX[position];
        
      } else {
        xCoordinate = _duckDiveCoordinateX[minute - BEGIN_DIVE_MINUTE];
      }
    
      break;
  }
      
  return xCoordinate;
}

static int16_t getDuckCoordinateY(uint16_t minute, int16_t imageHeight, SCENE scene) {
  int16_t yCoordinate = 0;
  int16_t yWaveOffset = getWaveOffsetY(minute);
  
  switch (scene) {
    case FRIDAY13:
    case THANKSGIVING:
      yCoordinate = WATER_TOP(minute) - WAVE_HEIGHT - imageHeight + yWaveOffset;
      break;
    
    default:
      if (minute < BEGIN_DIVE_MINUTE) {
        yCoordinate = WATER_TOP(minute) - WAVE_HEIGHT - imageHeight + yWaveOffset;
        
      } else {
        yCoordinate = _duckDiveCoordinateY[minute - BEGIN_DIVE_MINUTE] - imageHeight;
      }

    break;
  }
    
  return yCoordinate;
}

static GRect getDuckFrame(uint16_t minute, int16_t imageWidth, int16_t imageHeight, SCENE scene) {
  int16_t xCoordinate = getDuckCoordinateX(minute, scene);
  int16_t yCoordinate = getDuckCoordinateY(minute, imageHeight, scene);
  return GRect(xCoordinate - (imageWidth / 2), yCoordinate, imageWidth, imageHeight);
}

static uint32_t getDuckResourceId(uint16_t minute, SCENE scene) {
  uint32_t resourceId = 0;
  
  switch (scene) {
    case FRIDAY13:
      if (minute >= SHARK_SCENE_HIDE_DUCK_MINUTE) {
        resourceId = 0;
        
      } else if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_DUCK;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_DUCK_LEFT;
      }

      break;
    
    case THANKSGIVING:
      if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_TURKEY;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_TURKEY_LEFT;
      }
      
      break;
    
    default:
      if (minute > BEGIN_DIVE_MINUTE) {
        // Note: The first minute of the dive sequence is the DUCK_LEFT image that rotates.
        resourceId = RESOURCE_ID_IMAGE_DUCK_DIVE;
        
      } else if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_DUCK;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_DUCK_LEFT;
      }
      
      break;
  }
  
  return resourceId;
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

static void animationStoppedHandler(Animation *animation, bool finished, void *context) {
  property_animation_destroy(_animation);
  _animation = NULL;
}

static RotationAnimation getRotationAnimation(uint16_t minute, SCENE scene) {
  RotationAnimation rotate = { 0, 0 };
  
  switch (scene) {
    case DUCK:
    case CHRISTMAS:
      if (minute == BEGIN_DIVE_MINUTE) {
        rotate.startAngle = 0;
        rotate.endAngle = 360 - (PEBBLE_ANGLE_PER_DEGREE * 60);
      }

      break;
    
    default:
      break;
  }
  
  return rotate;
}

static void rotationTimerCallback(void *callback_data) {
  _rotationTimer = NULL;
  DuckLayerData *data = (DuckLayerData*) callback_data;
  
  data->duck.angle -= (PEBBLE_ANGLE_PER_DEGREE * 5);
  rot_bitmap_layer_set_angle(data->duck.layer, data->duck.angle);
  if (data->duck.angle > _targetRotationAngle) {
    _rotationTimer = app_timer_register(ROTATION_INCREMENT_DURATION, (AppTimerCallback) rotationTimerCallback, (void*) data);
  }
}