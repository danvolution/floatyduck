#include <pebble.h>
#include "shark_layer.h"

#define SHARK_PASS_1_START_MINUTE 19
#define SHARK_PASS_1_MINUTE_COUNT 8
#define SHARK_PASS_1_COORDINATE_Y 129
#define SHARK_PASS_2_START_MINUTE 48
#define SHARK_PASS_2_MINUTE_COUNT 12

static GPoint _sharkPass1Position[SHARK_PASS_1_MINUTE_COUNT] = { 
  {-60, SHARK_PASS_1_COORDINATE_Y}, {-34, SHARK_PASS_1_COORDINATE_Y}, 
  {-7, SHARK_PASS_1_COORDINATE_Y}, {19, SHARK_PASS_1_COORDINATE_Y}, 
  {46, SHARK_PASS_1_COORDINATE_Y}, {72, SHARK_PASS_1_COORDINATE_Y}, 
  {99, SHARK_PASS_1_COORDINATE_Y}, {125, SHARK_PASS_1_COORDINATE_Y} 
};

static GPoint _sharkPass2Position[SHARK_PASS_2_MINUTE_COUNT] = { 
  {136, 25}, {118, 15}, {100, 5}, {82, 0}, 
  {55, 0}, {37, 5}, {19, 10}, {1, 15}, 
  {-8, 15}, {-26, 15}, {-44, 15}, {-62, 15}
};

static uint32_t _sharkPass2ResourceId[SHARK_PASS_2_MINUTE_COUNT] = { 
  RESOURCE_ID_IMAGE_SHARK_LEFT, RESOURCE_ID_IMAGE_SHARK_OPEN_1, RESOURCE_ID_IMAGE_SHARK_OPEN_3, RESOURCE_ID_IMAGE_SHARK_OPEN_5,
  RESOURCE_ID_IMAGE_SHARK_EAT_1, RESOURCE_ID_IMAGE_SHARK_EAT_2, RESOURCE_ID_IMAGE_SHARK_EAT_3, RESOURCE_ID_IMAGE_SHARK_EAT_4,
  RESOURCE_ID_IMAGE_SHARK_OPEN_1, RESOURCE_ID_IMAGE_SHARK_LEFT, RESOURCE_ID_IMAGE_SHARK_LEFT, RESOURCE_ID_IMAGE_SHARK_LEFT 
};
  
static uint32_t getSharkResourceId(uint16_t minute);
static GPoint getSharkPosition(uint16_t minute);
static GRect getSharkFrame(uint16_t minute, int16_t imageWidth, int16_t imageHeight);

SharkLayerData* CreateSharkLayer(Layer* relativeLayer, LayerRelation relation) {
  SharkLayerData* data = malloc(sizeof(SharkLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(SharkLayerData));
    data->shark.layer = bitmap_layer_create(GRect(0, -5, 5, 5));
    bitmap_layer_set_compositing_mode(data->shark.layer, GCompOpAnd);
    AddLayer(relativeLayer, (Layer*) data->shark.layer, relation);    
    data->hidden = false;
  }
  
  return data;
}

void DrawSharkLayer(SharkLayerData* data, uint16_t hour, uint16_t minute) {
  uint32_t sharkResourceId = getSharkResourceId(minute);
  
  if (sharkResourceId == 0) {
    if (data->hidden == false) {
      layer_set_hidden((Layer*) data->shark.layer, true);
      data->hidden = true;
    }
    
    return;
  }
  
  if (sharkResourceId != data->shark.resourceId) {
    if (data->shark.bitmap != NULL) {
      gbitmap_destroy(data->shark.bitmap);
      data->shark.bitmap = NULL;
      data->shark.resourceId = 0;
    }
    
    data->shark.bitmap = gbitmap_create_with_resource(sharkResourceId);
    data->shark.resourceId = sharkResourceId;
    bitmap_layer_set_bitmap(data->shark.layer, data->shark.bitmap);
  }

  GRect sharkFrame = getSharkFrame(minute, data->shark.bitmap->bounds.size.w, data->shark.bitmap->bounds.size.h);
  layer_set_frame((Layer*) data->shark.layer, sharkFrame);    
  layer_set_bounds((Layer*) data->shark.layer, GRect(0, 0, sharkFrame.size.w, sharkFrame.size.h));    

  if (data->hidden == true) {
    layer_set_hidden((Layer*) data->shark.layer, false);
    data->hidden = false;
  }
}

void DestroySharkLayer(SharkLayerData* data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->shark);
    free(data);
  }  
}

static uint32_t getSharkResourceId(uint16_t minute) {
  if (minute >= SHARK_PASS_1_START_MINUTE && 
      minute < (SHARK_PASS_1_START_MINUTE + SHARK_PASS_1_MINUTE_COUNT)) {
    
    return RESOURCE_ID_IMAGE_SHARK;
    
  } else if (minute >= SHARK_PASS_2_START_MINUTE && 
             minute < (SHARK_PASS_2_START_MINUTE + SHARK_PASS_2_MINUTE_COUNT)) {
               
    return _sharkPass2ResourceId[minute - SHARK_PASS_2_START_MINUTE];
    
  } else {    
    return 0;
  }  
}

static GPoint getSharkPosition(uint16_t minute) {
  if (minute >= SHARK_PASS_1_START_MINUTE && 
      minute < (SHARK_PASS_1_START_MINUTE + SHARK_PASS_1_MINUTE_COUNT)) {
    
    return _sharkPass1Position[minute - SHARK_PASS_1_START_MINUTE];
    
  } else if (minute >= SHARK_PASS_2_START_MINUTE && 
             minute < (SHARK_PASS_2_START_MINUTE + SHARK_PASS_2_MINUTE_COUNT)) {
               
    return _sharkPass2Position[minute - SHARK_PASS_2_START_MINUTE];
    
  } else {
               
    return (GPoint) { 0, 0 };
  }  
}

static GRect getSharkFrame(uint16_t minute, int16_t imageWidth, int16_t imageHeight) {
  GPoint position = getSharkPosition(minute);
  return (GRect) { .origin = position, .size = { imageWidth, imageHeight} };
}
