//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct font_glyph 
{
    vec2_t uv_min;
    vec2_t uv_max;
    vec2_t size;
    float advance;
    vec2_t bearing;
    vec2_t sdf_offset;
} font_glyph_t;

typedef struct font_kerning_entry 
{
    uint64_t key;           // (first_char << 32) | second_char
    float amount;
    UT_hash_handle hh;      // uthash handle
} font_kerning_entry_t;

typedef struct font_glyph_entry 
{
    char character;
    font_glyph_t glyph;
    UT_hash_handle hh;      // uthash handle
} font_glyph_entry_t;

// Font implementation
typedef struct font_impl 
{
    string128_t name;
    material_t material;
    texture_t texture;
    float baseline;
    font_glyph_entry_t* glyphs;     // hash table of glyphs
    font_kerning_entry_t* kerning;  // hash table of kerning pairs
} font_impl_t;

static object_registry_t g_font_cache = NULL;
static SDL_GPUDevice* g_device = NULL;
static object_type_t g_font_type = NULL;

static void font_destroy_impl(font_impl_t* impl)
{
    assert(impl);
    
    // Free glyph hash table
    font_glyph_entry_t* glyph_entry;
    font_glyph_entry_t* glyph_tmp;
    HASH_ITER(hh, impl->glyphs, glyph_entry, glyph_tmp) 
    {
        HASH_DEL(impl->glyphs, glyph_entry);
        free(glyph_entry);
    }
    
    // Free kerning hash table
    font_kerning_entry_t* kerning_entry;
    font_kerning_entry_t* kerning_tmp;
    HASH_ITER(hh, impl->kerning, kerning_entry, kerning_tmp) 
    {
        HASH_DEL(impl->kerning, kerning_entry);
        free(kerning_entry);
    }
    
    impl->glyphs = NULL;
    impl->kerning = NULL;
}

font_t font_load(const char* name)
{
    assert(g_device);
    assert(g_font_cache);
    assert(name);

    uint64_t key = hash_string(name);

    // Check if font exists in cache
    object_t cached_obj = object_registry_get(g_font_cache, key);
    if (cached_obj) 
    {
        return (font_t)cached_obj;
    }

    const char* font_path = asset_path(name, "font");
    stream_t stream = stream_create_from_file(font_path);
    if (!stream) 
    {
        return NULL;
    }

    // Read header
    if (!stream_read_signature(stream, "FONT", 4))
    {
        stream_destroy(stream);
        return NULL;
    }

    uint32_t version = stream_read_uint32(stream);
    if (version != 1)
    {
        stream_destroy(stream);
        return NULL;
    }

    // Create new font
    object_t cache_obj = object_registry_alloc(g_font_cache, key);
    if (!cache_obj) 
    {
        stream_destroy(stream);
        return NULL;
    }
    
    font_impl_t* impl = (font_impl_t*)object_impl(cache_obj, g_font_type);
    
    // Initialize implementation
    impl->name = malloc(strlen(name) + 1);
    if (!impl->name) 
    {
        stream_destroy(stream);
        object_destroy(cache_obj);
        return NULL;
    }
    strcpy(impl->name, name);
    
    impl->glyphs = NULL;    // uthash tables start as NULL
    impl->kerning = NULL;

    uint32_t original_font_size = stream_read_uint32(stream);
    uint32_t atlas_width = stream_read_uint32(stream);
    uint32_t atlas_height = stream_read_uint32(stream);
    float ascent = stream_read_float(stream);
    float descent = stream_read_float(stream);
    float line_height = stream_read_float(stream);
    impl->baseline = stream_read_float(stream);

    // Read glyph count and glyph data
    uint16_t glyph_count = stream_read_uint16(stream);
    for (uint32_t i = 0; i < glyph_count; ++i)
    {
        uint32_t codepoint = stream_read_uint32(stream);
        float uvx = stream_read_float(stream);
        float uvy = stream_read_float(stream);
        float stx = stream_read_float(stream);
        float sty = stream_read_float(stream);
        float width = stream_read_float(stream);
        float height = stream_read_float(stream);
        float advance_x = stream_read_float(stream);
        float advance_y = stream_read_float(stream);
        float bearing_x = stream_read_float(stream);
        float bearing_y = stream_read_float(stream);
        float sdf_distance = stream_read_float(stream);

        // Create glyph entry
        font_glyph_entry_t* glyph_entry = malloc(sizeof(font_glyph_entry_t));
        if (!glyph_entry) continue;
        
        glyph_entry->character = (char)codepoint;
        glyph_entry->glyph.uv_min.x = uvx;
        glyph_entry->glyph.uv_min.y = uvy;
        glyph_entry->glyph.uv_max.x = stx;
        glyph_entry->glyph.uv_max.y = sty;
        glyph_entry->glyph.size.x = width;
        glyph_entry->glyph.size.y = height;
        glyph_entry->glyph.advance = advance_x;
        glyph_entry->glyph.bearing.x = bearing_x;
        glyph_entry->glyph.bearing.y = bearing_y;
        glyph_entry->glyph.sdf_offset.x = sdf_distance;
        glyph_entry->glyph.sdf_offset.y = sdf_distance;

        HASH_ADD(hh, impl->glyphs, character, sizeof(char), glyph_entry);
    }

    // Read kerning count and kerning data
    uint16_t kerning_count = stream_read_uint16(stream);
    for (uint32_t i = 0; i < kerning_count; ++i)
    {
        uint32_t first = stream_read_uint32(stream);
        uint32_t second = stream_read_uint32(stream);
        float amount = stream_read_float(stream);
        
        font_kerning_entry_t* kerning_entry = malloc(sizeof(font_kerning_entry_t));
        if (!kerning_entry) continue;
        
        kerning_entry->key = ((uint64_t)first << 32) | (uint64_t)second;
        kerning_entry->amount = amount;
        
        HASH_ADD(hh, impl->kerning, key, sizeof(uint64_t), kerning_entry);
    }

    // Read atlas data
    uint32_t atlas_data_size = atlas_width * atlas_height; // R8 format
    uint8_t* atlas_data = malloc(atlas_data_size);
    if (!atlas_data) 
    {
        stream_destroy(stream);
        font_destroy_impl(impl);
        object_destroy(cache_obj);
        return NULL;
    }
    
    stream_read_bytes(stream, atlas_data, atlas_data_size);
    stream_destroy(stream);
    
    impl->texture = create_texture(atlas_data, atlas_width, atlas_height, texture_format_r8, name);
    free(atlas_data);
    
    if (!impl->texture) 
    {
        font_destroy_impl(impl);
        object_destroy(cache_obj);
        return NULL;
    }
    
    impl->material = create_material("shaders/text", "font");
    if (!impl->material) 
    {
        font_destroy_impl(impl);
        object_destroy(cache_obj);
        return NULL;
    }
    
    set_texture(impl->material, impl->texture);

    return (font_t)cache_obj;
}

// Property getter functions with new naming convention
const font_glyph_t* font_glyph(font_t font, char ch)
{
    if (!font) return NULL;
    
    font_impl_t* impl = (font_impl_t*)object_impl((object_t)font, g_font_type);
    
    // Find glyph in hash table
    font_glyph_entry_t* glyph_entry;
    HASH_FIND(hh, impl->glyphs, &ch, sizeof(char), glyph_entry);
    if (glyph_entry) 
    {
        return &glyph_entry->glyph;
    }

    // If character not found, try unknown glyph (ASCII DEL character)
    char unknown_ch = 0x7F;
    HASH_FIND(hh, impl->glyphs, &unknown_ch, sizeof(char), glyph_entry);
    if (glyph_entry) 
    {
        return &glyph_entry->glyph;
    }

    // Return default glyph if nothing found
    static font_glyph_t default_glyph = {
        {0.0f, 0.0f},   // uv_min
        {0.0f, 0.0f},   // uv_max
        {0.0f, 0.0f},   // size
        0.0f,           // advance
        {0.0f, 0.0f},   // bearing
        {0.0f, 0.0f}    // sdf_offset
    };

    return &default_glyph;
}

float font_kerning(font_t font, char first, char second)
{
    if (!font) return 0.0f;
    
    font_impl_t* impl = (font_impl_t*)object_impl((object_t)font, g_font_type);
    
    uint64_t key = ((uint64_t)first << 32) | (uint64_t)second;
    
    font_kerning_entry_t* kerning_entry;
    HASH_FIND(hh, impl->kerning, &key, sizeof(uint64_t), kerning_entry);
    if (kerning_entry) 
    {
        return kerning_entry->amount;
    }

    return 0.0f;
}

float font_baseline(font_t font)
{
    if (!font) return 0.0f;
    font_impl_t* impl = (font_impl_t*)object_impl((object_t)font, g_font_type);
    return impl->baseline;
}

material_t font_material(font_t font)
{
    if (!font) return NULL;
    font_impl_t* impl = (font_impl_t*)object_impl((object_t)font, g_font_type);
    return impl->material;
}

// Legacy function names for compatibility with existing code
font_t load_font(const char* name)
{
    return font_load(name);
}

const font_glyph_t* get_glyph(font_t font, char ch)
{
    return font_glyph(font, ch);
}

float get_kerning(font_t font, char first, char second)
{
    return font_kerning(font, first, second);
}

float get_baseline(font_t font)
{
    return font_baseline(font);
}

material_t get_material(font_t font)
{
    return font_material(font);
}

static void font_init(const renderer_traits* traits, SDL_GPUDevice* device)
{
    g_font_type = object_type_create("font");
    g_font_cache = object_registry_create(g_font_type, sizeof(font_impl_t), INITIAL_CACHE_SIZE);
    g_device = device;
}

static void font_uninit()
{
    object_destroy((object_t)g_font_cache);
    g_font_cache = nullptr;
    g_font_type = nullptr;
    g_device = nullptr;
}
