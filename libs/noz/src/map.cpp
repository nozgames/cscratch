//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr size_t INVALID_KEY_INDEX = 0xFFFFFFFF;

Map CreateMap(u64* keys, size_t capacity, void* data, size_t data_stride, size_t initial_count)
{
    return { .capacity = capacity, .count = initial_count, .keys = keys, .data = data, .data_stride = data_stride };
}

static size_t FindKey(const Map& map, u64 key)
{
    auto map_key = map.keys;
    for (size_t i=0, c=map.count; i<c; i++, map_key++)
        if (*map_key == key)
            return i;

    return INVALID_KEY_INDEX;
}

bool HasKey(const Map& map, u64 key)
{
    return FindKey(map, key) != INVALID_KEY_INDEX;
}

void* GetValue(const Map& map, u64 key)
{
    auto key_index = FindKey(map, key);
    if (key_index == INVALID_KEY_INDEX)
        return nullptr;

    return (u8*)map.data + key_index * map.data_stride;
}

void* GetValue(const Map& map, const char* key)
{
    assert(key);
    return GetValue(map, Hash(key));
}

void* SetValue(Map& map, const char* key, void* value)
{
    return SetValue(map, Hash(key), value);
}

void* SetValue(Map& map, u64 key, void* value)
{
    auto key_index = FindKey(map, key);
    if (key_index == INVALID_KEY_INDEX)
    {
        if (map.count >= map.capacity)
            return nullptr;

        key_index = map.count;
        map.count++;
    }

    auto data = (u8*)map.data + key_index + map.data_stride;
    if (value)
        memcpy(data, value, map.data_stride);
    return data;
}

#if 0
void Enumerate(Map* map, MapEnumeratePredicate callback, void* user_data)
{
    assert(map);
    assert(callback);

    MapImpl* impl = Impl(map);

    for (size_t i = 0; i < impl->capacity; i++)
    {
        MapEntry* entry = &impl->entries[i];
        if (entry->is_occupied && !entry->is_deleted)
        {
            callback(entry->key, entry->value, user_data);
        }
    }
}

#endif