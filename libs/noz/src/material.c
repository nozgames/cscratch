//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct uniform_buffer_info
{
    uint32_t size;
    uint32_t offset;
} uniform_buffer_info_t;

typedef struct material_impl
{
    string128_t name;
    int vertex_uniform_count;
    int fragment_uniform_count;
    shader_t shader;
    texture_t* textures;
    size_t texture_count;
    uniform_buffer_info_t* uniforms;
	uint8_t* uniforms_data;
    //vector<texture> textures;
    //vector<uniform_buffer_info> uniforms;
    //vector<uint8_t> uniforms_data;
} material_impl_t;

static object_type_t g_material_type = nullptr;

inline material_impl_t* to_impl(material_t material)
{
    assert(material);
    return (material_impl_t*)object_impl((object_t)material, g_material_type);
}

inline uint8_t* mesh_vertex_uniform_data(material_impl_t* impl)
{
    assert(impl->vertex_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + 0;
    return impl->uniforms_data + buffer->offset;
}

inline size_t mesh_vertex_uniform_data_size(material_impl_t* impl)
{
    assert(impl->vertex_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count - 1;
    return buffer->offset + buffer->size;
}

inline uint8_t* mesh_fragment_uniform_data(material_impl_t* impl)
{
    assert(impl->fragment_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count;
    return impl->uniforms_data + buffer->offset;
}

inline size_t mesh_fragment_uniform_data_size(material_impl_t* impl)
{
    assert(impl->fragment_uniform_count > 0);
    uniform_buffer_info_t* buffer = impl->uniforms + impl->vertex_uniform_count + impl->fragment_uniform_count - 1;
    return buffer->offset + buffer->size;
}

material_t material_create(shader_t shader, const char* name)
{
	material_t material = (material_t)object_create(g_material_type, sizeof(material_impl_t));
    material_impl_t* impl = to_impl(material);;
    impl->shader = shader;
	impl->vertex_uniform_count = shader_vertex_uniform_count(shader);
    impl->fragment_uniform_count = shader_fragment_uniform_count(shader);
    //impl->textures.resize(get_sampler_count(shader));
    return material;
}

const char* material_name(material_t material)
{
    return to_impl(material)->name.data;
}

shader_t material_shader(material_t material)
{
    return to_impl(material)->shader;
}

void material_set_texture(material_t material, texture_t texture, size_t index)
{
	material_impl_t* impl = to_impl(material);
    assert(index >= 0 && index < impl->texture_count);
    impl->textures[index] = texture;
}



void material_bind_gpu(material_t material, SDL_GPUCommandBuffer* cb)
{
#if 0
	material_impl_t* impl = to_impl(material);

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

void material_init()
{
	g_material_type = object_type_create("material");
}

void material_uninit()
{
    object_type_destroy(g_material_type);
    g_material_type = nullptr;
}
