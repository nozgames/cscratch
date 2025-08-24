//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: rework memory management here after asset loading

struct ShaderImpl
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

static Map* g_shader_cache = nullptr;
static SDL_GPUDevice* g_device = nullptr;

static ShaderImpl* Impl(Shader* s) { return (ShaderImpl*)Cast(s, TYPE_SHADER); }

// todo: destructor
#if 0

static void shader_destroy_impl(ShaderImpl* impl)
{
    assert(impl);
    if (impl->vertex)
    {
        SDL_ReleaseGPUShader(g_device, impl->vertex);
        impl->vertex = nullptr;
    }

    if (impl->fragment)
    {
        SDL_ReleaseGPUShader(g_device, impl->fragment);
        impl->fragment = nullptr;
    }    
}
#endif

Shader* LoadShader(Allocator* allocator, const name_t* name)
{
    assert(g_device);
    assert(g_shader_cache);
    assert(name);

    uint64_t key = Hash(name);
    auto shader = (Shader*)GetValue(g_shader_cache, key);
    if (shader) 
        return nullptr;

    shader = (Shader*)CreateObject(allocator, sizeof(ShaderImpl), TYPE_SHADER);
    if (!shader)
        return nullptr;
   
    auto* impl = Impl(shader);
    SetName(&impl->name, name);
    impl->vertex = nullptr;
    impl->fragment = nullptr;
    impl->vertex_uniform_count = 0;
    impl->fragment_uniform_count = 0;
    impl->sampler_count = 0;
    impl->flags = shader_flags_none;
    impl->src_blend = SDL_GPU_BLENDFACTOR_ONE;
    impl->dst_blend = SDL_GPU_BLENDFACTOR_ZERO;
    impl->cull = SDL_GPU_CULLMODE_NONE;

    // Load shader file
    std::string asset_name = std::string(name->value) + ".shader";
    auto stream = LoadAssetStream(allocator, asset_name.c_str());
    if (!stream) 
        return nullptr;

    if (!ReadFileSignature(stream, "SHDR", 4))
    {
        Destroy(stream);
        return nullptr;
    }

    u32 version = ReadU32(stream);
    if (version != 1)
    {
        Destroy(stream);
        return nullptr;
    }

    // Read bytecode lengths
    auto vertex_bytecode_length = ReadU32(stream);
    auto* vertex_bytecode = (u8*)Alloc(allocator, vertex_bytecode_length);
    if (!vertex_bytecode) 
    {
        Destroy(stream);
        return nullptr;
    }
    ReadBytes(stream, vertex_bytecode, vertex_bytecode_length);

    auto fragment_bytecode_length = ReadU32(stream);
    auto* fragment_bytecode = (u8*)Alloc(allocator, fragment_bytecode_length);
    if (!fragment_bytecode) 
    {
        free(vertex_bytecode);
        Destroy(stream);
        return nullptr;
    }
    ReadBytes(stream, fragment_bytecode, fragment_bytecode_length);

    impl->vertex_uniform_count = ReadI32(stream);
    impl->fragment_uniform_count = ReadI32(stream);
    impl->sampler_count = ReadI32(stream);
    impl->flags = (shader_flags_t)ReadU8(stream);
    impl->src_blend = (SDL_GPUBlendFactor)ReadU32(stream);
    impl->dst_blend = (SDL_GPUBlendFactor)ReadU32(stream);
    impl->cull = (SDL_GPUCullMode)ReadU32(stream);

    Destroy(stream);

    // Create fragment shader
    SDL_GPUShaderCreateInfo fragment_create_info = {0};
    fragment_create_info.code = fragment_bytecode;
    fragment_create_info.code_size = fragment_bytecode_length;
    fragment_create_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    fragment_create_info.entrypoint = "ps";
    fragment_create_info.num_samplers = impl->sampler_count + (u32)sampler_register_user0;
    fragment_create_info.num_storage_textures = 0;
    fragment_create_info.num_storage_buffers = 0;
    fragment_create_info.num_uniform_buffers = impl->fragment_uniform_count + (u32)fragment_register_user0;
    fragment_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(fragment_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->fragment = SDL_CreateGPUShader(g_device, &fragment_create_info);
    SDL_DestroyProperties(fragment_create_info.props);

    if (!impl->fragment)
    {
        free(vertex_bytecode);
        free(fragment_bytecode);
        return nullptr;
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
    vertex_create_info.num_uniform_buffers = impl->vertex_uniform_count + (u32)vertex_register_user0;
    vertex_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(vertex_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name->value);
    impl->vertex = SDL_CreateGPUShader(g_device, &vertex_create_info);
    SDL_DestroyProperties(vertex_create_info.props);

    free(vertex_bytecode);
    free(fragment_bytecode);

    if (!impl->vertex)
        return nullptr;

    SetValue(g_shader_cache, key, shader);

    return shader;
}

// Shader property getter functions with new naming convention
SDL_GPUShader* shader_gpu_vertex_shader(Shader* shader)
{
    return Impl(shader)->vertex;
}

SDL_GPUShader* shader_gpu_fragment_shader(Shader* shader)
{
    return Impl(shader)->fragment;
}

SDL_GPUCullMode shader_gpu_cull_mode(Shader* shader)
{
    return Impl(shader)->cull;
}

bool shader_blend_enabled(Shader* shader)
{
    return (Impl(shader)->flags & shader_flags_blend) != 0;
}

SDL_GPUBlendFactor shader_gpu_src_blend(Shader* shader)
{
    return Impl(shader)->src_blend;
}

SDL_GPUBlendFactor shader_gpu_dst_blend(Shader* shader)
{
    return Impl(shader)->dst_blend;
}

bool shader_depth_test_enabled(Shader* shader)
{
    return (Impl(shader)->flags & shader_flags_depth_test) != 0;
}

bool shader_depth_write_enabled(Shader* shader)
{
    return (Impl(shader)->flags & shader_flags_depth_write) != 0;
}

int shader_vertex_uniform_count(Shader* shader)
{
    return Impl(shader)->vertex_uniform_count;
}

int shader_fragment_uniform_count(Shader* shader)
{
    return Impl(shader)->fragment_uniform_count;
}

int shader_sampler_count(Shader* shader)
{
    return Impl(shader)->sampler_count;
}

const name_t* GetName(Shader* shader)
{
    return &Impl(shader)->name;
}

void InitShader(RendererTraits* traits, SDL_GPUDevice* device)
{
    g_device = device;
    g_shader_cache = CreateMap(nullptr, traits->max_shaders);
}

void ShutdownShader()
{
    assert(g_device);
    Destroy(g_shader_cache);
    g_device = nullptr;
	g_shader_cache = nullptr;
}