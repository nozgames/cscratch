//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Entity : Object { };
struct Camera : Entity { };

#define ENTITY_BASE_SIZE 192
#define ENTITY_BASE EntityBase __entity;

struct EntityBase { u8 __entity[ENTITY_BASE_SIZE]; };

inline void* Cast(Object* obj, type_t type_id)
{
    assert(obj && *((type_t*)obj) == type_id);
    return obj;
}

inline void* CastToBase(Object* obj, type_t base_id)
{
    assert(obj && *(((int16_t*)obj) + 1) == base_id);
    return obj;
}

// @traits
struct EntityTraits
{
    void(*destroy)(const Entity*) = nullptr;
    void(*update)(const Entity*) = nullptr;
    void(*render)(const Entity*, const Camera*) = nullptr;
    void(*on_enabled)(const Entity*) = nullptr;
    void(*on_disabled)(const Entity*) = nullptr;
};


// @scene
void InitScene();
void ShutdownScene();

// @entity
Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id);
vec3 GetPosition(Entity* entity);
const mat4& GetWorldToLocal(Entity* entity);
const mat4& GetLocalToWorld(Entity* entity);

// @camera
Camera* CreateCamera();
const mat4& GetProjection(Camera* camera);
