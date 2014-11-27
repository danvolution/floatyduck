#pragma once
#include "common.h"
#include "duck_layer.h"
  
typedef struct {
  BitmapGroup shark;
  DuckLayerData* duckData;
  bool hidden;
} SharkLayerData;

SharkLayerData* CreateSharkLayer(Layer* relativeLayer, LayerRelation relation, DuckLayerData* duckData);
void DrawSharkLayer(SharkLayerData* data, uint16_t hour, uint16_t minute, uint16_t second);
void DestroySharkLayer(SharkLayerData* data);