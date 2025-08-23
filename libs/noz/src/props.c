//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/props.h>
#include <stdio.h>

#define MAX_PROPERTY_VALUES 32
#define HASH_SIZE 256

typedef struct prop_value
{
    prop_type_t type;
    text_t key;                                  // Store the key for iteration
    union
    {
        text_t single;                           // Single value
        struct
        {
            text_t values[MAX_PROPERTY_VALUES];  // List values
            size_t count;
        } list;
    };
} prop_value_t;

typedef struct props_impl
{
    OBJECT_BASE;
    map_t* property_map;        // Maps name_t key hash -> prop_value_t*
    prop_value_t* pool;    // Pool of prop values
    size_t pool_size;
    size_t pool_used;
    
    // Cache for key enumeration
    name_t* key_cache;
    size_t key_cache_size;
    size_t key_count;
    bool key_cache_dirty;
} props_impl_t;

static prop_value_t* alloc_prop_value(props_impl_t* impl);
static void rebuild_key_cache(props_impl_t* impl);

// Parsing functions
static bool parse_ini_line_with_section(props_impl_t* impl, const char* line, const char* section);
static bool parse_vector(const char* str, vec3_t* result);

// Implementation
static inline props_impl_t* to_impl(void* p) { return (props_impl_t*)to_object(p, type_props); }

props_t* props_alloc(allocator_t* allocator)
{
    props_t* props = (props_t*)object_alloc(allocator, sizeof(props_impl_t), type_props);
    if (!props)
        return NULL;
    
    props_impl_t* impl = to_impl(props);
    impl->property_map = map_alloc(allocator, HASH_SIZE);
    impl->pool_size = HASH_SIZE;
    impl->pool = (prop_value_t*)calloc(impl->pool_size, sizeof(prop_value_t));
    impl->pool_used = 0;
    
    impl->key_cache_size = 64;
    impl->key_cache = (name_t*)calloc(impl->key_cache_size, sizeof(name_t));
    impl->key_count = 0;
    impl->key_cache_dirty = false;
    
    return props;
}

#if 0
void props_destroy(props_t* props)
{
    props_impl_t* impl = to_impl(props);
    
    if (impl->property_map)
        object_free(impl->property_map);
    
    if (impl->pool)
    {
        free(impl->pool);
    }
    
    if (impl->key_cache)
    {
        free(impl->key_cache);
    }
    
    object_destroy((object_t)props);
}
#endif

props_t* props_load_from_file(allocator_t* allocator, const path_t* file_path)
{
    assert(file_path);
    
    stream_t* stream = stream_load_from_file(allocator, file_path);
    if (!stream)
        return false;
    
    stream_seek_end(stream, 0);
    stream_write_uint8(stream, 0);
    
    props_t* result = props_load_from_memory(allocator, (const char*)stream_data(stream), stream_size(stream) - 1);

    object_free(stream);
    
    return result;
}

props_t* props_load_from_memory(allocator_t* allocator, const char* content, size_t content_length)
{
    assert(content);
    assert(content_length > 0);
    assert(content[content_length] == 0);
    
    props_t* props = (props_t*)props_alloc(allocator);
    if (!props)
        return NULL;

    props_impl_t* impl = to_impl(props);

    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, content);
    
    text_t line;
    text_t current_section;
    text_init(&current_section);
    
    while (tokenizer_read_line(&tokenizer, &line))
    {
        text_trim(&line);
        
        // Skip empty lines and comments
        if (line.length == 0 || line.value[0] == ';' || line.value[0] == '#')
        {
            continue;
        }
        
        // Handle sections [section_name]
        if (line.value[0] == '[')
        {
            // Extract section name
            if (line.length >= 3 && line.value[line.length - 1] == ']')
            {
                // Copy section name without brackets
                text_clear(&current_section);
                for (size_t i = 1; i < line.length - 1; i++)
                {
                    char ch_str[2] = {line.value[i], '\0'};
                    text_append(&current_section, ch_str);
                }
            }
            continue;
        }
        
        // Parse line content
        if (strchr(line.value, '=') != NULL)
        {
            // Key=value format - use current section as prefix
            if (!parse_ini_line_with_section(impl, line.value, current_section.value))
            {
                // Continue parsing even if a line fails
            }
        }
        else
        {
            // List item format - add to current section as list
            if (current_section.length > 0)
            {
                props_add_to_list(props, current_section.value, line.value);
            }
        }
    }
    
    impl->key_cache_dirty = true;
    return props;
}

void props_clear(props_t* props)
{
    if (!props)
    {
        return;
    }
    
    props_impl_t* impl = to_impl(props);
    
    // Clear the map (this doesn't free the property values)
    map_clear(impl->property_map);
    
    // Reset pool
    impl->pool_used = 0;
    memset(impl->pool, 0, impl->pool_size * sizeof(prop_value_t));
    
    impl->key_count = 0;
    impl->key_cache_dirty = true;
}

void props_set_string(props_t* props, const char* key, const char* value)
{
    if (!props || !key || !value)
    {
        return;
    }
    
    props_impl_t* impl = to_impl(props);
    name_t key_name;
    name_set(&key_name, key);
    
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    if (!prop)
    {
        prop = alloc_prop_value(impl);
        if (!prop)
        {
            return;
        }
        map_set(impl->property_map, hash_key, prop);
        impl->key_cache_dirty = true;
    }
    
    prop->type = prop_type_value;
    text_set(&prop->key, key);
    text_set(&prop->single, value);
}

void props_set_int(props_t* props, const char* key, int value)
{
    text_t str_value;
    text_format(&str_value, "%d", value);
    props_set_string(props, key, str_value.value);
}

void props_set_float(props_t* props, const char* key, float value)
{
    text_t str_value;
    text_format(&str_value, "%.6f", value);
    props_set_string(props, key, str_value.value);
}

void props_set_vec3(props_t* props, const char* key, vec3_t value)
{
    text_t str_value;
    text_format(&str_value, "(%.6f,%.6f,%.6f)", value.x, value.y, value.z);
    props_set_string(props, key, str_value.value);
}

void props_add_to_list(props_t* props, const char* key, const char* value)
{
    if (!props || !key || !value)
    {
        return;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    if (!prop)
    {
        prop = alloc_prop_value(impl);
        if (!prop)
        {
            return;
        }
        prop->type = prop_type_list;
        text_set(&prop->key, key);
        prop->list.count = 0;
        map_set(impl->property_map, hash_key, prop);
        impl->key_cache_dirty = true;
    }
    
    // Convert single to list if needed
    if (prop->type == prop_type_value)
    {
        text_t old_value = prop->single;
        prop->type = prop_type_list;
        prop->list.count = 1;
        prop->list.values[0] = old_value;
    }
    
    // Add to list
    if (prop->list.count < MAX_PROPERTY_VALUES)
    {
        text_set(&prop->list.values[prop->list.count], value);
        prop->list.count++;
    }
}

bool props_has_key(props_t* props, const char* key)
{
    if (!props || !key)
    {
        return false;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    return map_get(impl->property_map, hash_key) != NULL;
}

bool props_is_list(props_t* props, const char* key)
{
    if (!props || !key)
    {
        return false;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    return prop && prop->type == prop_type_list;
}

prop_type_t props_get_type(props_t* props, const char* key)
{
    if (!props || !key)
    {
        return prop_type_value;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    return prop ? prop->type : prop_type_value;
}

const char* props_get_string(props_t* props, const char* key, const char* default_value)
{
    if (!props || !key)
    {
        return default_value;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    if (!prop || prop->type != prop_type_value)
    {
        return default_value;
    }
    
    return prop->single.value;
}

int props_get_int(props_t* props, const char* key, int default_value)
{
    const char* str_value = props_get_string(props, key, NULL);
    if (!str_value)
    {
        // For lists, return count
        if (props_is_list(props, key))
        {
            return (int)props_get_list_count(props, key);
        }
        return default_value;
    }
    
    return atoi(str_value);
}

float props_get_float(props_t* props, const char* key, float default_value)
{
    const char* str_value = props_get_string(props, key, NULL);
    if (!str_value)
    {
        return default_value;
    }
    
    return (float)atof(str_value);
}

vec3_t props_get_vec3(props_t* props, const char* key, vec3_t default_value)
{
    const char* str_value = props_get_string(props, key, NULL);
    if (!str_value)
    {
        return default_value;
    }
    
    vec3_t result;
    if (parse_vector(str_value, &result))
    {
        return result;
    }
    
    return default_value;
}

size_t props_get_list_count(props_t* props, const char* key)
{
    if (!props || !key)
    {
        return 0;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    if (!prop)
    {
        return 0;
    }
    
    return prop->type == prop_type_list ? prop->list.count : 1;
}

const char* props_get_list_item(props_t* props, const char* key, size_t index, const char* default_value)
{
    if (!props || !key)
    {
        return default_value;
    }
    
    props_impl_t* impl = to_impl(props);
    uint64_t hash_key = hash_string(key);
    prop_value_t* prop = (prop_value_t*)map_get(impl->property_map, hash_key);
    
    if (!prop)
    {
        return default_value;
    }
    
    if (prop->type == prop_type_value)
    {
        return index == 0 ? prop->single.value : default_value;
    }
    
    if (index >= prop->list.count)
    {
        return default_value;
    }
    
    return prop->list.values[index].value;
}

size_t props_get_key_count(props_t* props)
{
    if (!props)
    {
        return 0;
    }
    
    props_impl_t* impl = to_impl(props);
    if (impl->key_cache_dirty)
    {
        rebuild_key_cache(impl);
    }
    
    return impl->key_count;
}

const char* props_get_key_at(props_t* props, size_t index)
{
    if (!props)
    {
        return NULL;
    }
    
    props_impl_t* impl = to_impl(props);
    if (impl->key_cache_dirty)
    {
        rebuild_key_cache(impl);
    }
    
    if (index >= impl->key_count)
    {
        return NULL;
    }
    
    return impl->key_cache[index].value;
}

void props_print(props_t* props)
{
    if (!props)
    {
        return;
    }
    
    size_t count = props_get_key_count(props);
    printf("Props (%zu keys):\n", count);
    
    for (size_t i = 0; i < count; i++)
    {
        const char* key = props_get_key_at(props, i);
        if (props_is_list(props, key))
        {
            size_t list_count = props_get_list_count(props, key);
            printf("  %s = [", key);
            for (size_t j = 0; j < list_count; j++)
            {
                const char* value = props_get_list_item(props, key, j, "");
                printf("%s\"%s\"", j > 0 ? ", " : "", value);
            }
            printf("]\n");
        }
        else
        {
            const char* value = props_get_string(props, key, "");
            printf("  %s = \"%s\"\n", key, value);
        }
    }
}

// Private helper functions

static prop_value_t* alloc_prop_value(props_impl_t* impl)
{
    if (impl->pool_used >= impl->pool_size)
    {
        return NULL;  // Pool exhausted
    }
    
    return &impl->pool[impl->pool_used++];
}


static void rebuild_key_cache(props_impl_t* impl)
{
    impl->key_cache_dirty = false;
    impl->key_count = 0;
    
    // Build key cache from used pool entries
    for (size_t i = 0; i < impl->pool_used; i++)
    {
        prop_value_t* prop = &impl->pool[i];
        if (prop->type != prop_type_value && prop->type != prop_type_list)
        {
            continue; // Skip unused entries
        }
        
        // Ensure we have space in the cache
        if (impl->key_count >= impl->key_cache_size)
        {
            break; // Cache is full
        }
        
        // Copy the key to the cache
        name_set(&impl->key_cache[impl->key_count], prop->key.value);
        impl->key_count++;
    }
    
}


// Parsing functions


static bool parse_ini_line_with_section(props_impl_t* impl, const char* line, const char* section)
{
    tokenizer_t tok;
    tokenizer_init(&tok, line);
    
    // Read key
    text_t key;
    if (!tokenizer_read_until(&tok, '=', &key))
    {
        return false;
    }
    
    text_trim(&key);
    if (key.length == 0)
    {
        return false;
    }
    
    // Create full key with section prefix
    text_t full_key;
    text_init(&full_key);
    
    if (section && strlen(section) > 0)
    {
        text_set(&full_key, section);
        text_append(&full_key, ".");
        text_append(&full_key, key.value);
    }
    else
    {
        text_copy(&full_key, &key);
    }
    
    // Skip '='
    if (tokenizer_peek(&tok) != '=')
    {
        return false;
    }
    tokenizer_next(&tok);
    
    // Read value
    text_t value;
    tokenizer_skip_whitespace(&tok);
    
    // Read rest of line as value
    text_clear(&value);
    while (tokenizer_has_more(&tok))
    {
        char ch_str[2] = {tokenizer_next(&tok), '\0'};
        text_append(&value, ch_str);
    }
    
    text_trim(&value);
    
    // Store the property
    uint64_t hash_key = hash_string(full_key.value);
    prop_value_t* prop = alloc_prop_value(impl);
    if (!prop)
    {
        return false;
    }
    
    prop->type = prop_type_value;
    text_copy(&prop->key, &full_key);
    text_copy(&prop->single, &value);
    
    map_set(impl->property_map, hash_key, prop);
    impl->key_cache_dirty = true;
    
    return true;
}

static bool parse_vector(const char* str, vec3_t* result)
{
    if (!str || !result)
    {
        return false;
    }
    
    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, str);
    
    return tokenizer_read_vec3(&tokenizer, result);
}

