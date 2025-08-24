//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define MAP_DELETED_KEY UINT64_MAX
#define MAP_LOAD_FACTOR 0.75

typedef struct map_entry
{
	uint64_t key;
	void* value;
	bool is_occupied;
	bool is_deleted;
} map_entry_t;

typedef struct map_impl
{
	OBJECT_BASE;
	map_entry_t* entries;
	size_t capacity;
	size_t count;
	size_t deleted_count;
} map_impl_t;

static inline map_impl_t* to_impl(const void* m) { return (map_impl_t*)to_object((object_t*)m, type_map); }

static size_t map_next_power_of_2(size_t n)
{
	if (n <= 1) return 2;
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	return n + 1;
}

map_t* map_alloc(allocator_t* allocator, size_t capacity)
{
	if (capacity == 0) capacity = 16;
	capacity = map_next_power_of_2(capacity);
	
	size_t total_size = sizeof(map_impl_t) + sizeof(map_entry_t) * capacity;
	map_impl_t* impl = to_impl(object_alloc(allocator, total_size, type_map));
	if (!impl) return NULL;
	
	impl->entries = (map_entry_t*)(impl + 1);
	impl->capacity = capacity;
	impl->count = 0;
	impl->deleted_count = 0;
	
	memset(impl->entries, 0, sizeof(map_entry_t) * capacity);
	return (map_t*)impl;
}

static size_t map_find_key(map_entry_t* entries, size_t capacity, uint64_t key)
{
	size_t index = key & (capacity - 1);
	size_t original_index = index;
	
	do {
		if (!entries[index].is_occupied && !entries[index].is_deleted) {
			return SIZE_MAX;
		}
		if (entries[index].is_occupied && entries[index].key == key) {
			return index;
		}
		index = (index + 1) & (capacity - 1);
	} while (index != original_index);
	
	return SIZE_MAX;
}

void* map_get(map_t* map, uint64_t key)
{
	assert(map);
	map_impl_t* impl = to_impl(map);
	
	if (key == MAP_DELETED_KEY)
		return NULL;
	
	size_t index = map_find_key(impl->entries, impl->capacity, key);
	if (index == SIZE_MAX) {
		return NULL;
	}
	
	return impl->entries[index].value;
}

void* map_get_string(map_t* map, const char* key)
{
	assert(key);
	return map_get(map, hash_string(key));
}

static void map_resize(map_t* map)
{
	assert(map);
	map_impl_t* impl = to_impl(map);
	
	size_t old_capacity = impl->capacity;
	map_entry_t* old_entries = impl->entries;
	
	size_t new_capacity = old_capacity * 2;
	//size_t new_total_size = sizeof(map_impl_t) + sizeof(map_entry_t) * new_capacity;
	
	map_entry_t* new_entries = (map_entry_t*)malloc(sizeof(map_entry_t) * new_capacity);
	if (!new_entries) {
		return;
	}
	memset(new_entries, 0, sizeof(map_entry_t) * new_capacity);
	
	//size_t old_count = impl->count;
	impl->entries = new_entries;
	impl->capacity = new_capacity;
	impl->count = 0;
	impl->deleted_count = 0;
	
	for (size_t i = 0; i < old_capacity; i++) {
		if (old_entries[i].is_occupied) {
			map_set(map, old_entries[i].key, old_entries[i].value);
		}
	}
	
	free(old_entries);
}

static size_t map_find_slot(map_entry_t* entries, size_t capacity, uint64_t key)
{
	size_t index = key & (capacity - 1);
	size_t original_index = index;
	
	do {
		if (!entries[index].is_occupied || entries[index].key == key) {
			return index;
		}
		index = (index + 1) & (capacity - 1);
	} while (index != original_index);
	
	return SIZE_MAX;
}

void map_set(map_t* map, uint64_t key, void* value)
{
	assert(map);
	map_impl_t* impl = to_impl(map);
	
	if (key == MAP_DELETED_KEY) {
		return;
	}
	
	double load_factor = (double)(impl->count + impl->deleted_count) / impl->capacity;
	if (load_factor > MAP_LOAD_FACTOR) {
		map_resize(map);
	}
	
	size_t index = map_find_slot(impl->entries, impl->capacity, key);
	assert(index != SIZE_MAX);
	
	map_entry_t* entry = &impl->entries[index];
	if (!entry->is_occupied) {
		impl->count++;
		entry->is_occupied = true;
		entry->is_deleted = false;
	}
	
	entry->key = key;
	entry->value = value;
}

void map_set_string(map_t* map, const char* key, void* value)
{
	assert(key);
	map_set(map, hash_string(key), value);
}
void map_remove(map_t* map, uint64_t key)
{
	assert(map);
	map_impl_t* impl = to_impl(map);
	
	if (key == MAP_DELETED_KEY) {
		return;
	}
	
	size_t index = map_find_key(impl->entries, impl->capacity, key);
	if (index == SIZE_MAX) {
		return;
	}
	
	map_entry_t* entry = &impl->entries[index];
	entry->is_occupied = false;
	entry->is_deleted = true;
	entry->value = NULL;
	
	impl->count--;
	impl->deleted_count++;
}

void map_remove_string(map_t* map, const char* key)
{
	assert(key);
	map_remove(map, hash_string(key));
}

void map_clear(map_t* map)
{
	assert(map);
	map_impl_t* impl = to_impl(map);
	
	// Clear all entries
	memset(impl->entries, 0, sizeof(map_entry_t) * impl->capacity);
	impl->count = 0;
	impl->deleted_count = 0;
}

void map_iterate(map_t* map, map_iterate_fn callback, void* user_data)
{
	assert(map);
	assert(callback);
	
	map_impl_t* impl = to_impl(map);
	
	for (size_t i = 0; i < impl->capacity; i++)
	{
		map_entry_t* entry = &impl->entries[i];
		if (entry->is_occupied && !entry->is_deleted)
		{
			callback(entry->key, entry->value, user_data);
		}
	}
}
