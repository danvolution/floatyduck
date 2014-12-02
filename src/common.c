#include <pebble.h>
#include "common.h"

static uint16_t getImageHypotenuse(uint32_t imageResourceId);

void AddLayer(Layer *relativeLayer, Layer *newLayer, LayerRelation relation) {
  switch (relation) {
    case ABOVE_SIBLING:
      layer_insert_above_sibling(newLayer, relativeLayer);
    break;
    
    case BELOW_SIBLING:
      layer_insert_below_sibling(newLayer, relativeLayer);
      break;
    
    default:
      layer_add_child(relativeLayer, newLayer);      
      break;
  }
}

void SetLayerHidden(Layer *layer, bool *currentHidden, bool newHidden) {
  if (*currentHidden != newHidden) {
    layer_set_hidden(layer, newHidden);
    *currentHidden = newHidden;   
  }
}

// Returns whether the bitmap was changed.
bool BitmapGroupSetBitmap(BitmapGroup *group, uint32_t imageResourceId) {
  bool imageChanged = false;
  
  if (group->resourceId != imageResourceId) {
    imageChanged = true;
    
    if (group->bitmap != NULL) {
      gbitmap_destroy(group->bitmap);
      group->bitmap = NULL;
      group->resourceId = 0;
    }
    
    group->bitmap = gbitmap_create_with_resource(imageResourceId);
    group->resourceId = imageResourceId;
    bitmap_layer_set_bitmap((BitmapLayer*) group->layer, group->bitmap);
  }
  
  return imageChanged;
}

// Returns new GRect frame for the RotBitmapLayer. Frame and bounds will be adjusted to
// new image, however the frame will most likely not be in the right position.
GRect RotBitmapGroupChangeBitmap(RotBitmapGroup *group, uint32_t imageResourceId) {
  if (group->bitmap != NULL) {
    gbitmap_destroy(group->bitmap);
    group->bitmap = NULL;
    group->resourceId = 0;
  }

  // Create and set the new bitmap on the RotBitmapLayer
  group->bitmap = gbitmap_create_with_resource(imageResourceId);
  group->resourceId = imageResourceId;
  bitmap_layer_set_bitmap((BitmapLayer*) group->layer, group->bitmap);

  // Get the hypotenuse of the new image which is what the RotBitmapLayer uses
  // for the frame width & height.
  uint16_t hypotenuse = getImageHypotenuse(imageResourceId);

  // Adjust the frame size
  GRect rotFrame = layer_get_frame((Layer*) group->layer);   
  rotFrame.size.w = hypotenuse;
  rotFrame.size.h = hypotenuse;
  layer_set_frame((Layer*) group->layer, rotFrame);    

  // Adjust the bounds
  layer_set_bounds((Layer*) group->layer, GRect(0, 0, hypotenuse, hypotenuse));

  // Set the center point for the new bitmap. This causes the RotBitmapLayer to
  // center the new bitmap.
  rot_bitmap_set_src_ic(group->layer, grect_center_point(&group->bitmap->bounds));
  
  return rotFrame;
}

void DestroyBitmapGroup(BitmapGroup *group) {
  if (group != NULL) {
    if (group->bitmap != NULL) {
      gbitmap_destroy(group->bitmap);
      group->bitmap = NULL;
    }
    
    group->resourceId = 0;
    
    if (group->layer != NULL) {
      layer_remove_from_parent((Layer*) group->layer);
      bitmap_layer_destroy(group->layer);
      group->layer = NULL;
    }
  }
}

void DestroyRotBitmapGroup(RotBitmapGroup *group) {
  if (group != NULL) {
    if (group->bitmap != NULL) {
      gbitmap_destroy(group->bitmap);
      group->bitmap = NULL;
    }
    
    group->resourceId = 0;
    
    if (group->layer != NULL) {
      layer_remove_from_parent((Layer*) group->layer);
      rot_bitmap_layer_destroy(group->layer);
      group->layer = NULL;
    }
  }
}

static uint16_t getImageHypotenuse(uint32_t imageResourceId) {
  uint16_t hypotenuse = 0;
  
  switch (imageResourceId) {
    case RESOURCE_ID_IMAGE_DUCK:
    case RESOURCE_ID_IMAGE_DUCK_LEFT:
      hypotenuse = 36;
      break;
    
    case RESOURCE_ID_IMAGE_DUCK_DIVE:
      hypotenuse = 41;
      break;
    
    case RESOURCE_ID_IMAGE_TURKEY:
    case RESOURCE_ID_IMAGE_TURKEY_LEFT:
      hypotenuse = 42;
      break;
    
    default:
      break;
  }
  
  return hypotenuse;
}