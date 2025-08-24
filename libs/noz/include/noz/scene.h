//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct object_impl entity_t;
typedef struct object_impl camera_t;

#define ENTITY_BASE_SIZE 160
#define ENTITY_BASE char __entity[ENTITY_BASE_SIZE]

inline void* to_object(object_t* obj, int16_t type_id)
{
	assert(obj && *((int16_t*)obj) == type_id);
	return (void*)obj;
}

inline void* to_base_object(object_t* obj, int16_t base_id)
{
	assert(obj && *(((int16_t*)obj) + 1) == base_id);
	return (void*)obj;
}

// @scene
void scene_init();
void scene_uninit();

// @entity
entity_t* entity_create(allocator_t* allocator, size_t entity_size, int16_t type_id);
vec3 entity_position(entity_t* entity);
mat4 entity_world_to_local(entity_t* entity);
mat4 entity_local_to_world(entity_t* entity);

// @camera
camera_t* camera_create();
mat4 camera_projection(camera_t* camera);
