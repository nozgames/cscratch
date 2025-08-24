//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define MAP_DELETED_KEY UINT64_MAX
#define MAP_LOAD_FACTOR 0.75

struct MapEntry
{
    u64 key;
    void* value;
    bool is_occupied;
    bool is_deleted;
};

struct MapImpl
{
    OBJECT_BASE;
    MapEntry* entries;
    size_t capacity;
    size_t count;
    size_t deleted_count;
};

static MapImpl* Impl(const void* m)
{
    return (MapImpl*)Cast((Object*)m, TYPE_MAP);
}


Map* CreateMap(Allocator* allocator, size_t capacity)
{
    if (capacity == 0)
        capacity = 16;
    capacity = noz::NextPowerOf2(capacity);

    size_t total_size = sizeof(MapImpl) + sizeof(MapEntry) * capacity;
    MapImpl* impl = Impl(CreateObject(allocator, total_size, TYPE_MAP));
    if (!impl)
        return NULL;

    impl->entries = (MapEntry*)(impl + 1);
    impl->capacity = capacity;
    impl->count = 0;
    impl->deleted_count = 0;

    memset(impl->entries, 0, sizeof(MapEntry) * capacity);
    return (Map*)impl;
}

static size_t FindKey(MapEntry* entries, size_t capacity, u64 key)
{
    size_t index = key & (capacity - 1);
    size_t original_index = index;

    do
    {
        if (!entries[index].is_occupied && !entries[index].is_deleted)
        {
            return SIZE_MAX;
        }
        if (entries[index].is_occupied && entries[index].key == key)
        {
            return index;
        }
        index = (index + 1) & (capacity - 1);
    } while (index != original_index);

    return SIZE_MAX;
}

void* GetValue(Map* map, u64 key)
{
    assert(map);
    MapImpl* impl = Impl(map);

    if (key == MAP_DELETED_KEY)
        return NULL;

    size_t index = FindKey(impl->entries, impl->capacity, key);
    if (index == SIZE_MAX)
    {
        return NULL;
    }

    return impl->entries[index].value;
}

void* GetValue(Map* map, const char* key)
{
    assert(key);
    return GetValue(map, Hash(key));
}

static void ResizeMap(Map* map)
{
    assert(map);
    MapImpl* impl = Impl(map);

    auto old_capacity = impl->capacity;
    auto* old_entries = impl->entries;

    auto new_capacity = old_capacity * 2;
    // size_t new_total_size = sizeof(MapImpl) + sizeof(map_entry_t) * new_capacity;

    auto* new_entries = (MapEntry*)malloc(sizeof(MapEntry) * new_capacity);
    if (!new_entries)
    {
        return;
    }
    memset(new_entries, 0, sizeof(MapEntry) * new_capacity);

    // size_t old_count = impl->count;
    impl->entries = new_entries;
    impl->capacity = new_capacity;
    impl->count = 0;
    impl->deleted_count = 0;

    for (size_t i = 0; i < old_capacity; i++)
        if (old_entries[i].is_occupied)
            SetValue(map, old_entries[i].key, old_entries[i].value);

    free(old_entries);
}

static size_t FindSlot(MapEntry* entries, size_t capacity, u64 key)
{
    size_t index = key & (capacity - 1);
    size_t original_index = index;

    do
    {
        if (!entries[index].is_occupied || entries[index].key == key)
        {
            return index;
        }
        index = (index + 1) & (capacity - 1);
    } while (index != original_index);

    return SIZE_MAX;
}

void SetValue(Map* map, u64 key, void* value)
{
    assert(map);
    MapImpl* impl = Impl(map);

    if (key == MAP_DELETED_KEY)
    {
        return;
    }

    double load_factor = (double)(impl->count + impl->deleted_count) / impl->capacity;
    if (load_factor > MAP_LOAD_FACTOR)
    {
        ResizeMap(map);
    }

    size_t index = FindSlot(impl->entries, impl->capacity, key);
    assert(index != SIZE_MAX);

    MapEntry* entry = &impl->entries[index];
    if (!entry->is_occupied)
    {
        impl->count++;
        entry->is_occupied = true;
        entry->is_deleted = false;
    }

    entry->key = key;
    entry->value = value;
}

void SetValue(Map* map, const char* key, void* value)
{
    assert(key);
    SetValue(map, Hash(key), value);
}

void Remove(Map* map, u64 key)
{
    assert(map);
    MapImpl* impl = Impl(map);

    if (key == MAP_DELETED_KEY)
    {
        return;
    }

    size_t index = FindKey(impl->entries, impl->capacity, key);
    if (index == SIZE_MAX)
    {
        return;
    }

    MapEntry* entry = &impl->entries[index];
    entry->is_occupied = false;
    entry->is_deleted = true;
    entry->value = NULL;

    impl->count--;
    impl->deleted_count++;
}

void Remove(Map* map, const char* key)
{
    assert(key);
    Remove(map, Hash(key));
}

void Clear(Map* map)
{
    assert(map);
    MapImpl* impl = Impl(map);

    // Clear all entries
    memset(impl->entries, 0, sizeof(MapEntry) * impl->capacity);
    impl->count = 0;
    impl->deleted_count = 0;
}

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
