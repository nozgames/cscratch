#pragma once

#include "object.h"

typedef struct entity* entity_t;
typedef struct camera* camera_t;

// @scene
void scene_init();
void scene_uninit();

// @entity
entity_t entity_create(object_type_t entity_type, size_t entity_size);
vec3_t entity_position(entity_t entity);

// @camera
camera_t camera_create();
