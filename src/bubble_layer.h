#pragma once
#include "common.h"
  
typedef struct {
  Layer* layer;
} BubbleLayerData;

BubbleLayerData* CreateBubbleLayer(Layer* relativeLayer, LayerRelation relation);
void DrawBubbleLayer(BubbleLayerData* data, uint16_t hour, uint16_t minute);
void DestroyBubbleLayer(BubbleLayerData* data);