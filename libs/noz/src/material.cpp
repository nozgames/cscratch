//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct UniformBuffer
{
    u32 size;
    u32 offset;
};

struct MaterialImpl
{    
    OBJECT_BASE;
    name_t name;
    int vertex_uniform_count;
    int fragment_uniform_count;
    Shader* shader;
    Texture** textures;
    size_t texture_count;
    UniformBuffer* uniforms;
	uint8_t* uniforms_data;
    //vector<texture> textures;
    //vector<uniform_buffer_info> uniforms;
    //vector<uint8_t> uniforms_data;
};

static MaterialImpl* Impl(Material* m) { return (MaterialImpl*)Cast(m, TYPE_MATERIAL); }


#if 0
inline uint8_t* material_vertex_uniform_data(MaterialImpl* impl)
{
    assert(impl->vertex_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + 0;
    return impl->uniforms_data + buffer->offset;
}

inline size_t material_vertex_uniform_data_size(MaterialImpl* impl)
{
    assert(impl->vertex_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count - 1;
    return buffer->offset + buffer->size;
}

inline uint8_t* material_fragment_uniform_data(MaterialImpl* impl)
{
    assert(impl->fragment_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count;
    return impl->uniforms_data + buffer->offset;
}

inline size_t material_fragment_uniform_data_size(MaterialImpl* impl)
{
    assert(impl->fragment_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count + impl->fragment_uniform_count - 1;
    return buffer->offset + buffer->size;
}
#endif

Material* CreateMaterial(Allocator* allocator, Shader* shader, name_t* name)
{
    MaterialImpl* impl = Impl((Material*)CreateObject(allocator, sizeof(MaterialImpl*), TYPE_MATERIAL));
    if (!impl)
        return nullptr;
    impl->shader = shader;
    impl->vertex_uniform_count = shader_vertex_uniform_count(shader);
    impl->fragment_uniform_count = shader_fragment_uniform_count(shader);
    impl->texture_count = shader_sampler_count(shader);
    SetName(&impl->name, name);
    return (Material*)impl;
}

name_t* GetName(Material* material)
{
    return &Impl(material)->name;
}

Shader* GetShader(Material* material)
{
    return Impl(material)->shader;
}

void SetTexture(Material* material, Texture* texture, size_t index)
{
    auto impl = Impl(material);
    assert(index < impl->texture_count);
    impl->textures[index] = texture;
}

void BindMaterialGPU(Material* material, SDL_GPUCommandBuffer* cb)
{
    auto impl = Impl(material);
    BindShaderGPU(impl->shader);

    // todo: fix

#if 0
	MaterialImpl* impl = Impl(material);

    // Then push uniform buffer data for vertex shader (additional buffers beyond default 0,1,2)
    if (impl->vertex_uniform_count > 0)
    {
        auto size = get_vertex_uniform_data_size(impl);
        if (size > 0)
            SDL_PushGPUVertexUniformData(
                cb,
                vertex_register_user0,
                get_vertex_uniform_data(impl),
                size);
    }

    // Push uniform buffer data for fragment shader (starts at slot 0, no defaults)
    if (impl->fragment_uniform_count > 0)
    {
        auto size = get_fragment_uniform_data_size(impl);
        if (size > 0)
            SDL_PushGPUFragmentUniformData(
                cb,
                fragment_register_user0,
                get_fragment_uniform_data(impl),
                size);
    }

    //for (size_t i = 0, c = impl->textures.size(); i < c; ++i)
    //    bind_texture(cb, impl->textures[i], static_cast<int>(i) + static_cast<int>(sampler_register::user0));
#endif
}
