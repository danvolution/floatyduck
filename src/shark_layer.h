#pragma once
#include "common.h"
#include "duck_layer.h"
  
typedef struct {
  BitmapGroup shark;
  DuckLayerData* duckData;
  bool hidden;
  int16_t lastUpdateMinute;
} SharkLayerData;

SharkLayerData* CreateSharkLayer(Layer *relativeLayer, LayerRelation relation, DuckLayerData* duckData);
void DrawSharkLayer(SharkLayerData *data, uint16_t hour, uint16_t minute, uint16_t second);
void DestroySharkLayer(SharkLayerData *data);
void HandleTapSharkLayer(SharkLayerData *data, uint16_t hour, uint16_t minute, uint16_t second);