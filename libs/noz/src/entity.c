//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct entity_impl
{
    vec3_t position;
} entity_impl_t;

static object_type_t g_entity_type = nullptr;

entity_t entity_create(object_type_t entity_type, size_t entity_size)
{
    object_t o = object_create_with_base(entity_type, entity_size, g_entity_type, sizeof(entity_impl_t));
    entity_impl_t* impl = (entity_impl_t*)object_base_impl(o, g_entity_type);
    return (entity_t)o;
}

vec3_t entity_position(entity_t e)
{
    entity_impl_t* impl = (entity_impl_t*)object_base_impl((object_t)e, g_entity_type);
    return impl->position;
}

void entity_set_position(entity_t e, float x, float y, float z)
{
    entity_impl_t* impl = (entity_impl_t*)object_base_impl((object_t)e, g_entity_type);
    impl->position = (vec3_t){x, y, z};
}

void entity_init()
{
    g_entity_type = object_type_create("entity");
}

void entity_uninit()
{    
    g_entity_type = nullptr;
}