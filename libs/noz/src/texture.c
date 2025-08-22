//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct texture_impl 
{
    name_t name;
    SDL_GPUTexture* handle;
    sampler_options_t sampler_options;
    ivec2_t size;
} texture_impl_t;

static map_t g_texture_cache = NULL;

SDL_GPUTextureFormat texture_format_to_sdl(texture_format_t format)
{
    switch (format) {
        case texture_format_rgba8:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        case texture_format_rgba16f:
            return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        case texture_format_r8:
            return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
        default:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }
}
static SDL_GPUDevice* g_device = NULL;
static object_type_t g_texture_type = NULL;

#define INITIAL_CACHE_SIZE 64

// Forward declarations
static void texture_create_from_memory_impl(texture_impl_t* impl, const void* data, size_t width, size_t height, int channels, bool generate_mipmaps);
static void texture_load_impl(texture_impl_t* impl);
static void texture_destroy_impl(texture_impl_t* impl);

static inline texture_impl_t* to_impl(texture_t texture)
{
    assert(texture);
    return (texture_impl_t*)object_impl((object_t)texture, g_texture_type);
}

texture_t texture_load(const char* name)
{
    assert(g_device);
    assert(g_texture_cache);
    assert(name);

    uint64_t key = hash_string(name);

    // Check if texture exists in cache
    object_t cached_obj = (object_t)map_get(g_texture_cache, key);
    if (cached_obj) 
    {
        return (texture_t)cached_obj;
    }

    // Create new texture
    object_t cache_obj = (object_t)object_create(g_texture_type, sizeof(texture_impl_t));
    if (cache_obj) map_set(g_texture_cache, key, cache_obj);
    if (!cache_obj) 
    {
        return NULL;
    }
    
    texture_impl_t* impl = (texture_impl_t*)object_impl(cache_obj, g_texture_type);
    name_set(&impl->name, name);    
    impl->handle = NULL;
    impl->size.x = 0;
    impl->size.y = 0;
    impl->sampler_options.min_filter = texture_filter_linear;
    impl->sampler_options.mag_filter = texture_filter_linear;
    impl->sampler_options.clamp_u = texture_clamp_clamp;
    impl->sampler_options.clamp_v = texture_clamp_clamp;
    impl->sampler_options.clamp_w = texture_clamp_clamp;
    impl->sampler_options.compare_op = SDL_GPU_COMPAREOP_INVALID;

    // Handle special "white" texture
    if (strcmp(name, "white") == 0)
    {
        uint8_t white_pixel[4] = {255, 255, 255, 255};
        texture_create_from_memory_impl(impl, white_pixel, 1, 1, 4, false);
        return (texture_t)cache_obj;
    }

    texture_load_impl(impl);
    return (texture_t)cache_obj;
}

texture_t texture_create_render_target(int width, int height, texture_format_t format, const char* name)
{
    assert(width > 0);
    assert(height > 0);
    assert(g_device);
       
    texture_t texture = (texture_t)object_create(g_texture_type, sizeof(texture_impl_t));
    if (!texture)
        return nullptr;
    
    texture_impl_t* impl = (texture_impl_t*)to_impl(texture);
    impl->size.x = width;
    impl->size.y = height;

    if (name)
        name_set(&impl->name, name);
    else
        name_format(&impl->name, "render_%s_%dx%d", name, width, height);

    SDL_GPUTextureCreateInfo texture_info = {0};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = texture_format_to_sdl(format);
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name);
    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);

    return (texture_t)texture;
}

texture_t texture_create_raw(const uint8_t* data, size_t width, size_t height, texture_format_t format, const char* name)
{
    assert(data);
    
    // Create texture with unique name
    texture_t texture = (texture_t)object_create(g_texture_type, sizeof(texture_impl_t));
    if (!texture)
        return nullptr;
    
	texture_impl_t* impl = (texture_impl_t*)to_impl(texture);
    if (name)
        name_set(&impl->name, name);
    else
        name_format(&impl->name, "data_%s_%zux%zu", name, width, height);
        
    texture_create_from_memory_impl(impl, data, width, height, texture_format_bytes_per_pixel(format), false);
    return (texture_t)texture;
}

static void texture_destroy_impl(texture_impl_t* impl)
{
    assert(impl);
    
    if (impl->handle)
		SDL_ReleaseGPUTexture(g_device, impl->handle);
}

ivec2_t texture_size(texture_t texture)
{
    return to_impl(texture)->size;
}

int texture_width(texture_t texture)
{
    return to_impl(texture)->size.x;
}

int texture_height(texture_t texture)
{
    return to_impl(texture)->size.y;
}

SDL_GPUTexture* texture_gpu_handle(texture_t texture)
{
    return to_impl(texture)->handle;
}

sampler_options_t texture_sampler_options(texture_t texture)
{
    sampler_options_t default_options = {
        texture_filter_linear,
        texture_filter_linear,
        texture_clamp_clamp,
        texture_clamp_clamp,
        texture_clamp_clamp,
        SDL_GPU_COMPAREOP_INVALID
    };
    
    if (!texture) return default_options;
    
    texture_impl_t* impl = (texture_impl_t*)object_impl((object_t)texture, g_texture_type);
    return impl->sampler_options;
}

static void texture_create_from_memory_impl(texture_impl_t* impl, const void* data, size_t width, size_t height, int channels, bool generate_mipmaps)
{
    assert(impl);
    assert(data);
    assert(width > 0);
    assert(height > 0);
    assert(channels > 0);
    assert(g_device);

    // Handle different channel formats
    uint8_t* rgba_data = NULL;
    bool allocated_rgba = false;
    
    if (channels == 1)
    {
        // Single channel - use as-is for R8_UNORM
        // No conversion needed
    }
    // Convert RGB to RGBA
    else if (channels == 3)
    {
        const uint8_t* rgb_data = (const uint8_t*)data;
        rgba_data = malloc(width * height * 4);
        if (!rgba_data) return;
        allocated_rgba = true;

        for (size_t i = 0; i < width * height; ++i)
        {
            rgba_data[i * 4 + 0] = rgb_data[i * 3 + 0]; // R
            rgba_data[i * 4 + 1] = rgb_data[i * 3 + 1]; // G
            rgba_data[i * 4 + 2] = rgb_data[i * 3 + 2]; // B
            rgba_data[i * 4 + 3] = 255;                 // A
        }
        data = rgba_data;
        channels = 4;
    }
    else if (channels != 4)
    {
        return;
    }

    // Create transfer buffer for pixel data
    const int pitch = width * channels;
    const int size = pitch * height;

    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = size;
    transfer_info.props = 0;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(g_device, &transfer_info);
    if (!transfer_buffer)
    {
        if (allocated_rgba) free(rgba_data);
        return;
    }

    // Map transfer buffer and copy pixel data
    void* mapped = SDL_MapGPUTransferBuffer(g_device, transfer_buffer, false);
    if (!mapped)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba) free(rgba_data);
        return;
    }
    SDL_memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(g_device, transfer_buffer);

    // Calculate number of mipmap levels if requested
    uint32_t num_levels = 1;
    if (generate_mipmaps)
    {
        // Calculate maximum number of mipmap levels
        int max_dim = (width > height) ? width : height;
        num_levels = 1 + (uint32_t)floor(log2((double)max_dim));
    }

    // Create GPU texture with appropriate format based on channel count
    SDL_GPUTextureCreateInfo texture_info = {0};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = (channels == 1) ? SDL_GPU_TEXTUREFORMAT_R8_UNORM : SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = num_levels;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, impl->name.data);

    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);
    if (!impl->handle)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba) free(rgba_data);
        return;
    }

    SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(g_device);
    if (!cb)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba) free(rgba_data);
        return;
    }

    // Upload pixel data to GPU texture
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cb);
    SDL_GPUTextureTransferInfo source = {transfer_buffer, 0, (uint32_t)width, (uint32_t)height};
    SDL_GPUTextureRegion destination = {0};
    destination.texture = impl->handle;
    destination.w = (uint32_t)width;
    destination.h = (uint32_t)height;
    destination.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cb);
    SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);

    impl->size.x = width;
    impl->size.y = height;
    
    if (allocated_rgba) free(rgba_data);
}

static void texture_load_impl(texture_impl_t* impl)
{
    const char* texture_path = asset_path(impl->name.data, "texture");
    stream_t reader = stream_create_from_file(texture_path);
    if (!reader) 
        return;

    // Validate file signature
    if (!stream_read_signature(reader, "NZXT", 4))
    {
        stream_destroy(reader);
        return;
    }

    // Read version
    uint32_t version = stream_read_uint32(reader);
    if (version != 1)
    {
        stream_destroy(reader);
        return;
    }

    // Read texture data
    uint32_t format = stream_read_uint32(reader);
    uint32_t width = stream_read_uint32(reader);
    uint32_t height = stream_read_uint32(reader);

    // Validate format
    if (format > 1)
    {
        stream_destroy(reader);
        return;
    }

    // Read sampler options
    impl->sampler_options.min_filter = (texture_filter_t)stream_read_uint8(reader);
    impl->sampler_options.mag_filter = (texture_filter_t)stream_read_uint8(reader);
    impl->sampler_options.clamp_u = (texture_clamp_t)stream_read_uint8(reader);
    impl->sampler_options.clamp_v = (texture_clamp_t)stream_read_uint8(reader);
    impl->sampler_options.clamp_w = (texture_clamp_t)stream_read_uint8(reader);
    bool mips = stream_read_bool(reader);

    if (mips)
    {
        // Read number of mip levels
        uint32_t num_mip_levels = stream_read_uint32(reader);

        // For now, just read the base level and let GPU handle mipmaps
        // TODO: Upload all mip levels to GPU
        for (uint32_t level = 0; level < num_mip_levels; ++level)
        {
            uint32_t mip_width = stream_read_uint32(reader);
            uint32_t mip_height = stream_read_uint32(reader);
            uint32_t mip_data_size = stream_read_uint32(reader);

            if (level == 0)
            {
                uint8_t* mip_data = malloc(mip_data_size);
                if (mip_data) 
                {
                    stream_read_bytes(reader, mip_data, mip_data_size);
                    texture_create_from_memory_impl(impl, mip_data, width, height, (format == 1) ? 4 : 3, true);
                    free(mip_data);
                }
                else 
                {
                    // Skip data if allocation failed
                    stream_set_position(reader, stream_position(reader) + mip_data_size);
                }
            }
            else
            {
                // Skip other mip levels for now
                stream_set_position(reader, stream_position(reader) + mip_data_size);
            }
        }
    }
    else
    {
        int channels = (format == 1) ? 4 : 3;
        size_t data_size = width * height * channels;
        uint8_t* texture_data = malloc(data_size);
        if (texture_data) 
        {
            stream_read_bytes(reader, texture_data, data_size);
            texture_create_from_memory_impl(impl, texture_data, width, height, channels, false);
            free(texture_data);
        }
    }
    
    stream_destroy(reader);
}

int texture_format_bytes_per_pixel(texture_format_t format)
{
    switch (format)
    {
    case texture_format_rgba8:
        return 4;
    case texture_format_rgba16f:
        return 8;
    case texture_format_r8:
        return 1;
    default:
        return 4; // Default to RGBA
    }
}

SDL_GPUTexture* texture_gpu_texture(texture_t texture)
{
    return texture_gpu_handle(texture);
}

int texture_bytes_per_pixel(texture_format_t format)
{
    return texture_format_bytes_per_pixel(format);
}

void texture_init(const renderer_traits* traits, SDL_GPUDevice* dev)
{
    g_device = dev;
    g_texture_type = object_type_create("texture");
    g_texture_cache = map_create(traits->max_textures);
}

void texture_uninit()
{
    object_destroy((object_t)g_texture_cache);
    g_texture_cache = NULL;
    g_device = NULL;
}
