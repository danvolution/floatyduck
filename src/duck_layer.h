#pragma once
#include "common.h"
  
typedef struct {
  BitmapGroup duck;
  SCENE scene;
  bool hidden;
} DuckLayerData;

DuckLayerData* CreateDuckLayer(Layer* relativeLayer, LayerRelation relation, SCENE scene);
void DrawDuckLayer(DuckLayerData* data, uint16_t hour, uint16_t minute);
void DestroyDuckLayer(DuckLayerData* data);
void SwitchDuckScene(DuckLayerData* data, SCENE scene);