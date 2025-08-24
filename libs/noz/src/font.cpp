//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define MAX_GLYPHS 256  // Support extended ASCII
#define MAX_KERNING (MAX_GLYPHS * MAX_GLYPHS)  // All possible kerning pairs

typedef struct font_glyph 
{
    vec2 uv_min;
    vec2 uv_max;
    vec2 size;
    float advance;
    vec2 bearing;
    vec2 sdf_offset;
} font_glyph_t;

typedef struct font_impl 
{
    OBJECT_BASE;
    name_t name;
    material_t* material;
    texture_t* texture;
    float baseline;
	uint32_t original_font_size;
    float descent;
    float ascent;
    float line_height;
	int atlas_width;
	int atlas_height;
    font_glyph_t glyphs[MAX_GLYPHS];     // Fixed array for glyphs (advance == 0 means no glyph)
    uint16_t kerning_index[MAX_KERNING];  // Index into kerning_values array (0xFFFF = no kerning)
    float* kerning_values;                 // Dynamic array of actual kerning values
    uint16_t kerning_count;                // Number of kerning pairs
} font_impl_t;

static map_t* g_font_cache = NULL;
static SDL_GPUDevice* g_device = NULL;

inline font_impl_t* to_impl(void* s) { return (font_impl_t*)to_object((object_t*)(s), type_font); }

#if 0
static void font_destroy_impl(font_impl_t* impl)
{
    assert(impl);
    // Free dynamic kerning values array
    if (impl->kerning_values) {
        free(impl->kerning_values);
        impl->kerning_values = NULL;
    }
}
#endif

font_t* font_load_from_stream(allocator_t* allocator, stream_t* stream, name_t* name)
{
    // Read header
    if (!stream_read_signature(stream, "FONT", 4))
    {
        object_free(stream);
        return NULL;
    }

    uint32_t version = stream_read_uint32(stream);
    if (version != 1)
    {
        object_free(stream);
        return NULL;
    }

    font_impl_t* impl = (font_impl_t*)object_alloc(allocator, sizeof(font_impl_t), type_font);
    if (!impl)
        return NULL;

    name_copy(&impl->name, name);

    // Arrays are already zeroed by object_create
    // Initialize kerning index to 0xFFFF (no kerning)
    memset(impl->kerning_index, 0xFF, sizeof(impl->kerning_index));
    impl->kerning_values = NULL;
    impl->kerning_count = 0;

    impl->original_font_size = stream_read_uint32(stream);
    impl->atlas_width = stream_read_uint32(stream);
    impl->atlas_height = stream_read_uint32(stream);
    impl->ascent = stream_read_float(stream);
    impl->descent = stream_read_float(stream);
    impl->line_height = stream_read_float(stream);
    impl->baseline = stream_read_float(stream);

    // Read glyph count and glyph data
    uint16_t glyph_count = stream_read_uint16(stream);

    // New efficient format: codepoint, then font_glyph_t structure directly
    for (uint32_t i = 0; i < glyph_count; ++i)
    {
        uint32_t codepoint = stream_read_uint32(stream);

        if (codepoint < MAX_GLYPHS) {
            // Read directly into the glyph structure
            font_glyph_t* glyph = &impl->glyphs[codepoint];
            stream_read(stream, glyph, sizeof(font_glyph_t));
        }
        else {
            // Skip this glyph's data
            stream_seek_begin(stream, stream_position(stream) + sizeof(font_glyph_t));
        }
    }

    // Read kerning count and kerning data
    impl->kerning_count = stream_read_uint16(stream);

    if (impl->kerning_count > 0) {
        // Allocate kerning values array
        impl->kerning_values = (float*)malloc(impl->kerning_count * sizeof(float));
        if (!impl->kerning_values) {
            impl->kerning_count = 0;
        }
        else {
            // Read all kerning pairs
            for (uint16_t i = 0; i < impl->kerning_count; ++i)
            {
                uint32_t first = stream_read_uint32(stream);
                uint32_t second = stream_read_uint32(stream);
                float amount = stream_read_float(stream);

                // Store in sparse representation
                if (first < MAX_GLYPHS && second < MAX_GLYPHS) {
                    uint32_t index = first * MAX_GLYPHS + second;
                    impl->kerning_index[index] = i;
                    impl->kerning_values[i] = amount;
                }
            }
        }
    }

    // Read atlas data
    uint32_t atlas_data_size = impl->atlas_width * impl->atlas_height; // R8 format
    uint8_t* atlas_data = (uint8_t*)malloc(atlas_data_size);
    if (!atlas_data)
    {
        object_free((font_t*)impl);
        return NULL;
    }

    stream_read_bytes(stream, atlas_data, atlas_data_size);
    object_free(stream);

    impl->texture = texture_alloc_raw(allocator, atlas_data, impl->atlas_width, impl->atlas_height, texture_format_r8, name);
    free(atlas_data);

    if (!impl->texture)
    {
        object_free((font_t*)impl);
        return NULL;
    }

    name_t material_name;
    name_t shader_name;
    name_set(&material_name, "font");
    name_set(&shader_name, "shaders/text");
    impl->material = material_alloc(allocator, shader_load(allocator, &shader_name), &material_name);
    if (!impl->material)
    {
        object_free((font_t*)impl);
        return NULL;
    }

    material_set_texture(impl->material, impl->texture, 0);

    return (font_t*)impl;
}

font_t* font_load(allocator_t* allocator, name_t* name)
{
    assert(g_device);
    assert(g_font_cache);
    assert(name);

    uint64_t key = hash_name(name);

    // Check if font exists in cache
    font_t* font = (font_t*)map_get(g_font_cache, key);
    if (font)
        return font;

    path_t font_path;
    asset_path(&font_path, name, "font");
    stream_t* stream = stream_load_from_file(NULL, &font_path);
    if (!stream)
        return NULL;

    font = font_load_from_stream(allocator, stream, name);

    if (font)
        map_set(g_font_cache, key, font);

    object_free(stream);

    return font;
}

const font_glyph_t* font_glyph(font_t* font, char ch)
{
    font_impl_t* impl = to_impl(font);
    
    // Check if glyph exists (advance > 0 means valid glyph)
    unsigned char index = (unsigned char)ch;
    if (index < MAX_GLYPHS && impl->glyphs[index].advance > 0.0f) 
    {
        return &impl->glyphs[index];
    }

    // If character not found, try unknown glyph (ASCII DEL character)
    unsigned char unknown_ch = 0x7F;
    if (unknown_ch < MAX_GLYPHS && impl->glyphs[unknown_ch].advance > 0.0f) 
    {
        return &impl->glyphs[unknown_ch];
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

float font_kerning(font_t* font, char first, char second)
{
	font_impl_t* impl = to_impl(font);
    
    unsigned char f = (unsigned char)first;
    unsigned char s = (unsigned char)second;
    if (f < MAX_GLYPHS && s < MAX_GLYPHS) 
    {
        uint32_t index = f * MAX_GLYPHS + s;
        uint16_t value_index = impl->kerning_index[index];
        if (value_index != 0xFFFF && value_index < impl->kerning_count) 
            return impl->kerning_values[value_index];
    }

    return 0.0f;
}

float font_baseline(font_t* font)
{
    return to_impl(font)->baseline;
}

material_t* font_material(font_t* font)
{
    return to_impl(font)->material;
}

void font_init(renderer_traits* traits, SDL_GPUDevice* device)
{
    g_font_cache = map_alloc(NULL, traits->max_fonts);
    g_device = device;
}

void font_uninit()
{
    object_free(g_font_cache);
    g_font_cache = NULL;
    g_device = NULL;
}
