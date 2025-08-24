//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/props.h>
#include <cstdio>
#include <cstring>

#define MAX_PROPERTY_VALUES 32
#define HASH_SIZE 256

struct PropValue
{
    PropType type;
    text_t key;
    union
    {
        text_t single;
        struct
        {
            text_t values[MAX_PROPERTY_VALUES];
            size_t count;
        } list;
    };
};

struct PropsImpl
{
    OBJECT_BASE;
    Map* property_map;
    PropValue* pool;
    size_t pool_size;
    size_t pool_used;
    name_t* key_cache;
    size_t key_cache_size;
    size_t key_count;
    bool key_cache_dirty;
};

static PropValue* AllocPropValue(PropsImpl* impl);
static void RebuildKeyCache(PropsImpl* impl);
static bool ParseLine(PropsImpl* impl, const char* line, const char* section);
static bool ParseVector(const char* str, vec3* result);
static bool ParseColor(const char* str, color_t* result);

static PropsImpl* Impl(Props * p) { return (PropsImpl*)Cast((Object*)p, TYPE_PROPS); }

Props* CreateProps(Allocator* allocator)
{
    auto* props = (Props*)CreateObject(allocator, sizeof(PropsImpl), TYPE_PROPS);
    if (!props)
        return nullptr;
    
    PropsImpl* impl = Impl(props);
    impl->property_map = CreateMap(allocator, HASH_SIZE);
    impl->pool_size = HASH_SIZE;
    impl->pool = (PropValue*)calloc(impl->pool_size, sizeof(PropValue));
    impl->pool_used = 0;
    
    impl->key_cache_size = 64;
    impl->key_cache = (name_t*)calloc(impl->key_cache_size, sizeof(name_t));
    impl->key_count = 0;
    impl->key_cache_dirty = false;
    
    return props;
}

#if 0
void props_destroy(Props* props)
{
    PropsImpl* impl = Impl(props);
    
    if (impl->property_map)
        Destroy(impl->property_map);
    
    if (impl->pool)
    {
        free(impl->pool);
    }
    
    if (impl->key_cache)
    {
        free(impl->key_cache);
    }
    
    object_destroy((Object)props);
}
#endif

Props* LoadProps(Allocator* allocator, Stream* stream)
{
    if (!stream)
        return nullptr;
    
    SeekEnd(stream, 0);
    WriteU8(stream, 0);
    
    Props* result = LoadProps(allocator, (const char*)GetData(stream), GetSize(stream) - 1);
    
    return result;
}

Props* LoadProps(Allocator* allocator, const char* content, size_t content_length)
{
    assert(content);
    assert(content_length > 0);
    assert(content[content_length] == 0);
    
    auto* props = CreateProps(allocator);
    if (!props)
        return nullptr;

    PropsImpl* impl = Impl(props);

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
        if (strchr(line.value, '=') != nullptr)
        {
            // Key=value format - use current section as prefix
            if (!ParseLine(impl, line.value, current_section.value))
            {
                // Continue parsing even if a line fails
            }
        }
        else
        {
            // List item format - add to current section as list
            if (current_section.length > 0)
            {
                AddToList(props, current_section.value, line.value);
            }
        }
    }
    
    impl->key_cache_dirty = true;
    return props;
}

void Clear(Props* props)
{
    if (!props)
    {
        return;
    }
    
    PropsImpl* impl = Impl(props);
    
    // Clear the map (this doesn't free the property values)
    Clear(impl->property_map);
    
    // Reset pool
    impl->pool_used = 0;
    memset(impl->pool, 0, impl->pool_size * sizeof(PropValue));
    
    impl->key_count = 0;
    impl->key_cache_dirty = true;
}

void SetString(Props* props, const char* key, const char* value)
{
    if (!props || !key || !value)
    {
        return;
    }
    
    PropsImpl* impl = Impl(props);
    name_t key_name;
    name_set(&key_name, key);
    
    uint64_t hash_key = Hash(key);
    PropValue* prop = (PropValue*)GetValue(impl->property_map, hash_key);
    
    if (!prop)
    {
        prop = AllocPropValue(impl);
        if (!prop)
        {
            return;
        }
        SetValue(impl->property_map, hash_key, prop);
        impl->key_cache_dirty = true;
    }
    
    prop->type = PROP_TYPE_VALUE;
    text_set(&prop->key, key);
    text_set(&prop->single, value);
}

void SetInt(Props* props, const char* key, int value)
{
    text_t str_value;
    text_format(&str_value, "%d", value);
    SetString(props, key, str_value.value);
}

void SetFloat(Props* props, const char* key, float value)
{
    text_t str_value;
    text_format(&str_value, "%.6f", value);
    SetString(props, key, str_value.value);
}

void SetVec3(Props* props, const char* key, vec3 value)
{
    text_t str_value;
    text_format(&str_value, "(%.6f,%.6f,%.6f)", value.x, value.y, value.z);
    SetString(props, key, str_value.value);
}

void SetColor(Props* props, const char* key, color_t value)
{
    text_t str_value;
    text_format(&str_value, "rgba(%.0f,%.0f,%.0f,%.3f)", 
                value.r * 255.0f, value.g * 255.0f, value.b * 255.0f, value.a);
    SetString(props, key, str_value.value);
}

void AddToList(Props* props, const char* key, const char* value)
{
    if (!props || !key || !value)
    {
        return;
    }
    
    auto* impl = Impl(props);
    uint64_t prop_key = Hash(key);
    auto* prop_value = (PropValue*)GetValue(impl->property_map, prop_key);
    
    if (!prop_value)
    {
        prop_value = AllocPropValue(impl);
        if (!prop_value)
        {
            return;
        }
        prop_value->type = PROP_TYPE_LIST;
        text_set(&prop_value->key, key);
        prop_value->list.count = 0;
        SetValue(impl->property_map, prop_key, prop_value);
        impl->key_cache_dirty = true;
    }
    
    // Convert single to list if needed
    if (prop_value->type == PROP_TYPE_VALUE)
    {
        text_t old_value = prop_value->single;
        prop_value->type = PROP_TYPE_LIST;
        prop_value->list.count = 1;
        prop_value->list.values[0] = old_value;
    }
    
    // Add to list
    if (prop_value->list.count < MAX_PROPERTY_VALUES)
    {
        text_set(&prop_value->list.values[prop_value->list.count], value);
        prop_value->list.count++;
    }
}

bool HasKey(Props* props, const char* key)
{
    assert(key);
    return GetValue(Impl(props)->property_map, Hash(key)) != nullptr;
}

bool IsList(Props* props, const char* key)
{
    assert(key);
    auto* prop = (PropValue*)GetValue(Impl(props)->property_map, Hash(key));
    return prop && prop->type == PROP_TYPE_LIST;
}

PropType GetPropertyType(Props* props, const char* key)
{
    assert(key);
    auto* prop = (PropValue*)GetValue(Impl(props)->property_map, Hash(key));
    return prop ? prop->type : PROP_TYPE_VALUE;
}

const char* GetString(Props* props, const char* key, const char* default_value)
{
    assert(key);

    auto* prop = (PropValue*)GetValue(Impl(props)->property_map, Hash(key));
    if (!prop || prop->type != PROP_TYPE_VALUE)
        return default_value;

    return prop->single.value;
}

int GetInt(Props* props, const char* key, int default_value)
{
    const char* str_value = GetString(props, key, nullptr);
    if (!str_value)
    {
        // For lists, return count
        if (IsList(props, key))
        {
            return (int)GetListCount(props, key);
        }
        return default_value;
    }
    
    return atoi(str_value);
}

float GetFloat(Props* props, const char* key, float default_value)
{
    const char* str_value = GetString(props, key, nullptr);
    if (!str_value)
        return default_value;

    return (float)atof(str_value);
}

bool GetBool(Props* props, const char* key, bool default_value)
{
    const char* str_value = GetString(props, key, nullptr);
    if (!str_value)
        return default_value;

    if (strcmp(str_value, "true") == 0 ||
        strcmp(str_value, "1") == 0 ||
        strcmp(str_value, "yes") == 0 ||
        strcmp(str_value, "on") == 0)
        return true;

    if (strcmp(str_value, "false") == 0 ||
        strcmp(str_value, "0") == 0 ||
        strcmp(str_value, "no") == 0 ||
        strcmp(str_value, "off") == 0)
        return false;

    return default_value;
}

vec3 GetVec3(Props* props, const char* key, vec3 default_value)
{
    const char* str_value = GetString(props, key, nullptr);
    if (!str_value)
        return default_value;

    vec3 result;
    if (ParseVector(str_value, &result))
        return result;

    return default_value;
}

color_t GetColor(Props* props, const char* key, color_t default_value)
{
    const char* str_value = GetString(props, key, nullptr);
    if (!str_value)
        return default_value;

    color_t result;
    if (ParseColor(str_value, &result))
        return result;

    return default_value;
}

size_t GetListCount(Props* props, const char* key)
{
    assert(key);

    auto* prop = (PropValue*)GetValue(Impl(props)->property_map, Hash(key));
    if (!prop)
        return 0;

    return prop->type == PROP_TYPE_LIST ? prop->list.count : 1;
}

const char* GetListElement(Props* props, const char* key, size_t index, const char* default_value)
{
    assert(key);

    auto prop = (PropValue*)GetValue(Impl(props)->property_map, Hash(key));
    if (!prop)
        return default_value;

    if (prop->type == PROP_TYPE_VALUE)
        return index == 0 ? prop->single.value : default_value;

    if (index >= prop->list.count)
        return default_value;

    return prop->list.values[index].value;
}

size_t GetKeyCount(Props* props)
{
    auto* impl = Impl(props);
    if (impl->key_cache_dirty)
        RebuildKeyCache(impl);

    return impl->key_count;
}

const char* GetKeyAt(Props* props, size_t index)
{
    auto impl = Impl(props);
    if (impl->key_cache_dirty)
        RebuildKeyCache(impl);

    if (index >= impl->key_count)
        return nullptr;

    return impl->key_cache[index].value;
}

void Print(Props* props)
{
    if (!props)
        return;

    size_t count = GetKeyCount(props);
    printf("Props (%zu keys):\n", count);
    
    for (size_t i = 0; i < count; i++)
    {
        const char* key = GetKeyAt(props, i);
        if (IsList(props, key))
        {
            size_t list_count = GetListCount(props, key);
            printf("  %s = [", key);
            for (size_t j = 0; j < list_count; j++)
            {
                const char* value = GetListElement(props, key, j, "");
                printf("%s\"%s\"", j > 0 ? ", " : "", value);
            }
            printf("]\n");
        }
        else
        {
            const char* value = GetString(props, key, "");
            printf("  %s = \"%s\"\n", key, value);
        }
    }
}

static PropValue* AllocPropValue(PropsImpl* impl)
{
    if (impl->pool_used >= impl->pool_size)
        return nullptr;

    return &impl->pool[impl->pool_used++];
}


static void RebuildKeyCache(PropsImpl* impl)
{
    impl->key_cache_dirty = false;
    impl->key_count = 0;
    
    // Build key cache from used pool entries
    for (size_t i = 0; i < impl->pool_used; i++)
    {
        PropValue* prop = &impl->pool[i];
        if (prop->type != PROP_TYPE_VALUE && prop->type != PROP_TYPE_LIST)
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

static bool ParseLine(PropsImpl* impl, const char* line, const char* section)
{
    tokenizer_t tok;
    tokenizer_init(&tok, line);
    
    // Read key
    text_t key;
    if (!tokenizer_read_until(&tok, '=', &key))
        return false;

    text_trim(&key);
    if (key.length == 0)
        return false;

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
        return false;

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
    uint64_t hash_key = Hash(full_key.value);
    PropValue* prop = AllocPropValue(impl);
    if (!prop)
        return false;

    prop->type = PROP_TYPE_VALUE;
    text_copy(&prop->key, &full_key);
    text_copy(&prop->single, &value);
    
    SetValue(impl->property_map, hash_key, prop);
    impl->key_cache_dirty = true;
    
    return true;
}

static bool ParseVector(const char* str, vec3* result)
{
    assert(str);
    assert(result);

    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, str);
    return tokenizer_read_vec3(&tokenizer, result);
}

static bool ParseColor(const char* str, color_t* result)
{
    assert(str);
    assert(result);

    tokenizer_t tokenizer;
    tokenizer_init(&tokenizer, str);
    return tokenizer_read_color(&tokenizer, result);
}

