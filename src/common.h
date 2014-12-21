#pragma once

//#define RUN_TEST true

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#ifdef RUN_TEST
#define SHARK_ANIMATION_DURATION 2000
#define SANTA_ANIMATION_DURATION 2000
#else
#define SHARK_ANIMATION_DURATION 8000
#define SANTA_ANIMATION_DURATION 10000
#endif

#define WAVE_HEIGHT 4
#define WAVE_COUNT 4
#define WATER_RISE_DURATION 500
#define SHARK_SCENE_EAT_MINUTE 51

// The last minute in the hour that Santa will fly. Otherwise he gets 
// too close to the duck.
#define LAST_SANTA_ANIMATION_MINUTE 30
  
// Delay animations to give the watchface animation to finish when it is first launched.
#define FIRST_DISPLAY_ANIMATION_DELAY 500
#define PEBBLE_ANGLE_PER_DEGREE (TRIG_MAX_ANGLE / 360)

// Convert degree to Pebble angle
#define PEBBLE_ANGLE_FROM_DEGREE(degree) (degree * PEBBLE_ANGLE_PER_DEGREE)

// Convert from minute to Y coordinate
#define WATER_TOP(minute) (SCREEN_HEIGHT - (minute * 14 / 5))

typedef enum { CHILD, ABOVE_SIBLING, BELOW_SIBLING } LayerRelation;
typedef enum { UNDEFINED_SCENE, CHRISTMAS, DUCK, FRIDAY13, THANKSGIVING } SCENE;

typedef struct {
  BitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
} BitmapGroup;

typedef struct {
  RotBitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
  int32_t angle;        // Angle in degrees
} RotBitmapGroup;

void AddLayer(Layer *relativeLayer, Layer *newLayer, LayerRelation relation);
void SetLayerHidden(Layer *layer, bool *currentHidden, bool newHidden);
bool BitmapGroupSetBitmap(BitmapGroup *group, uint32_t imageResourceId);
GRect RotBitmapGroupChangeBitmap(RotBitmapGroup *group, uint32_t imageResourceId);
void DestroyBitmapGroup(BitmapGroup *group);
void DestroyRotBitmapGroup(RotBitmapGroup *group);