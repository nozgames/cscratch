//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void reset_state();
void update_back_buffer();
void init_gamma_pass();
void init_shadow_pass();
void render_gamma_pass();

typedef struct renderer_impl
{
    SDL_GPUDevice* device;
    SDL_Window* window;
    SDL_GPUCommandBuffer* command_buffer;
    SDL_GPURenderPass* render_pass;
    mat4_t view_projection;
    mat4_t view;

    // gamma
    //mesh gamma_mesh;
    //material gamma_material;

    // Depth buffer support
    SDL_GPUTexture* depth_texture;
    int depth_width;
    int depth_height;

    // MSAA support
    SDL_GPUTexture* msaa_color_texture;
    SDL_GPUTexture* msaa_depth_texture;

    // Default texture for rendering
    //texture default_texture;

    // Light view projection matrix for shadow mapping
    mat4_t light_view;

//    texture linear_back_buffer;
    SDL_GPUTexture* swap_chain_texture;
    SDL_GPUTexture* shadow_map;
    SDL_GPUSampler* shadow_sampler;
    //shader shadow_shader;
    bool shadow_pass;
    bool msaa;
    //render_buffer render_buffer;
    SDL_GPUGraphicsPipeline* pipeline;
} renderer_impl;

static renderer_impl g_renderer = {0};

void renderer_init(const renderer_traits* traits, SDL_Window* window)
{
    texture_init(traits);
    mesh_builder_init();

    g_renderer.window = window;
    //g_renderer.render_buffer = create_render_buffer();

    // Create GPU device
    g_renderer.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (!g_renderer.device)
        application_error(SDL_GetError());

    // Claim window for GPU device
    if (!SDL_ClaimWindowForGPUDevice(g_renderer.device, window))
    {
        SDL_DestroyGPUDevice(g_renderer.device);
        g_renderer.device = nullptr;
        application_error(SDL_GetError());
    }

    sampler_factory_init(g_renderer.device);
    pipeline_factory_init(window, g_renderer.device);

    // Create default texture
    //g_renderer.shadow_shader = load_shader("shaders/shadow");
    //g_renderer.default_texture = load_texture("white");

    init_gamma_pass();
    init_shadow_pass(traits);
}

void renderer_uninit()
{
    texture_uninit();

#if 0
    assert(g_renderer.device);

    g_renderer.gamma_mesh.reset();

    // Release depth textures first
    if (g_renderer.depth_texture)
    {
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.depth_texture);
        g_renderer.depth_texture = nullptr;
    }

    // Release MSAA textures
    if (g_renderer.msaa_color_texture)
    {
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_color_texture);
        g_renderer.msaa_color_texture = nullptr;
    }

    if (g_renderer.msaa_depth_texture)
    {
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_depth_texture);
        g_renderer.msaa_depth_texture = nullptr;
    }

    // Release shadow resources
    if (g_renderer.shadow_map)
    {
        SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.shadow_map);
        g_renderer.shadow_map = nullptr;
    }

    if (g_renderer.shadow_sampler)
    {
        SDL_ReleaseGPUSampler(g_renderer.device, g_renderer.shadow_sampler);
        g_renderer.shadow_sampler = nullptr;
    }

    // Release default texture
    g_renderer.default_texture.reset();

    unload_sampler_factory();
    unload_pipeline_factory();

    // Destroy GPU device last
    if (g_renderer.device)
        SDL_DestroyGPUDevice(g_renderer.device);

    g_renderer.device = nullptr;
    g_renderer.command_buffer = nullptr;
    g_renderer.render_pass = nullptr;
    g_renderer.shadow_pass = false;

    delete g_renderer;
    g_renderer = nullptr;
#endif

    memset(&g_renderer, 0, sizeof(renderer_impl));
}

void init_shadow_pass(const renderer_traits* traits)
{
    if (!traits->shadow_map_size)
        return;

    // Create shadow map using D32_FLOAT format for depth writing and sampling
    SDL_GPUTextureCreateInfo shadow_info = {0};
    shadow_info.type = SDL_GPU_TEXTURETYPE_2D;
    shadow_info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT; // Depth format for depth-stencil target
    shadow_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    shadow_info.width = traits->shadow_map_size;
    shadow_info.height = traits->shadow_map_size;
    shadow_info.layer_count_or_depth = 1;
    shadow_info.num_levels = 1;
    shadow_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    shadow_info.props = SDL_CreateProperties();
    SDL_SetStringProperty(shadow_info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "ShadowMap");
    g_renderer.shadow_map = SDL_CreateGPUTexture(g_renderer.device, &shadow_info);
    SDL_DestroyProperties(shadow_info.props);
    if (!g_renderer.shadow_map)
        application_error(SDL_GetError());

    // Create shadow sampler with depth comparison
    SDL_GPUSamplerCreateInfo shadow_sampler_info = {0};
    shadow_sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    shadow_sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    shadow_sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    shadow_sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadow_sampler_info.enable_compare = true;
    shadow_sampler_info.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    g_renderer.shadow_sampler = SDL_CreateGPUSampler(g_renderer.device, &shadow_sampler_info);
    SDL_DestroyProperties(shadow_info.props);
    if (!g_renderer.shadow_sampler)
        application_error(SDL_GetError());
}

#if 0
bool begin_frame_internal()
{
    update_back_buffer();

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(g_renderer.device);
    SDL_GPUTexture* backbuffer = nullptr;
    Uint32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd, g_renderer.window, &g_renderer.swap_chain_texture, &width, &height);
    if (!g_renderer.swap_chain_texture)
        return false;

    if (width == 0 || height == 0)
    {
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(g_renderer.window, &windowWidth, &windowHeight);

        if (windowWidth > 0 && windowHeight > 0)
        {
            width = (Uint32)windowWidth;
            height = (Uint32)windowHeight;
        }
        else
        {
            width = 800;
            height = 600;
        }
    }

    // Create depth texture if it doesn't exist or if size changed
    if (!g_renderer.depth_texture || g_renderer.depth_width != (int)width || g_renderer.depth_height != (int)height)
    {
        if (g_renderer.depth_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.depth_texture);
            g_renderer.depth_texture = nullptr;
        }

        // Create property group for D3D12 clear depth value to match our clear value
        SDL_PropertiesID depthProps = SDL_CreateProperties();
        SDL_SetFloatProperty(depthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

        SDL_GPUTextureCreateInfo depthInfo = {};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthInfo.width = width;
        depthInfo.height = height;
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;
        depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        depthInfo.props = depthProps; // Use the D3D12 properties

        g_renderer.depth_texture = SDL_CreateGPUTexture(g_renderer.device, &depthInfo);
        throw_if_null(g_renderer.depth_texture);

        SDL_DestroyProperties(depthProps);

        // Create MSAA textures
        if (g_renderer.msaa_color_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_color_texture);
            g_renderer.msaa_color_texture = nullptr;
        }
        if (g_renderer.msaa_depth_texture)
        {
            SDL_ReleaseGPUTexture(g_renderer.device, g_renderer.msaa_depth_texture);
            g_renderer.msaa_depth_texture = nullptr;
        }

        // Create MSAA color texture - use 16-bit float to match render target format
        SDL_GPUTextureCreateInfo msaaColorInfo = {};
        msaaColorInfo.type = SDL_GPU_TEXTURETYPE_2D;
        msaaColorInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        msaaColorInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        msaaColorInfo.width = width;
        msaaColorInfo.height = height;
        msaaColorInfo.layer_count_or_depth = 1;
        msaaColorInfo.num_levels = 1;
        msaaColorInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
        msaaColorInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(msaaColorInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "MSAAColor");

        g_renderer.msaa_color_texture = SDL_CreateGPUTexture(g_renderer.device, &msaaColorInfo);

        SDL_DestroyProperties(msaaColorInfo.props);

        // Create MSAA depth texture
        SDL_PropertiesID msaaDepthProps = SDL_CreateProperties();
        SDL_SetFloatProperty(msaaDepthProps, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, 1.0f);

        SDL_GPUTextureCreateInfo msaaDepthInfo = {};
        msaaDepthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        msaaDepthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        msaaDepthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        msaaDepthInfo.width = width;
        msaaDepthInfo.height = height;
        msaaDepthInfo.layer_count_or_depth = 1;
        msaaDepthInfo.num_levels = 1;
        msaaDepthInfo.sample_count = SDL_GPU_SAMPLECOUNT_4; // 4x MSAA
        msaaDepthInfo.props = msaaDepthProps;

        g_renderer.msaa_depth_texture = SDL_CreateGPUTexture(g_renderer.device, &msaaDepthInfo);

        g_renderer.depth_width = (int)width;
        g_renderer.depth_height = (int)height;
    }

    g_renderer.command_buffer = cmd;
    return true;
}

render_buffer begin_renderer_frame()
{
    assert(!g_renderer.command_buffer);

    if (!begin_frame_internal())
        return g_renderer.render_buffer;

    clear(g_renderer.render_buffer);

    return g_renderer.render_buffer;
}

void end_renderer_frame()
{
    assert(!g_renderer.render_pass);

    if (!g_renderer.command_buffer)
        return;

    render_gamma_pass();

    execute(g_renderer.render_buffer, g_renderer.command_buffer);
    SDL_SubmitGPUCommandBuffer(g_renderer.command_buffer);

    g_renderer.command_buffer = nullptr;
    g_renderer.render_pass = nullptr;
}

SDL_GPURenderPass* begin_renderer_pass(SDL_GPUTexture* target, bool clear, color clear_color)
{
    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = target;
    color_target.clear_color = to_sdl(clear_color);
    color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target = {};
    depth_target.texture = g_renderer.depth_texture;
    depth_target.clear_depth = 1.0f;
    depth_target.clear_stencil = 0;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op = SDL_GPU_STOREOP_STORE;

    g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, &color_target, 1, &depth_target);
    assert(g_renderer.render_pass);

    reset_state();

    return g_renderer.render_pass;
}

SDL_GPURenderPass* begin_renderer_pass(const texture& target, bool clear, color clear_color, bool msaa)
{
    assert(!g_renderer.render_pass);

    SDL_GPUColorTargetInfo color_target = {};
    SDL_GPUDepthStencilTargetInfo depth_target = {};

    // TODO: handle msaa to a target texture
    auto gpu_texture = target ? get_gpu_texture(target) : get_gpu_texture(g_renderer.linear_back_buffer);

    begin_renderer_pass(gpu_texture, clear, clear_color);
    return g_renderer.render_pass;

#if 0
        // Use MSAA textures for scene rendering with resolve to backbuffer
        if (false) // msaa && g_renderer.msaa_color_texture && g_renderer.msaa_depth_texture)
        {
            _msaa = true;
            color_target.texture = g_renderer.msaa_color_texture;
            color_target.clear_color = to_sdl(clear_color);
            color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            color_target.store_op = SDL_GPU_STOREOP_RESOLVE; // Resolve MSAA to backbuffer
            color_target.resolve_texture = gpu_texture; // Resolve target
            color_target.resolve_mip_level = 0;
            color_target.resolve_layer = 0;

            depth_target.texture = g_renderer.msaa_depth_texture;
            depth_target.clear_depth = 1.0f;
            depth_target.clear_stencil = 0;
            depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE; // Don't need to store MSAA depth
        }
        // Standard rendering without MSAA
        else
        {
            _msaa = false;
            color_target.texture = gpu_texture;
            color_target.clear_color = to_sdl(clear_color);
            color_target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            color_target.store_op = SDL_GPU_STOREOP_STORE;

            depth_target.texture = g_renderer.depth_texture;
            depth_target.clear_depth = 1.0f;
            depth_target.clear_stencil = 0;
            depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_target.store_op = SDL_GPU_STOREOP_STORE;
        }

        g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, &color_target, 1, &depth_target);
        assert(g_renderer.render_pass);

        reset_state();

        return g_renderer.render_pass;
#endif
}

void end_renderer_pass()
{
    assert(g_renderer.render_pass);

    SDL_EndGPURenderPass(g_renderer.render_pass);
    g_renderer.render_pass = nullptr;
    g_renderer.shadow_pass = false;
    g_renderer.msaa = false;
}

void bind_default_texture()
{
    assert(g_renderer);
    bind_texture(g_renderer.command_buffer, g_renderer.default_texture, static_cast<int>(sampler_register::user0));
}

void bind_texture(SDL_GPUCommandBuffer* cb, const texture& texture, int index)
{
    if (g_renderer.shadow_pass)
        return;

    // Get the actual texture to bind (use default if none provided)
    auto actual_texture = texture ? texture : g_renderer.default_texture;

    // Main pass: bind diffuse texture and shadow map
    SDL_GPUTextureSamplerBinding binding{};
    binding.sampler = get_sampler(actual_texture);
    binding.texture = get_gpu_texture(actual_texture);
    SDL_BindGPUFragmentSamplers(g_renderer.render_pass, index, &binding, 1);
}

void bind_shader(shader shader)
{
    assert(shader);

    auto* pipeline = get_pipeline(g_renderer.shadow_pass ? g_renderer.shadow_shader : shader, g_renderer.msaa,
                                  g_renderer.shadow_pass);
    if (!pipeline)
        return;

    // Only bind if pipeline changed
    if (g_renderer.pipeline == pipeline)
        return;

    SDL_BindGPUGraphicsPipeline(g_renderer.render_pass, pipeline);
    g_renderer.pipeline = pipeline;
}

void bind_renderer_material(const material& material)
{
    assert(material);

    auto shader = get_shader(material);
    assert(shader);

    bind_shader(shader);
    bind(g_renderer.command_buffer, material);
}

void bind_transform(const mat4& transform)
{
    SDL_PushGPUVertexUniformData(g_renderer.command_buffer, static_cast<uint32_t>(vertex_register::object), &transform,
                                 sizeof(mat4));
}

void bind_bones(const mat4* bones, int count)
{
    assert(bones);
    assert(count > 0);

    SDL_PushGPUVertexUniformData(g_renderer.command_buffer, static_cast<uint32_t>(vertex_register::bone), bones,
                                 static_cast<Uint32>(count * sizeof(mat4)));
}

SDL_GPURenderPass* begin_renderer_shadow_pass()
{
    assert(!g_renderer.render_pass);
    assert(g_renderer.command_buffer);

    // Start shadow pass using depth-only rendering
    SDL_GPUDepthStencilTargetInfo depth_info = {};
    depth_info.texture = g_renderer.shadow_map;
    depth_info.clear_depth = 1.0f;
    depth_info.clear_stencil = 0;
    depth_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_info.store_op = SDL_GPU_STOREOP_STORE;

    g_renderer.render_pass = SDL_BeginGPURenderPass(g_renderer.command_buffer, nullptr, 0, &depth_info);
    g_renderer.shadow_pass = true;
    reset_state();

    return g_renderer.render_pass;
}

void reset_state()
{
    // Reset all state tracking variables to force rebinding
    g_renderer.pipeline = nullptr;

    for (int i = 0; i < static_cast<int>(sampler_register::count); i++)
        bind_texture(g_renderer.command_buffer, g_renderer.default_texture, i);
}

void update_back_buffer()
{
    // is the back buffer the correct size?
    assert(g_renderer.device);
    auto screen_size = get_screen_size();
    if (g_renderer.linear_back_buffer && get_size(g_renderer.linear_back_buffer) == screen_size)
        return;

    g_renderer.linear_back_buffer =
        create_render_texture(screen_size.x, screen_size.y, texture_format::rgba16f, "linear");
}

void init_gamma_pass()
{
    mesh_builder builder;
    builder.add_vertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f));
    builder.add_vertex(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f));
    builder.add_vertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f));
    builder.add_vertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f));
    builder.add_triangle(0, 1, 2);
    builder.add_triangle(0, 2, 3);
    g_renderer.gamma_mesh = builder.to_mesh("gamma", true);
    g_renderer.gamma_material = create_material("shaders/gamma", "gamma");
}

SDL_GPURenderPass* begin_renderer_gamma_pass()
{
    return begin_renderer_pass(g_renderer.swap_chain_texture, false, color::transparent);
}

void render_gamma_pass()
{
    set_texture(g_renderer.gamma_material, g_renderer.linear_back_buffer, 0);

    static mat4 identity = glm::identity<mat4>();
    auto& rb = g_renderer.render_buffer;
    begin_gamma_pass(rb);
    bind_camera(rb, identity, identity);
    bind_transform(rb, identity);
    bind_material(rb, g_renderer.gamma_material);
    render_mesh(rb, g_renderer.gamma_mesh);
    end_render_pass(rb);
}

SDL_GPUDevice* get_gpu_device()
{
    return g_renderer.device;
}


#endif
