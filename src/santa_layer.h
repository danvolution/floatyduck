#pragma once
#include "common.h"
  
typedef struct {
  BitmapGroup santa;
  int16_t lastUpdateMinute;
} SantaLayerData;

SantaLayerData* CreateSantaLayer(Layer *relativeLayer, LayerRelation relation);
void DrawSantaLayer(SantaLayerData *data, uint16_t hour, uint16_t minute);
void DestroySantaLayer(SantaLayerData *data);