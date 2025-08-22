//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
// 

typedef struct texture* texture_t;

typedef struct sampler 
{
    SDL_GPUSampler* sampler;
} sampler_t;

static map_t g_sampler_cache = NULL;
static SDL_GPUDevice* g_device = NULL;
static object_type_t g_sampler_type = NULL;

#define INITIAL_CACHE_SIZE 32

static uint64_t sampler_options_hash(const sampler_options_t* options) 
{
    return hash_64(options, sizeof(sampler_options_t));
}

SDL_GPUFilter to_sdl_filter(texture_filter_t filter);
SDL_GPUSamplerAddressMode to_sdl_clamp(texture_clamp_t clamp);

SDL_GPUSampler* sampler_factory_sampler(texture_t texture)
{
    assert(g_device);
    assert(g_sampler_cache);
    assert(texture);

    sampler_options_t options = texture_sampler_options(texture);
    uint64_t key = sampler_options_hash(&options);

    // Check if sampler exists in cache
    object_t cached_obj = (object_t)map_get(g_sampler_cache, key);
    if (cached_obj) 
    {
        sampler_t* sampler_obj = (sampler_t*)object_impl(cached_obj, g_sampler_type);
        return sampler_obj->sampler;
    }

    // Create new sampler
    SDL_GPUSamplerCreateInfo sampler_info = {0};
    sampler_info.min_filter = to_sdl_filter(options.min_filter);
    sampler_info.mag_filter = to_sdl_filter(options.mag_filter);
    sampler_info.mipmap_mode = (options.min_filter == texture_filter_nearest) ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
                                                                              : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = to_sdl_clamp(options.clamp_u);
    sampler_info.address_mode_v = to_sdl_clamp(options.clamp_v);
    sampler_info.address_mode_w = to_sdl_clamp(options.clamp_w);
    sampler_info.enable_compare = sampler_info.compare_op != SDL_GPU_COMPAREOP_INVALID;
    sampler_info.compare_op = options.compare_op;
    sampler_info.props = 0;

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(g_device, &sampler_info);
    if (!sampler) 
    {
        // Handle error - in C we can't throw exceptions
        return NULL;
    }

    // Store in cache
    object_t cache_obj = (object_t)object_create(g_sampler_type, sizeof(sampler_t));
    if (cache_obj) map_set(g_sampler_cache, key, cache_obj);
    if (cache_obj) 
    {
        sampler_t* sampler_obj = (sampler_t*)object_impl(cache_obj, g_sampler_type);
        sampler_obj->sampler = sampler;
    }

    return sampler;
}

SDL_GPUFilter to_sdl_filter(texture_filter_t filter)
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

SDL_GPUSamplerAddressMode to_sdl_clamp(texture_clamp_t mode)
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

void sampler_factory_init(const renderer_traits* traits, SDL_GPUDevice* dev)
{
    g_device = dev;
    g_sampler_type = object_type_create("sampler");
    g_sampler_cache = map_create(traits->max_samplers);
}

void sampler_factory_uninit()
{
    object_destroy((object_t)g_sampler_cache);
    g_sampler_type = nullptr;
    g_sampler_cache = nullptr;
    g_device = nullptr;
}
