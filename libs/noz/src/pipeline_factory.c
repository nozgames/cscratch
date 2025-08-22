//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct shader* shader_t;

static map_t g_pipeline_cache = NULL;
static SDL_GPUDevice* g_device = NULL;
static SDL_Window* g_window = NULL;
static object_type_t g_pipeline_factory_type = NULL;

#define INITIAL_CACHE_SIZE 64

typedef struct pipeline_object 
{
    SDL_GPUGraphicsPipeline* pipeline;
} pipeline_object_t;

static uint64_t pipeline_key(shader_t shader, bool msaa, bool shadow) 
{
    struct {
        void* shader_ptr;
        bool msaa;
        bool shadow;
    } key_data = {shader, msaa, shadow};
    
    return hash_64(&key_data, sizeof(key_data));
}

static uint32_t vertex_stride(const SDL_GPUVertexAttribute* attributes, size_t attribute_count) 
{
    if (attribute_count == 0) 
    {
        return 0;
    }

    // Calculate stride based on the last attribute's offset + size
    const SDL_GPUVertexAttribute* last_attr = &attributes[attribute_count - 1];
    uint32_t stride = last_attr->offset;

    // Add size based on attribute format
    switch (last_attr->format) 
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

// TODO: These shader functions need to be implemented
extern SDL_GPUShader* get_gpu_vertex_shader(shader_t shader);
extern SDL_GPUShader* get_gpu_fragment_shader(shader_t shader);
extern const char* get_name(shader_t shader);
extern SDL_GPUCullMode get_gpu_cull_mode(shader_t shader);
extern bool is_depth_test_enabled(shader_t shader);
extern bool is_depth_write_enabled(shader_t shader);
extern bool is_blend_enabled(shader_t shader);
extern SDL_GPUBlendFactor get_gpu_src_blend(shader_t shader);
extern SDL_GPUBlendFactor get_gpu_dst_blend(shader_t shader);

static SDL_GPUGraphicsPipeline* create_pipeline(shader_t shader, const SDL_GPUVertexAttribute* attributes,
                                                size_t attribute_count, bool msaa, bool shadow) 
{
    assert(g_window);
    assert(g_device);
    assert(shader);

    // Create pipeline directly using the shader's compiled shaders
    uint32_t vertexStride = vertex_stride(attributes, attribute_count);

    SDL_GPUVertexBufferDescription vertex_buffer_desc = {0};
    vertex_buffer_desc.pitch = vertexStride;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexInputState vertex_input_state = {0};
    vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    vertex_input_state.num_vertex_buffers = 1;
    vertex_input_state.vertex_attributes = attributes;
    vertex_input_state.num_vertex_attributes = (uint32_t)attribute_count;

    SDL_GPUColorTargetDescription color_target = {0};
    if (!shadow) {
        color_target.format = SDL_GetGPUSwapchainTextureFormat(g_device, g_window);
    }

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {0};
    pipeline_create_info.vertex_shader = get_gpu_vertex_shader(shader);
    pipeline_create_info.fragment_shader = get_gpu_fragment_shader(shader);
    pipeline_create_info.vertex_input_state = vertex_input_state;
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_create_info.props = SDL_CreateProperties();

    // Create name string
    const char* base_name = get_name(shader);
    char name[256];
    strcpy(name, base_name);
    if (msaa) {
        strcat(name, "_msaa");
    }
    if (shadow) {
        strcat(name, "_shadow");
    }

    SDL_SetStringProperty(pipeline_create_info.props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, name);

    // Set rasterizer state based on shader properties
    pipeline_create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_create_info.rasterizer_state.cull_mode = get_gpu_cull_mode(shader);
    pipeline_create_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    // Set multisample state based on current render target
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
    if (!shadow) {
        SDL_GPUColorTargetBlendState blend_state = {0};
        blend_state.enable_blend = is_blend_enabled(shader);
        if (blend_state.enable_blend) {
            SDL_GPUBlendFactor src_blend = get_gpu_src_blend(shader);
            SDL_GPUBlendFactor dst_blend = get_gpu_dst_blend(shader);
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
    } else {
        // Shadow shaders are depth-only, no color targets
        pipeline_create_info.target_info.num_color_targets = 0;
        pipeline_create_info.target_info.color_target_descriptions = NULL;
    }

    // Set depth stencil target info to match the depth texture
    pipeline_create_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pipeline_create_info.target_info.has_depth_stencil_target = true;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(g_device, &pipeline_create_info);
    SDL_DestroyProperties(pipeline_create_info.props);
    
    if (!pipeline) {
        // Handle error - in C we can't throw exceptions
        return NULL;
    }

    return pipeline;
}

SDL_GPUGraphicsPipeline* pipeline_factory_pipeline(shader_t shader, bool msaa, bool shadow)
{
    assert(g_window);
    assert(g_device);
    assert(shader);
    assert(g_pipeline_cache);

    uint64_t key = pipeline_key(shader, msaa, shadow);

    // Check if pipeline exists in cache
    object_t cached_obj = (object_t)map_get(g_pipeline_cache, key);
    if (cached_obj) {
        pipeline_object_t* pipeline_obj = (pipeline_object_t*)object_impl(cached_obj, g_pipeline_factory_type);
        return pipeline_obj->pipeline;
    }

    // Create new pipeline
    SDL_GPUVertexAttribute attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},                 // position : POSITION (semantic 0)
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 3}, // uv0 : TEXCOORD1 (semantic 2)
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float) * 5}, // normal : TEXCOORD2 (semantic 1)
        {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(float) * 8}   // bone_index : TEXCOORD3 (semantic 3)
    };

    SDL_GPUGraphicsPipeline* pipeline = create_pipeline(shader, attributes, 4, msaa, shadow);
    if (!pipeline) {
        return NULL;
    }

    // Store in cache
    object_t cache_obj = (object_t)object_create(g_pipeline_factory_type, sizeof(pipeline_object_t));
    if (cache_obj) map_set(g_pipeline_cache, key, cache_obj);
    if (cache_obj) {
        pipeline_object_t* pipeline_obj = (pipeline_object_t*)object_impl(cache_obj, g_pipeline_factory_type);
        pipeline_obj->pipeline = pipeline;
    }

    return pipeline;
}

void pipeline_factory_init(SDL_Window* win, SDL_GPUDevice* dev)
{
    assert(!g_pipeline_factory_type);

    g_window = win;
    g_device = dev;
    g_pipeline_factory_type = object_type_create("pipeline");
    g_pipeline_cache = map_create(INITIAL_CACHE_SIZE);
}

void pipeline_factory_uninit()
{
    assert(g_pipeline_factory_type);
    object_destroy((object_t)g_pipeline_cache);
    g_pipeline_cache = nullptr;
    g_window = nullptr;
    g_device = nullptr;
}
