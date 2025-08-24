//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
// 

#define INITIAL_CACHE_SIZE 32


typedef struct sampler_impl
{
    OBJECT_BASE;
    SDL_GPUSampler* sampler;
} sampler_impl_t;

static map_t* g_sampler_cache = NULL;
static SDL_GPUDevice* g_device = NULL;

static inline sampler_impl_t* to_impl(void* s) { return (sampler_impl_t*)to_object((object_t*)(s), type_sampler); }

static uint64_t sampler_options_hash(const sampler_options_t* options) 
{
    return hash_64((void*)options, sizeof(sampler_options_t));
}

SDL_GPUFilter to_sdl_filter(texture_filter filter);
SDL_GPUSamplerAddressMode to_sdl_clamp(texture_clamp clamp);

SDL_GPUSampler* sampler_factory_sampler(texture_t* texture)
{
    assert(g_device);
    assert(g_sampler_cache);
    assert(texture);

    sampler_options_t options = texture_sampler_options(texture);
    uint64_t key = sampler_options_hash(&options);

    sampler_impl_t* impl = (sampler_impl_t*)map_get(g_sampler_cache, key);
    if (impl)
        return impl->sampler;

    // Create new sampler
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = to_sdl_filter(options.min_filter);
    sampler_info.mag_filter = to_sdl_filter(options.mag_filter);
    sampler_info.mipmap_mode = (options.min_filter == texture_filter_nearest)
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
        return NULL;

    // Store in cache
    impl = to_impl(object_alloc(NULL, sizeof(sampler_impl_t), type_sampler));
    if (!impl)
        return NULL;

    impl->sampler = gpu_sampler;
    map_set(g_sampler_cache, key, impl);
    return impl->sampler;
}

SDL_GPUFilter to_sdl_filter(texture_filter filter)
{
    switch (filter)
    {
    case texture_filter_nearest:
        return SDL_GPU_FILTER_NEAREST;
    case texture_filter_linear:
        return SDL_GPU_FILTER_LINEAR;
    default:
        return SDL_GPU_FILTER_LINEAR;
    }
}

SDL_GPUSamplerAddressMode to_sdl_clamp(texture_clamp mode)
{
    switch (mode)
    {
    case texture_clamp_repeat:
        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case texture_clamp_clamp:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case texture_clamp_repeat_mirrored:
        return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    }
}

void sampler_factory_init(renderer_traits* traits, SDL_GPUDevice* dev)
{
    g_device = dev;
    g_sampler_cache = map_alloc(NULL, traits->max_samplers);
}

void sampler_factory_uninit()
{
    object_free(g_sampler_cache);
    g_sampler_cache = NULL;
    g_device = NULL;
}
