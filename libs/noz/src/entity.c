//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct entity_impl
{
    vec3_t position;
	mat4_t local_to_world;
	mat4_t world_to_local;
    bool local_to_world_dirty;
    bool world_to_local_dirty;
} entity_impl_t;

static object_type_t g_entity_type = nullptr;

static inline entity_impl_t* to_impl(entity_t entity)
{
    assert(entity);
    return (entity_impl_t*)object_impl((object_t)entity, g_entity_type);
}

static inline void entity_mark_dirty(entity_t entity)
{
    entity_impl_t* impl = to_impl(entity);
    impl->local_to_world_dirty = true;
	impl->world_to_local_dirty = true;
}

entity_t entity_create(object_type_t entity_type, size_t entity_size)
{
    object_t o = object_create_with_base(entity_type, entity_size, g_entity_type, sizeof(entity_impl_t));
    entity_impl_t* impl = (entity_impl_t*)object_base_impl(o, g_entity_type);
    return (entity_t)o;
}

vec3_t entity_position(entity_t e)
{
    // todo: transform
    return to_impl(e)->position;
}

void entity_set_position(entity_t e, float x, float y, float z)
{
    entity_impl_t* impl = to_impl(e);
    impl->position = (vec3_t){x, y, z};
    entity_mark_dirty(e);
}

mat4_t entity_world_to_local(entity_t entity)
{
	entity_impl_t* impl = to_impl(entity);
	return impl->world_to_local;
}

mat4_t entity_local_to_world(entity_t entity)
{
    entity_impl_t* impl = to_impl(entity);
    return impl->local_to_world;
}

void entity_init()
{
    g_entity_type = object_type_create("entity");
}

void entity_uninit()
{    
    g_entity_type = nullptr;
}