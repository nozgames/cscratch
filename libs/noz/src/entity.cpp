//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct EntityImpl
{
    OBJECT_BASE;
    vec3 local_position = vec3(0.0f);
    vec3 local_scale = vec3(1.0f);
    quat local_rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
    mat4 local_to_world = glm::identity<mat4>();
    mat4 world_to_local = glm::identity<mat4>();
    bool local_to_world_dirty = true;
    bool world_to_local_dirty = true;
    bool enabled = true;
    uint32_t version = 1;

    // todo
    // vector<entity> children;
    // linked_list<entity_impl>::node render_node{ this };
    // const entity_traits* traits = nullptr;
};

static_assert(ENTITY_BASE_SIZE == sizeof(EntityImpl));

static EntityImpl* Impl(Entity* e) { return (EntityImpl*)CastToBase(e, TYPE_ENTITY); }

static void MarkDirty(Entity* entity)
{
    EntityImpl* impl = Impl(entity);
    impl->local_to_world_dirty = true;
    impl->world_to_local_dirty = true;
}

Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id)
{
    EntityImpl* impl = Impl((Entity*)CreateObject(allocator, entity_size, type_id, TYPE_ENTITY));
    return (Entity*)impl;
}

vec3 GetPosition(Entity* entity)
{
    return vec3(GetLocalToWorld(entity)[3]);
}

void SetPosition(Entity* e, float x, float y, float z)
{
}

const mat4& GetWorldToLocal(Entity* e)
{
    return Impl(e)->world_to_local;
}

const mat4& GetLocalToWorld(Entity* e)
{
    return Impl(e)->local_to_world;
}
