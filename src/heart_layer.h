#pragma once
#include "common.h"
  
#define MAX_HEARTS 16
  
typedef struct {
  BitmapLayer *bitmapLayer;
  PropertyAnimation *animation;
} Heart;
  
typedef struct {
  Layer *layer;
  GBitmap *bitmap;
  Heart childHearts[MAX_HEARTS];
} HeartLayerData;

HeartLayerData* CreateHeartLayer(Layer* relativeLayer, LayerRelation relation);
void DrawHeartLayer(HeartLayerData* data, uint16_t hour, uint16_t minute);
void DestroyHeartLayer(HeartLayerData* data);
void AddHeart(HeartLayerData* data, GPoint startOrigin, GPoint endOrigin, uint16_t speed, uint16_t delayStart);