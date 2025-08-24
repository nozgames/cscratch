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

static EntityImpl* Impl(Entity* e) { return (EntityImpl*)to_base_object(e, type_entity); }

static void MarkDirty(Entity* entity)
{
    EntityImpl* impl = Impl(entity);
    impl->local_to_world_dirty = true;
    impl->world_to_local_dirty = true;
}

Entity* CreateEntity(Allocator* allocator, size_t entity_size, int16_t type_id)
{
    EntityImpl* impl = Impl((Entity*)Alloc(allocator, entity_size, type_id, type_entity));
    return (Entity*)impl;
}

vec3 entity_position(Entity* entity)
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

mat4 entity_world_to_local(Entity* e)
{
	return Impl(e)->world_to_local;
}

mat4 entity_local_to_world(Entity* e)
{
    return Impl(e)->local_to_world;
}

void InitEntity()
{
    size_t entity_type_size = sizeof(EntityImpl);
    if (entity_type_size != ENTITY_BASE_SIZE)
    {
        text_t msg;
        text_init(&msg);
        text_format(&msg, "ENTITY_BASE_SIZE != %zu", entity_type_size);
        Exit(msg.value);
        return;
    }
}

void ShutdownEntity()
{    
}
