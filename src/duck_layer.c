#include <pebble.h>
#include "duck_layer.h"

#define HORIZONTAL_POSITIONS 15
#define DIVE_POSITIONS 7
#define BEGIN_PREPARE_DIVE_MINUTE 53
#define BEGIN_DIVE_MINUTE 56
#define SHARK_SCENE_HIDE_DUCK_MINUTE 52

static int16_t _duckCoordinateX[HORIZONTAL_POSITIONS] = { 17, 25, 32, 40, 48, 56, 63, 71, 79, 86, 94, 102, 110, 117, 125 };
static int16_t _duckDiveCoordinateX[DIVE_POSITIONS] = { 63, 56, 50, 44, 38, 32, 28 };
static int16_t _duckDiveCoordinateY[DIVE_POSITIONS] = { 26, 34, 44, 75, 100, 135, 180 };
static int16_t _waveOffsetY[HORIZONTAL_POSITIONS] = { 1, 1, 4, 3, 1, 1, 2, 4, 2, 1, 1, 3, 4, 2, 1 };
static int16_t _duckDiveResourceId[DIVE_POSITIONS] = { 
  RESOURCE_ID_IMAGE_DUCK_LEFT_15, RESOURCE_ID_IMAGE_DUCK_LEFT_30, RESOURCE_ID_IMAGE_DUCK_LEFT_45, 
  RESOURCE_ID_IMAGE_DUCK_DIVE, RESOURCE_ID_IMAGE_DUCK_DIVE, RESOURCE_ID_IMAGE_DUCK_DIVE, 
  RESOURCE_ID_IMAGE_DUCK_DIVE 
};

static GRect getDuckFrame(uint16_t minute, int16_t width, int16_t height, SCENE scene);
static int16_t getDuckCoordinateX(uint16_t minute, SCENE scene);
static int16_t getDuckCoordinateY(uint16_t minute, int16_t imageHeight, SCENE scene);
static int16_t getWaveOffsetY(uint16_t minute);
static uint32_t getDuckResourceId(uint16_t minute, SCENE scene);
static uint16_t getHorizontalPosition(uint16_t minute);
static bool isMovingRight(uint16_t minute);

DuckLayerData* CreateDuckLayer(Layer* relativeLayer, LayerRelation relation, SCENE scene) {
  DuckLayerData* data = malloc(sizeof(DuckLayerData));
  if (data != NULL) {
    memset(data, 0, sizeof(DuckLayerData));
    data->scene = scene;
    data->duck.layer = bitmap_layer_create(GRect(0, -5, 5, 5));
    bitmap_layer_set_compositing_mode(data->duck.layer, GCompOpAnd);
    AddLayer(relativeLayer, (Layer*) data->duck.layer, relation);
    data->hidden = false;
  }
  
  return data;
}

void DrawDuckLayer(DuckLayerData* data, uint16_t hour, uint16_t minute) {
  uint32_t duckResourceId = getDuckResourceId(minute, data->scene);
  
  if (duckResourceId == 0) {
    if (data->hidden == false) {
      layer_set_hidden((Layer*) data->duck.layer, true);
      data->hidden = true;
    }
    
    return;
  }
  
  if (duckResourceId != data->duck.resourceId) {
    if (data->duck.bitmap != NULL) {
      gbitmap_destroy(data->duck.bitmap);
      data->duck.bitmap = NULL;
      data->duck.resourceId = 0;
    }
    
    data->duck.bitmap = gbitmap_create_with_resource(duckResourceId);
    data->duck.resourceId = duckResourceId;
    bitmap_layer_set_bitmap(data->duck.layer, data->duck.bitmap);
  }
  
  GRect duckFrame = getDuckFrame(minute, data->duck.bitmap->bounds.size.w, data->duck.bitmap->bounds.size.h, data->scene);
  layer_set_frame((Layer*) data->duck.layer, duckFrame);    
  layer_set_bounds((Layer*) data->duck.layer, GRect(0, 0, duckFrame.size.w, duckFrame.size.h));

  // The shark layer controls the duck visibility during the SHARK_SCENE_EAT_MINUTE minute.
  bool sharkSceneControl = (data->scene == FRIDAY13 && minute == SHARK_SCENE_EAT_MINUTE);
  if (data->hidden == true && sharkSceneControl == false) {
    layer_set_hidden((Layer*) data->duck.layer, false);
    data->hidden = false;
  }
}

void DestroyDuckLayer(DuckLayerData* data) {
  if (data != NULL) {    
    DestroyBitmapGroup(&data->duck);
    free(data);
  }
}

void SwitchDuckScene(DuckLayerData* data, SCENE scene) {
  data->scene = scene;
}

static int16_t getWaveOffsetY(uint16_t minute) {
  uint16_t position = getHorizontalPosition(minute);
  return _waveOffsetY[position];
}

static int16_t getDuckCoordinateX(uint16_t minute, SCENE scene) {
  int16_t xCoordinate = 0;
  uint16_t position = getHorizontalPosition(minute);
  
  switch (scene) {
    case FRIDAY13:
    case THANKSGIVING:
      xCoordinate = _duckCoordinateX[position];
      break;
    
    default:
      if (minute < BEGIN_PREPARE_DIVE_MINUTE) {
        xCoordinate = _duckCoordinateX[position];
        
      } else {
        xCoordinate = _duckDiveCoordinateX[minute - BEGIN_PREPARE_DIVE_MINUTE];
      }
    
      break;
  }
      
  return xCoordinate;
}

static int16_t getDuckCoordinateY(uint16_t minute, int16_t imageHeight, SCENE scene) {
  int16_t yCoordinate = 0;
  int16_t yWaveOffset = getWaveOffsetY(minute);
  
  switch (scene) {
    case FRIDAY13:
    case THANKSGIVING:
      yCoordinate = WATER_TOP(minute) - WAVE_HEIGHT - imageHeight + yWaveOffset;
      break;
    
    default:
      if (minute < BEGIN_PREPARE_DIVE_MINUTE) {
        yCoordinate = WATER_TOP(minute) - WAVE_HEIGHT - imageHeight + yWaveOffset;
        
      } else {
        yCoordinate = _duckDiveCoordinateY[minute - BEGIN_PREPARE_DIVE_MINUTE] - imageHeight;
      }

    break;
  }
    
  return yCoordinate;
}

static GRect getDuckFrame(uint16_t minute, int16_t imageWidth, int16_t imageHeight, SCENE scene) {
  int16_t xCoordinate = getDuckCoordinateX(minute, scene);
  int16_t yCoordinate = getDuckCoordinateY(minute, imageHeight, scene);
  return GRect(xCoordinate - (imageWidth / 2), yCoordinate, imageWidth, imageHeight);
}

static uint32_t getDuckResourceId(uint16_t minute, SCENE scene) {
  uint32_t resourceId = 0;
  
  switch (scene) {
    case FRIDAY13:
      if (minute >= SHARK_SCENE_HIDE_DUCK_MINUTE) {
        resourceId = 0;
        
      } else if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_DUCK;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_DUCK_LEFT;
      }

      break;
    
    case THANKSGIVING:
      if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_TURKEY;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_TURKEY_LEFT;
      }
      
      break;
    
    default:
      if (minute >= BEGIN_PREPARE_DIVE_MINUTE) {
        resourceId = _duckDiveResourceId[minute - BEGIN_PREPARE_DIVE_MINUTE];
        
      } else if (isMovingRight(minute)) {
        resourceId = RESOURCE_ID_IMAGE_DUCK;
        
      } else {
        resourceId = RESOURCE_ID_IMAGE_DUCK_LEFT;
      }
      
      break;
  }
  
  return resourceId;
}

static uint16_t getHorizontalPosition(uint16_t minute) {
  uint16_t position;
  if (isMovingRight(minute)) {
    position = minute % HORIZONTAL_POSITIONS;
    
  } else {
    position = HORIZONTAL_POSITIONS - (minute % HORIZONTAL_POSITIONS) - 1;
  }
  
  return position;
}

static bool isMovingRight(uint16_t minute) {
  if (minute < HORIZONTAL_POSITIONS || (minute >= (HORIZONTAL_POSITIONS * 2) && minute < (HORIZONTAL_POSITIONS * 3))) {
    return true;
  }
  
  return false;
}
