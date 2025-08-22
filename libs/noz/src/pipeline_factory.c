//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

//unordered_map<size_t, SDL_GPUGraphicsPipeline*> _cache;
SDL_GPUDevice* _device = nullptr;
SDL_Window* _window = nullptr;

void load_pipeline_factory(SDL_Window* window, SDL_GPUDevice* device)
{
    _window = window;
    _device = device;
    _cache.reserve(INITIAL_CACHE_SIZE);
}

void unload_pipeline_factory()
{
    _window = nullptr;
    _device = nullptr;

    for (auto& [key, pipeline] : _cache)
    {
        if (pipeline)
        {
            // Note: We need the GPU device to release pipelines
            // For now, just clear the cache entries
        }
    }
    _cache.clear();
}

static size_t pipeline_key(shader shader, bool msaa, bool shadow)
{
    hash<const void*> hasher;
    hash<bool> boolHasher;
    size_t h1 = hasher(shader.impl());
    size_t h2 = boolHasher(msaa);
    size_t h3 = boolHasher(shadow);
    return h1 ^ (h2 << 1) ^ (h2 << 2);
}

static Uint32 vertex_stride(const vector<SDL_GPUVertexAttribute>& attributes)
{
    if (attributes.empty())
        return 0;

    // Calculate stride based on the last attribute's offset + size
    const auto& last_attr = attributes.back();
    auto stride = last_attr.offset;

    // Add size based on attribute format
    switch (last_attr.format)
    {
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
        stride += 12; // 3 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
        stride += 8; // 2 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_INT4:
        stride += 16; // 4 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
        stride += 16; // 4 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_INT:
        stride += 4; // 1 * 4 bytes
        break;
    case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
        stride += 4; // 1 * 4 bytes
        break;
    default:
        stride += 4; // Default to 4 bytes
        break;
    }

    return stride;
}

static SDL_GPUGraphicsPipeline* create_pipeline(const shader& shader, const vector<SDL_GPUVertexAttribute>& attributes,
                                                bool msaa, bool shadow)
{
    assert(_window);
    assert(_device);
    assert(shader);

    // Create pipeline directly using the shader's compiled shaders
    auto vertexStride = vertex_stride(attributes);

    SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
    vertex_buffer_desc.pitch = vertexStride;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexInputState vertex_input_state = {};
    vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    vertex_input_state.num_vertex_buffers = 1;
    vertex_input_state.vertex_attributes = attributes.data();
    vertex_input_state.num_vertex_attributes = static_cast<Uint32>(attributes.size());

    SDL_GPUColorTargetDescription color_target = {};
    if (!shadow)
        color_target.format = SDL_GetGPUSwapchainTextureFormat(_device, _window);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vertex_shader = get_gpu_vertex_shader(shader);
    pipeline_create_info.fragment_shader = get_gpu_fragment_shader(shader);
    pipeline_create_info.vertex_input_state = vertex_input_state;
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_create_info.props = SDL_CreateProperties();

    auto name = get_name(shader);
    if (msaa)
        name += "_msaa";
    if (shadow)
        name += "_shadow";

    SDL_SetStringProperty(pipeline_create_info.props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, name.c_str());

    // Set rasterizer state based on shader properties
    pipeline_create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_create_info.rasterizer_state.cull_mode = get_gpu_cull_mode(shader);
    pipeline_create_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    // pipeline_create.rasterizer_state.enable_depth_clip = false;
    // pipeline_create.rasterizer_state.enable_depth_bias = false;
    // pipeline_create.rasterizer_state.depth_bias_constant_factor = 0.0f;
    // pipeline_create.rasterizer_state.depth_bias_clamp = 0.0f;
    // pipeline_create.rasterizer_state.depth_bias_slope_factor = 0.0f;

    // Set multisample state based on current render target
    // Check if MSAA is active in the renderer
    pipeline_create_info.multisample_state.sample_count = msaa ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_1;
    pipeline_create_info.multisample_state.sample_mask = 0;
    pipeline_create_info.multisample_state.enable_mask = false;

    // Get pipeline properties from shader
    bool depth_test = is_depth_test_enabled(shader);
    bool depth_write = is_depth_write_enabled(shader);

    pipeline_create_info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    pipeline_create_info.depth_stencil_state.back_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.back_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    pipeline_create_info.depth_stencil_state.front_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    pipeline_create_info.depth_stencil_state.front_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    pipeline_create_info.depth_stencil_state.compare_mask = 0;
    pipeline_create_info.depth_stencil_state.write_mask = 0;
    pipeline_create_info.depth_stencil_state.enable_depth_test = depth_test;
    pipeline_create_info.depth_stencil_state.enable_depth_write = depth_write;
    pipeline_create_info.depth_stencil_state.enable_stencil_test = false;

    // Configure color targets and blend state (only for non-shadow shaders)
    if (!shadow)
    {
        SDL_GPUColorTargetBlendState blend_state = {};
        blend_state.enable_blend = is_blend_enabled(shader);
        if (blend_state.enable_blend)
        {
            auto src_blend = get_gpu_src_blend(shader);
            auto dst_blend = get_gpu_dst_blend(shader);
            blend_state.enable_blend = true;
            blend_state.src_color_blendfactor = src_blend;
            blend_state.dst_color_blendfactor = dst_blend;
            blend_state.src_alpha_blendfactor = src_blend;
            blend_state.dst_alpha_blendfactor = dst_blend;
            blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        }

        // Update color target description with blend state
        color_target.blend_state = blend_state;

        pipeline_create_info.target_info.num_color_targets = 1;
        pipeline_create_info.target_info.color_target_descriptions = &color_target;
    }
    else
    {
        // Shadow shaders are depth-only, no color targets
        pipeline_create_info.target_info.num_color_targets = 0;
        pipeline_create_info.target_info.color_target_descriptions = nullptr;
    }

    // Set depth stencil target info to match the depth texture
    pipeline_create_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pipeline_create_info.target_info.has_depth_stencil_target = true;

    auto pipeline = SDL_CreateGPUGraphicsPipeline(_device, &pipeline_create_info);
    SDL_DestroyProperties(pipeline_create_info.props);
    if (!pipeline)
        throw std::runtime_error(SDL_GetError());

    return pipeline;
}

SDL_GPUGraphicsPipeline* get_pipeline(const shader& shader, bool msaa, bool shadow)
{
    assert(_window);
    assert(_device);
    assert(shader);

    auto key = pipeline_key(shader, msaa, shadow);

    auto it = _cache.find(key);
    if (it != _cache.end())
        return it->second;

    // Create new pipeline
    std::vector<SDL_GPUVertexAttribute> attributes = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},                 // position : POSITION (semantic 0)
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3}, // uv0 : TEXCOORD1 (semantic 2)
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 5}, // normal : TEXCOORD2 (semantic 1)
        {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(float) * 8}   // bone_index : TEXCOORD3 (semantic 3)
    };

    _cache.clear();

    auto* pipeline = create_pipeline(shader, attributes, msaa, shadow);
    assert(pipeline);

    _cache[key] = pipeline;

    return pipeline;
}
