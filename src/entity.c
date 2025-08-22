#include "object.h"
#include "scene.h"

typedef struct entity
{
    vec3 position;
} entity;

static object_type_t g_entity_type = {0};

void entity_init()
{
    g_entity_type = object_type_register("entity");
}

object_type_t entity_type()
{
    return g_entity_type;
}

entity_t entity_create(object_type_t entity_type, size_t entity_size)
{
    object_t o = object_create_with_base(entity_type, entity_size, g_entity_type, sizeof(struct entity));
    entity* impl = (entity*)object_base_impl(o, g_entity_type);
    return (entity_t)o;
}

void entity_position(entity_t e, vec3 position)
{
    entity* impl = (entity*)object_base_impl((object_t)e, g_entity_type);
    glm_vec3_copy(position, impl->position);
}

void entity_set_position(entity_t e, float x, float y, float z)
{
    entity* impl = (entity*)object_base_impl((object_t)e, g_entity_type);
    impl->position[0] = x;
    impl->position[1] = y;
    impl->position[2] = z;
}