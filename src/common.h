#pragma once

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
 
#define WAVE_HEIGHT 4
#define WAVE_COUNT 4
#define SHARK_SCENE_EAT_MINUTE 51
#define WATER_RISE_DURATION 500
#define PEBBLE_ANGLE_PER_DEGREE (TRIG_MAX_ANGLE / 360)

// Convert from minute to Y coordinate
#define WATER_TOP(minute) (SCREEN_HEIGHT - (minute * 14 / 5))

typedef enum { CHILD, ABOVE_SIBLING, BELOW_SIBLING } LayerRelation;
typedef enum { UNDEFINED, CHRISTMAS, DUCK, FRIDAY13, THANKSGIVING } SCENE;

typedef struct {
  BitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
} BitmapGroup;

typedef struct {
  RotBitmapLayer *layer;
  GBitmap *bitmap;
  uint32_t resourceId;
  int32_t angle;        // Valid values 0 - 0x10000
} RotBitmapGroup;

void AddLayer(Layer *relativeLayer, Layer *newLayer, LayerRelation relation);
void SetLayerHidden(Layer *layer, bool *currentHidden, bool newHidden);
bool BitmapGroupSetBitmap(BitmapGroup *group, uint32_t imageResourceId);
GRect RotBitmapGroupChangeBitmap(RotBitmapGroup *group, uint32_t imageResourceId);
void DestroyBitmapGroup(BitmapGroup *group);
void DestroyRotBitmapGroup(RotBitmapGroup *group);