//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
// 

#define INITIAL_CACHE_SIZE 32


struct Sampler
{
    SDL_GPUSampler* gpu_sampler;
};

static Map* g_sampler_cache = nullptr;
static SDL_GPUDevice* g_device = nullptr;

static u64 Hash(const SamplerOptions* options)
{
    return Hash((void*)options, sizeof(SamplerOptions));
}

SDL_GPUFilter to_sdl_filter(TextureFilter filter);
SDL_GPUSamplerAddressMode to_sdl_clamp(TextureClamp clamp);

SDL_GPUSampler* GetGPUSampler(Texture* texture)
{
    assert(g_device);
    assert(g_sampler_cache);
    assert(texture);

    SamplerOptions options = GetSamplerOptions(texture);
    u64 key = Hash(&options);

    auto* sampler = (Sampler*)GetValue(g_sampler_cache, key);
    if (sampler)
        return sampler->gpu_sampler;

    // Create new sampler
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = to_sdl_filter(options.min_filter);
    sampler_info.mag_filter = to_sdl_filter(options.mag_filter);
    sampler_info.mipmap_mode = (options.min_filter == TEXTURE_FILTER_NEAREST)
        ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
        : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = to_sdl_clamp(options.clamp_u);
    sampler_info.address_mode_v = to_sdl_clamp(options.clamp_v);
    sampler_info.address_mode_w = to_sdl_clamp(options.clamp_w);
    sampler_info.enable_compare = sampler_info.compare_op != SDL_GPU_COMPAREOP_INVALID;
    sampler_info.compare_op = options.compare_op;
    sampler_info.props = 0;

    SDL_GPUSampler* gpu_sampler = SDL_CreateGPUSampler(g_device, &sampler_info);
    if (!gpu_sampler)
        return nullptr;

    // Store in cache
    sampler = (Sampler*)Alloc(nullptr, sizeof(Sampler));
    if (!sampler)
        return nullptr;

    sampler->gpu_sampler = gpu_sampler;
    SetValue(g_sampler_cache, key, sampler);
    return sampler->gpu_sampler;
}

SDL_GPUFilter to_sdl_filter(TextureFilter filter)
{
    switch (filter)
    {
    case TEXTURE_FILTER_NEAREST:
        return SDL_GPU_FILTER_NEAREST;
    default:
    case TEXTURE_FILTER_LINEAR:
        return SDL_GPU_FILTER_LINEAR;
    }
}

SDL_GPUSamplerAddressMode to_sdl_clamp(TextureClamp mode)
{
    switch (mode)
    {
    case TEXTURE_CLAMP_REPEAT:
        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case TEXTURE_CLAMP_CLAMP:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case TEXTURE_CLAMP_REPEAT_MIRRORED:
        return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    }
}

void InitSamplerFactory(RendererTraits* traits, SDL_GPUDevice* dev)
{
    assert(!g_sampler_cache);
    g_device = dev;
    g_sampler_cache = CreateMap(nullptr, traits->max_samplers);
}

void ShutdownSamplerFactory()
{
    assert(g_sampler_cache);
    //Enumerate(g_sampler_cache,

    Destroy(g_sampler_cache);
    g_sampler_cache = nullptr;
    g_device = nullptr;
}
