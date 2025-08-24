//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Entity : Object { };
struct Camera : Entity { };

#define ENTITY_BASE_SIZE 160
#define ENTITY_BASE char __entity[ENTITY_BASE_SIZE]

inline void* Cast(Object* obj, int16_t type_id)
{
    assert(obj && *((int16_t*)obj) == type_id);
    return obj;
}

inline void* CastToBase(Object* obj, int16_t base_id)
{
    assert(obj && *(((int16_t*)obj) + 1) == base_id);
    return obj;
}

// @scene
void InitScene();
void ShutdownScene();

// @entity
Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id);
const vec3& GetPosition(Entity* entity);
const mat4& GetWorldToLocal(Entity* entity);
const mat4& GetLocalToWorld(Entity* entity);

// @camera
Camera* CreateCamera();
const mat4& GetProjection(Camera* camera);
