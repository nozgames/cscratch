#pragma once

#include "object.h"

typedef struct entity* entity_t;
typedef struct camera* camera_t;

// @scene
void scene_init();
void scene_uninit();

// @entity
object_type_t entity_type();
entity_t entity_create(object_type_t entity_type, size_t entity_size);
void entity_position(entity_t entity, vec3 position);

// @camera
object_type_t camera_type();
camera_t camera_create();
