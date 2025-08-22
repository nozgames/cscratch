//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define MAX_TYPES 256

struct object_type_impl
{
    name_t name;
};

static struct object_type_impl g_types[MAX_TYPES] = {0};
static int g_type_count = 0;

object_type_t object_type_create(const char* name)
{
    assert(g_type_count < MAX_TYPES);

    struct object_type_impl* type = g_types + g_type_count++;
    name_set(&type->name, name);
    return type;
}

const char* object_type_name(object_type_t type)
{
    assert(type);
    return type->name.data;
}
