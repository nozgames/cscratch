//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
// 

#if 0
bool sampler_options::operator==(const sampler_options& other) const
{
    return min_filter == other.min_filter && mag_filter == other.mag_filter && clamp_u == other.clamp_u &&
           clamp_v == other.clamp_v && clamp_w == other.clamp_w && compare_op == other.compare_op;
}

struct sampler_options_hash
{
    std::size_t operator()(const sampler_options& options) const
    {
        std::size_t hash = 0;
        hash ^= std::hash<int>{}(static_cast<int>(options.min_filter)) << 0;
        hash ^= std::hash<int>{}(static_cast<int>(options.mag_filter)) << 4;
        hash ^= std::hash<int>{}(static_cast<int>(options.clamp_u)) << 8;
        hash ^= std::hash<int>{}(static_cast<int>(options.clamp_v)) << 12;
        hash ^= std::hash<int>{}(static_cast<int>(options.clamp_w)) << 16;
        hash ^= std::hash<int>{}(static_cast<int>(options.compare_op)) << 21;
        return hash;
    }
};
#endif

static SDL_GPUDevice* g_device;
static std::unordered_map<sampler_options, SDL_GPUSampler*, sampler_options_hash> g_samplers;

SDL_GPUFilter to_sdl(texture_filter filter);
SDL_GPUSamplerAddressMode to_sdl(texture_clamp clamp);

void sampler_factory_init(SDL_GPUDevice* device)
{
    g_device = device;
}

void unload_sampler_factory()
{
    if (!g_device)
        return;

    for (auto& pair : g_samplers)
    {
        if (pair.second)
        {
            SDL_ReleaseGPUSampler(g_device, pair.second);
        }
    }
    g_samplers.clear();

    g_device = nullptr;
}

SDL_GPUSampler* get_sampler(const texture& texture)
{
    auto options = get_sampler_options(texture);
    auto it = g_samplers.find(options);
    if (it != g_samplers.end())
        return it->second;

    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = to_sdl(options.min_filter);
    sampler_info.mag_filter = to_sdl(options.mag_filter);
    sampler_info.mipmap_mode = (options.min_filter == texture_filter::nearest) ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
                                                                               : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = to_sdl(options.clamp_u);
    sampler_info.address_mode_v = to_sdl(options.clamp_v);
    sampler_info.address_mode_w = to_sdl(options.clamp_w);
    sampler_info.enable_compare = sampler_info.compare_op != SDL_GPU_COMPAREOP_INVALID;
    sampler_info.compare_op = options.compare_op;
    sampler_info.props = 0;

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(g_device, &sampler_info);
    if (!sampler)
        throw std::runtime_error(SDL_GetError());

    g_samplers[options] = sampler;
    return sampler;
}

SDL_GPUFilter to_sdl(texture_filter filter)
{
    switch (filter)
    {
    case texture_filter::nearest:
        return SDL_GPU_FILTER_NEAREST;
    case texture_filter::linear:
        return SDL_GPU_FILTER_LINEAR;
    default:
        return SDL_GPU_FILTER_LINEAR;
    }
}

SDL_GPUSamplerAddressMode to_sdl(texture_clamp mode)
{
    switch (mode)
    {
    case texture_clamp::repeat:
        return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    case texture_clamp::clamp:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    case texture_clamp::repeat_mirrored:
        return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    default:
        return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    }
}
} // namespace noz