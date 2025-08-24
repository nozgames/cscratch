//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: rework memory management here after asset loading

struct shader_impl 
{
    OBJECT_BASE;
    name_t name;
    SDL_GPUShader* vertex;
    SDL_GPUShader* fragment;
    int vertex_uniform_count;
    int fragment_uniform_count;
    int sampler_count;
    shader_flags_t flags;
    SDL_GPUBlendFactor src_blend;
    SDL_GPUBlendFactor dst_blend;
    SDL_GPUCullMode cull;
};

static map_t* g_shader_cache = NULL;
static SDL_GPUDevice* g_device = NULL;

static inline shader_impl* to_impl(void* s) { return (shader_impl*)to_object((object_t*)s, type_shader); }

// todo: destructor
#if 0

static void shader_destroy_impl(shader_impl* impl)
{
    assert(impl);
    if (impl->vertex)
    {
        SDL_ReleaseGPUShader(g_device, impl->vertex);
        impl->vertex = NULL;
    }

    if (impl->fragment)
    {
        SDL_ReleaseGPUShader(g_device, impl->fragment);
        impl->fragment = NULL;
    }    
}
#endif

shader_t* shader_load(allocator_t* allocator, name_t* name)
{
    assert(g_device);
    assert(g_shader_cache);
    assert(name);

    uint64_t key = hash_name(name);
    shader_t* shader = (shader_t*)map_get(g_shader_cache, key);
    if (shader) 
        return NULL;

	shader = (shader_t*)object_alloc(allocator, sizeof(shader_impl), type_shader);
    if (!shader)
        return NULL;
   
    shader_impl* impl = (shader_impl*)to_impl(shader);
	name_copy(&impl->name, name);
    impl->vertex = NULL;
    impl->fragment = NULL;
    impl->vertex_uniform_count = 0;
    impl->fragment_uniform_count = 0;
    impl->sampler_count = 0;
    impl->flags = shader_flags_none;
    impl->src_blend = SDL_GPU_BLENDFACTOR_ONE;
    impl->dst_blend = SDL_GPU_BLENDFACTOR_ZERO;
    impl->cull = SDL_GPU_CULLMODE_NONE;

    // Load shader file
    path_t shader_path;
    asset_path(&shader_path, name, "shader");
    stream_t* stream = stream_load_from_file(allocator , &shader_path);
    if (!stream) 
        return NULL;

    if (!stream_read_signature(stream, "SHDR", 4))
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

    // Read bytecode lengths
    uint32_t vertex_bytecode_length = stream_read_uint32(stream);
    uint8_t* vertex_bytecode = (uint8_t*)allocator_alloc(allocator, vertex_bytecode_length);
    if (!vertex_bytecode) 
    {
        object_free(stream);
        return NULL;
    }
    stream_read_bytes(stream, vertex_bytecode, vertex_bytecode_length);

    uint32_t fragment_bytecode_length = stream_read_uint32(stream);
    uint8_t* fragment_bytecode = (uint8_t*)allocator_alloc(allocator, fragment_bytecode_length);
    if (!fragment_bytecode) 
    {
        free(vertex_bytecode);
        object_free(stream);
        return NULL;
    }
    stream_read_bytes(stream, fragment_bytecode, fragment_bytecode_length);

    impl->vertex_uniform_count = stream_read_int32(stream);
    impl->fragment_uniform_count = stream_read_int32(stream);
    impl->sampler_count = stream_read_int32(stream);
    impl->flags = (shader_flags_t)stream_read_uint8(stream);
    impl->src_blend = (SDL_GPUBlendFactor)stream_read_uint32(stream);
    impl->dst_blend = (SDL_GPUBlendFactor)stream_read_uint32(stream);
    impl->cull = (SDL_GPUCullMode)stream_read_uint32(stream);

    object_free(stream);

    // Create fragment shader
    SDL_GPUShaderCreateInfo fragment_create_info = {0};
    fragment_create_info.code = fragment_bytecode;
    fragment_create_info.code_size = fragment_bytecode_length;
    fragment_create_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_create_info.entrypoint = "ps";
    fragment_create_info.num_samplers = impl->sampler_count + (uint32_t)sampler_register_user0;
    fragment_create_info.num_storage_textures = 0;
    fragment_create_info.num_storage_buffers = 0;
    fragment_create_info.num_uniform_buffers = impl->fragment_uniform_count + (uint32_t)fragment_register_user0;
    fragment_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(fragment_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->fragment = SDL_CreateGPUShader(g_device, &fragment_create_info);
    SDL_DestroyProperties(fragment_create_info.props);

    if (!impl->fragment)
    {
        free(vertex_bytecode);
        free(fragment_bytecode);
        return NULL;
    }

    // Create vertex shader
    SDL_GPUShaderCreateInfo vertex_create_info = {0};
    vertex_create_info.code = vertex_bytecode;
    vertex_create_info.code_size = vertex_bytecode_length;
    vertex_create_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertex_create_info.entrypoint = "vs";
    vertex_create_info.num_samplers = 0;
    vertex_create_info.num_storage_textures = 0;
    vertex_create_info.num_storage_buffers = 0;
    vertex_create_info.num_uniform_buffers = impl->vertex_uniform_count + (uint32_t)vertex_register_user0;
    vertex_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(vertex_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->vertex = SDL_CreateGPUShader(g_device, &vertex_create_info);
    SDL_DestroyProperties(vertex_create_info.props);

    free(vertex_bytecode);
    free(fragment_bytecode);

    if (!impl->vertex)
        return NULL;

    map_set(g_shader_cache, key, shader);

    return shader;
}

// Shader property getter functions with new naming convention
SDL_GPUShader* shader_gpu_vertex_shader(shader_t* shader)
{
    return to_impl(shader)->vertex;
}

SDL_GPUShader* shader_gpu_fragment_shader(shader_t* shader)
{
    return to_impl(shader)->fragment;
}

SDL_GPUCullMode shader_gpu_cull_mode(shader_t* shader)
{
    return to_impl(shader)->cull;
}

bool shader_blend_enabled(shader_t* shader)
{
    return (to_impl(shader)->flags & shader_flags_blend) != 0;
}

SDL_GPUBlendFactor shader_gpu_src_blend(shader_t* shader)
{
    return to_impl(shader)->src_blend;
}

SDL_GPUBlendFactor shader_gpu_dst_blend(shader_t* shader)
{
    return to_impl(shader)->dst_blend;
}

bool shader_depth_test_enabled(shader_t* shader)
{
    return (to_impl(shader)->flags & shader_flags_depth_test) != 0;
}

bool shader_depth_write_enabled(shader_t* shader)
{
    return (to_impl(shader)->flags & shader_flags_depth_write) != 0;
}

int shader_vertex_uniform_count(shader_t* shader)
{
    return to_impl(shader)->vertex_uniform_count;
}

int shader_fragment_uniform_count(shader_t* shader)
{
    return to_impl(shader)->fragment_uniform_count;
}

int shader_sampler_count(shader_t* shader)
{
    return to_impl(shader)->sampler_count;
}

name_t* shader_name(shader_t* shader)
{
    return &to_impl(shader)->name;
}

void shader_init(renderer_traits* traits, SDL_GPUDevice* device)
{
    g_device = device;
    g_shader_cache = map_alloc(NULL, traits->max_shaders);
}

void shader_uninit()
{
    assert(g_device);
    object_free(g_shader_cache);
    g_device = NULL;
	g_shader_cache = NULL;
}