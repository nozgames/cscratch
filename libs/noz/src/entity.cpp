//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct EntityImpl
{
    OBJECT_BASE;
    vec3 position;
    mat4 local_to_world;
    mat4 world_to_local;
    bool local_to_world_dirty;
    bool world_to_local_dirty;
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

const vec3& GetPosition(Entity* entity)
{
    GetAllocator(entity);

    return Impl(entity)->position;
}

void entity_set_position(Entity* e, float x, float y, float z)
{
    EntityImpl* impl = Impl(e);
    impl->position = {x, y, z};
    MarkDirty(e);
}

const mat4& GetWorldToLocal(Entity* e)
{
    return Impl(e)->world_to_local;
}

const mat4& GetLocalToWorld(Entity* e)
{
    return Impl(e)->local_to_world;
}
