//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define INITIAL_CACHE_SIZE 64

struct TextureImpl
{
    OBJECT_BASE;
    name_t name;
    SDL_GPUTexture* handle;
    sampler_options_t sampler_options;
    ivec2 size;
};

static SDL_GPUDevice* g_device = nullptr;
static Map* g_texture_cache = nullptr;

static void AllocTexture(
    TextureImpl* impl,
    void* data,
    size_t width,
    size_t height,
    int channels,
    bool generate_mipmaps);
static void LoadTexture(Allocator* allocator, TextureImpl* impl);
static void texture_destroy_impl(TextureImpl* impl);
int GetBytesPerPixel(TextureFormat format);

static TextureImpl* Impl(Texture* t)
{
    return (TextureImpl*)to_object((Object*)t, type_texture);
}

SDL_GPUTextureFormat ToSDL(const TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_RGBA8:
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    case TEXTURE_FORMAT_RGBA16F:
        return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    case TEXTURE_FORMAT_R8:
        return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    default:
        return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }
}
Texture* LoadTexture(Allocator* allocator, const name_t* name)
{
    assert(g_device);
    assert(g_texture_cache);
    assert(name);

    const u64 key = Hash(name);

    // cached?
    auto* texture = (Texture*)MapGet(g_texture_cache, key);
    if (texture)
        return texture;

    // Create new texture
    texture = (Texture*)Alloc(allocator, sizeof(TextureImpl), type_texture);
    if (!texture)
        return nullptr;

    TextureImpl* impl = Impl(texture);
    MapSet(g_texture_cache, key, texture);

    impl->handle = nullptr;
    impl->size.x = 0;
    impl->size.y = 0;
    impl->sampler_options.min_filter = TEXTURE_FILTER_LINEAR;
    impl->sampler_options.mag_filter = TEXTURE_FILTER_LINEAR;
    impl->sampler_options.clamp_u = TEXTURE_CLAMP_CLAMP;
    impl->sampler_options.clamp_v = TEXTURE_CLAMP_CLAMP;
    impl->sampler_options.clamp_w = TEXTURE_CLAMP_CLAMP;
    impl->sampler_options.compare_op = SDL_GPU_COMPAREOP_INVALID;
    CopyName(&impl->name, name);

    // Handle special "white" texture
    if (name_eq_cstr(name, "white"))
    {
        uint8_t white_pixel[4] = {255, 255, 255, 255};
        AllocTexture(impl, white_pixel, 1, 1, 4, false);
        return (Texture*)impl;
    }

    LoadTexture(allocator, impl);
    return (Texture*)impl;
}

Texture* AllocTexture(Allocator* allocator, int width, int height, TextureFormat format, const name_t* name)
{
    assert(width > 0);
    assert(height > 0);
    assert(name);
    assert(g_device);

    auto* texture = (Texture*)Alloc(allocator, sizeof(TextureImpl), type_texture);
    if (!texture)
        return nullptr;

    auto impl = Impl(texture);
    impl->size.x = width;
    impl->size.y = height;
    CopyName(&impl->name, name);

    SDL_GPUTextureCreateInfo texture_info = {};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = ToSDL(format);
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = width;
    texture_info.height = height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name->value);
    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);

    return texture;
}

Texture* AllocTexture(
    Allocator* allocator,
    void* data,
    size_t width,
    size_t height,
    TextureFormat format,
    const name_t* name)
{
    assert(data);
    assert(name);

    auto* texture = (Texture*)Alloc(allocator, sizeof(TextureImpl), type_texture);
    if (!texture)
        return nullptr;

    AllocTexture(Impl(texture), data, width, height, GetBytesPerPixel(format), false);
    return texture;
}

#if 0
static void texture_destroy_impl(TextureImpl* impl)
{
    assert(impl);
    
    if (impl->handle)
		SDL_ReleaseGPUTexture(g_device, impl->handle);
}
#endif

ivec2 GetSize(Texture* texture)
{
    return Impl(texture)->size;
}

int GetWidth(Texture* texture)
{
    return Impl(texture)->size.x;
}

int GetHeight(Texture* texture)
{
    return Impl(texture)->size.y;
}

SDL_GPUTexture* GetGPUTexture(Texture* texture)
{
    return Impl(texture)->handle;
}

sampler_options_t GetSamplerOptions(Texture* texture)
{
    static sampler_options_t default_options = {TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_LINEAR,
                                                TEXTURE_CLAMP_CLAMP,   TEXTURE_CLAMP_CLAMP,
                                                TEXTURE_CLAMP_CLAMP,   SDL_GPU_COMPAREOP_INVALID};

    if (!texture)
        return default_options;
    return Impl(texture)->sampler_options;
}

static void AllocTexture(TextureImpl* impl, void* data, size_t width, size_t height, int channels,
                                            bool generate_mipmaps)
{
    assert(impl);
    assert(data);
    assert(width > 0);
    assert(height > 0);
    assert(channels > 0);
    assert(g_device);

    // Handle different channel formats
    uint8_t* rgba_data = nullptr;
    bool allocated_rgba = false;

    if (channels == 1)
    {
        // Single channel - use as-is for R8_UNORM
        // No conversion needed
    }
    // Convert RGB to RGBA
    else if (channels == 3)
    {
        const uint8_t* rgb_src = (const uint8_t*)data;
        // todo: use allocator
        rgba_data = (uint8_t*)malloc(width * height * 4);
        if (!rgba_data)
            return;

        allocated_rgba = true;

        for (size_t i = 0; i < width * height; ++i)
        {
            rgba_data[i * 4 + 0] = rgb_src[i * 3 + 0]; // R
            rgba_data[i * 4 + 1] = rgb_src[i * 3 + 1]; // G
            rgba_data[i * 4 + 2] = rgb_src[i * 3 + 2]; // B
            rgba_data[i * 4 + 3] = 255;                // A
        }
        data = rgba_data;
        channels = 4;
    }
    else if (channels != 4)
    {
        return;
    }

    // Create transfer buffer for pixel data
    const size_t pitch = width * channels;
    const size_t size = pitch * height;

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = (Uint32)size;
    transfer_info.props = 0;
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(g_device, &transfer_info);
    if (!transfer_buffer)
    {
        if (allocated_rgba)
            free(rgba_data);
        return;
    }

    // Map transfer buffer and copy pixel data
    void* mapped = SDL_MapGPUTransferBuffer(g_device, transfer_buffer, false);
    if (!mapped)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
        return;
    }
    SDL_memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(g_device, transfer_buffer);

    // Calculate number of mipmap levels if requested
    uint32_t num_levels = 1;
    if (generate_mipmaps)
    {
        // Calculate maximum number of mipmap levels
        size_t max_dim = (width > height) ? width : height;
        num_levels = 1 + (u32)floor(log2((double)max_dim));
    }

    // Create GPU texture with appropriate format based on channel count
    SDL_GPUTextureCreateInfo texture_info = {};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = (channels == 1) ? SDL_GPU_TEXTUREFORMAT_R8_UNORM : SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = (int)width;
    texture_info.height = (int)height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = num_levels;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(texture_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, impl->name.value);

    impl->handle = SDL_CreateGPUTexture(g_device, &texture_info);
    SDL_DestroyProperties(texture_info.props);
    if (!impl->handle)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
        return;
    }

    SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(g_device);
    if (!cb)
    {
        SDL_ReleaseGPUTransferBuffer(g_device, transfer_buffer);
        if (allocated_rgba)
            free(rgba_data);
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

    impl->size.x = (int)width;
    impl->size.y = (int)height;

    if (allocated_rgba)
        free(rgba_data);
}

static void LoadTexture(Allocator* allocator, TextureImpl* impl)
{
    path_t texture_path;
    SetAssetPath(&texture_path, &impl->name, "texture");
    stream_t* stream = LoadStream(allocator, &texture_path);
    if (!stream)
        return;

    // Validate file signature
    if (!stream_read_signature(stream, "NZXT", 4))
    {
        Free(stream);
        return;
    }

    // Read version
    uint32_t version = stream_read_uint32(stream);
    if (version != 1)
    {
        Free(stream);
        return;
    }

    // Read texture data
    uint32_t format = stream_read_uint32(stream);
    uint32_t width = stream_read_uint32(stream);
    uint32_t height = stream_read_uint32(stream);

    // Validate format
    if (format > 1)
    {
        Free(stream);
        return;
    }

    // Read sampler options
    impl->sampler_options.min_filter = (TextureFilter)stream_read_uint8(stream);
    impl->sampler_options.mag_filter = (TextureFilter)stream_read_uint8(stream);
    impl->sampler_options.clamp_u = (TextureClamp)stream_read_uint8(stream);
    impl->sampler_options.clamp_v = (TextureClamp)stream_read_uint8(stream);
    impl->sampler_options.clamp_w = (TextureClamp)stream_read_uint8(stream);
    bool mips = stream_read_bool(stream);

    if (mips)
    {
        // Read number of mip levels
        uint32_t num_mip_levels = stream_read_uint32(stream);

        // For now, just read the base level and let GPU handle mipmaps
        // TODO: Upload all mip levels to GPU
        for (uint32_t level = 0; level < num_mip_levels; ++level)
        {
            /*uint32_t mip_width =*/stream_read_uint32(stream);
            /*uint32_t mip_height = */ stream_read_uint32(stream);
            uint32_t mip_data_size = stream_read_uint32(stream);

            if (level == 0)
            {
                // todo: use allocator
                u8* mip_data = (u8*)malloc(mip_data_size);
                if (mip_data)
                {
                    stream_read_bytes(stream, mip_data, mip_data_size);
                    AllocTexture(impl, mip_data, width, height, (format == 1) ? 4 : 3, true);
                    free(mip_data);
                }
                else
                {
                    // Skip data if allocation failed
                    stream_set_position(stream, stream_position(stream) + mip_data_size);
                }
            }
            else
            {
                // Skip other mip levels for now
                stream_set_position(stream, stream_position(stream) + mip_data_size);
            }
        }
    }
    else
    {
        const int channels = (format == 1) ? 4 : 3;
        const size_t data_size = width * height * channels;
        if (const auto texture_data = (u8*)malloc(data_size))
        {
            stream_read_bytes(stream, texture_data, data_size);
            AllocTexture(impl, texture_data, width, height, channels, false);
            free(texture_data);
        }
    }

    Free(stream);
}

int GetBytesPerPixel(TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_RGBA8:
        return 4;
    case TEXTURE_FORMAT_RGBA16F:
        return 8;
    case TEXTURE_FORMAT_R8:
        return 1;
    default:
        return 4; // Default to RGBA
    }
}

void InitTexture(RendererTraits* traits, SDL_GPUDevice* dev)
{
    g_device = dev;
    g_texture_cache = AllocMap(nullptr, traits->max_textures);
}

void ShutdownTexture()
{
    g_texture_cache = nullptr;
    g_device = nullptr;
}
