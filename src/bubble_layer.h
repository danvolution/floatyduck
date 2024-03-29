#pragma once
#include "common.h"
  
typedef struct {
  Layer* layer;
  int16_t lastUpdateMinute;
  uint16_t nextMinute;
} BubbleLayerData;

BubbleLayerData* CreateBubbleLayer(Layer* relativeLayer, LayerRelation relation);
void DrawBubbleLayer(BubbleLayerData* data, uint16_t hour, uint16_t minute);
void DestroyBubbleLayer(BubbleLayerData* data);
void AddBubble(BubbleLayerData* data, GPoint startOrigin, uint16_t size, uint16_t speed, uint16_t delayStart);