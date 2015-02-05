#pragma once
#include "common.h"
#include "bubble_layer.h"
#include "heart_layer.h"
  
typedef struct {
  RotBitmapGroup duck;
  SCENE scene;
  bool hidden;
  int16_t lastUpdateMinute;
  uint16_t flyInReturnMinute;
  uint32_t flyInDelayAnimation;
  bool exited;
  BubbleLayerData *bubbleData;
  HeartLayerData *heartData;
} DuckLayerData;

DuckLayerData* CreateDuckLayer(Layer *relativeLayer, LayerRelation relation, SCENE scene);
void DrawDuckLayer(DuckLayerData *data, uint16_t hour, uint16_t minute, uint16_t second);
void DestroyDuckLayer(DuckLayerData *data);
void SwitchSceneDuckLayer(DuckLayerData *data, SCENE scene);
void HandleTapDuckLayer(DuckLayerData *data, uint16_t hour, uint16_t minute, uint16_t second);