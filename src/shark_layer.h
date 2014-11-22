#pragma once
#include "common.h"
  
typedef struct {
  BitmapGroup shark;
  bool hidden;
} SharkLayerData;

SharkLayerData* CreateSharkLayer(Layer* relativeLayer, LayerRelation relation);
void DrawSharkLayer(SharkLayerData* data, uint16_t hour, uint16_t minute);
void DestroySharkLayer(SharkLayerData* data);