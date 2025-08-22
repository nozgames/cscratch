#include <noz/map.h>

typedef struct map_impl
{
	int dummy;
} map_impl_t;

static object_type g_map_type = nullptr;

static inline map_impl_t* to_impl(map_t map)
{
	assert(map);
	return (map_impl_t*)object_impl((object_t)map, g_map_type);
}

map_t map_create(void)
{
	map_t map = (map_t)object_create(g_map_type, sizeof(map_impl_t));
	map_impl_t* impl = to_impl(map);
	return map;
}

void* map_get_string(map_t map, const char* key)
{

}

void* map_get(map_t map, uint64_t key)
{

}
void map_set_string(map_t map, const char* key, void* value)
{

}
void map_set(map_t map, uint64_t key, void* value)
{

}
void map_remove_string(map_t map, const char* key)
{

}
void map_remove(map_t map, uint64_t key)
{

}

void map_init()
{
	assert(!g_map_type);
	g_map_type = object_register_type("map", sizeof(map_impl_t), nullptr, nullptr);
}

void map_uninit()
{
	assert(g_map_type);
}
