#pragma once
#include "common.h"
  
typedef struct {
  RotBitmapGroup duck;
  SCENE scene;
  bool hidden;
  int16_t lastUpdateMinute;
} DuckLayerData;

DuckLayerData* CreateDuckLayer(Layer* relativeLayer, LayerRelation relation, SCENE scene);
void DrawDuckLayer(DuckLayerData* data, uint16_t hour, uint16_t minute);
void DestroyDuckLayer(DuckLayerData* data);
void SwitchDuckScene(DuckLayerData* data, SCENE scene);