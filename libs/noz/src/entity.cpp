//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct entity_impl
{
    OBJECT_BASE;
    vec3 position;
	mat4 local_to_world;
	mat4 world_to_local;
    bool local_to_world_dirty;
    bool world_to_local_dirty;
} entity_impl_t;

static inline entity_impl_t* to_impl(entity_t* e) { return (entity_impl_t*)to_base_object(e, type_entity); }

static inline void entity_mark_dirty(entity_t* entity)
{
	entity_impl_t* impl = to_impl(entity);
    impl->local_to_world_dirty = true;
	impl->world_to_local_dirty = true;
}

entity_t* entity_create(allocator_t* allocator, size_t entity_size, int16_t type_id)
{
	entity_impl_t* impl = to_impl(object_alloc_with_base(allocator, entity_size, type_id, type_entity));
    return (entity_t*)impl;
}

vec3 entity_position(entity_t* e)
{
    return to_impl(e)->position;
}

void entity_set_position(entity_t* e, float x, float y, float z)
{
    entity_impl_t* impl = to_impl(e);
    impl->position = {x, y, z};
    entity_mark_dirty(e);
}

mat4 entity_world_to_local(entity_t* e)
{
	return to_impl(e)->world_to_local;
}

mat4 entity_local_to_world(entity_t* e)
{
    return to_impl(e)->local_to_world;
}

void entity_init()
{
    size_t entity_type_size = sizeof(entity_impl_t);
    if (entity_type_size != ENTITY_BASE_SIZE)
    {
        text_t msg;
        text_init(&msg);
        text_format(&msg, "ENTITY_BASE_SIZE != %zu", entity_type_size);
        application_error(msg.value);
        return;
	}
}

void entity_uninit()
{    
}
