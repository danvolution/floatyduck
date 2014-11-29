#pragma once
#include "common.h"
  
typedef struct {
  Layer* layer;
  InverterLayer* childInverterLayers[WAVE_HEIGHT * WAVE_COUNT];
  int16_t lastUpdateMinute;
} WavesLayerData;

WavesLayerData* CreateWavesLayer(Layer* relativeLayer, LayerRelation relation);
void DrawWavesLayer(WavesLayerData* data, uint16_t hour, uint16_t minute);
void DestroyWavesLayer(WavesLayerData* data);