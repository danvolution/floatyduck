#pragma once

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
 
#define WAVE_HEIGHT 4
#define WAVE_COUNT 4

// Convert from minute to Y coordinate
#define WATER_TOP(minute) (SCREEN_HEIGHT - (minute * 14 / 5))

typedef enum { CHILD, ABOVE_SIBLING, BELOW_SIBLING } LayerRelation;
typedef enum { UNDEFINED, DUCK, FRIDAY13, THANKSGIVING } SCENE;

typedef struct {
  BitmapLayer* layer;
  GBitmap* bitmap;
  uint32_t resourceId;
} BitmapGroup;

void AddLayer(Layer* relativeLayer, Layer* newLayer, LayerRelation relation);
void DestroyBitmapGroup(BitmapGroup* group);