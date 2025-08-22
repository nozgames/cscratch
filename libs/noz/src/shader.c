//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/stream.h"
#include "noz/object.h"
#include "noz/hash.h"
#include "internal.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

typedef struct shader_impl 
{
    string128 name;
    SDL_GPUShader* vertex;
    SDL_GPUShader* fragment;
    int vertex_uniform_count;
    int fragment_uniform_count;
    int sampler_count;
    shader_flags_t flags;
    SDL_GPUBlendFactor src_blend;
    SDL_GPUBlendFactor dst_blend;
    SDL_GPUCullMode cull;
} shader_impl_t;

static object_registry_t g_shader_cache = nullptr;
static SDL_GPUDevice* g_device = nullptr;
static object_type_t g_shader_type = nullptr;

shader_impl_t* to_impl(shader_t shader)
{
    assert(shader);
    return (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
}

static void shader_destroy_impl(shader_impl_t* impl)
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

shader_t shader_load(const char* name)
{
    assert(g_device);
    assert(g_shader_cache);
    assert(name);

    uint64_t key = hash_string(name);

    // Check if shader exists in cache
    object_t cached_obj = object_registry_get(g_shader_cache, key);
    if (cached_obj) 
    {
        shader_impl_t* shader_obj = (shader_impl_t*)object_impl(cached_obj, g_shader_type);
        return (shader_t)cached_obj;
    }

    // Create new shader
    object_t cache_obj = object_registry_alloc(g_shader_cache, key);
    if (!cache_obj) 
    {
        return nullptr;
    }
    
    shader_impl_t* impl = (shader_impl_t*)object_impl(cache_obj, g_shader_type);
	string128_set(&impl->name, name);
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
    const char* shader_path = asset_path(name, "shader");
    stream_t reader = stream_create_from_file(shader_path);
    if (!reader) 
    {
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }

    if (!stream_read_signature(reader, "SHDR", 4))
    {
        stream_destroy(reader);
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }

    uint32_t version = stream_read_uint32(reader);
    if (version != 1)
    {
        stream_destroy(reader);
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }

    // Read bytecode lengths
    uint32_t vertex_bytecode_length = stream_read_uint32(reader);
    uint8_t* vertex_bytecode = malloc(vertex_bytecode_length);
    if (!vertex_bytecode) 
    {
        stream_destroy(reader);
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }
    stream_read_bytes(reader, vertex_bytecode, vertex_bytecode_length);

    uint32_t fragment_bytecode_length = stream_read_uint32(reader);
    uint8_t* fragment_bytecode = malloc(fragment_bytecode_length);
    if (!fragment_bytecode) 
    {
        free(vertex_bytecode);
        stream_destroy(reader);
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }
    stream_read_bytes(reader, fragment_bytecode, fragment_bytecode_length);

    impl->vertex_uniform_count = stream_read_int32(reader);
    impl->fragment_uniform_count = stream_read_int32(reader);
    impl->sampler_count = stream_read_int32(reader);
    impl->flags = (shader_flags_t)stream_read_uint8(reader);
    impl->src_blend = (SDL_GPUBlendFactor)stream_read_uint32(reader);
    impl->dst_blend = (SDL_GPUBlendFactor)stream_read_uint32(reader);
    impl->cull = (SDL_GPUCullMode)stream_read_uint32(reader);

    stream_destroy(reader);

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

    SDL_SetStringProperty(fragment_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name);
    impl->fragment = SDL_CreateGPUShader(g_device, &fragment_create_info);
    SDL_DestroyProperties(fragment_create_info.props);

    if (!impl->fragment)
    {
        free(vertex_bytecode);
        free(fragment_bytecode);
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
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
    vertex_create_info.num_uniform_buffers = impl->vertex_uniform_count + (uint32_t)vertex_register_user0;
    vertex_create_info.props = SDL_CreateProperties();

    SDL_SetStringProperty(vertex_create_info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name);
    impl->vertex = SDL_CreateGPUShader(g_device, &vertex_create_info);
    SDL_DestroyProperties(vertex_create_info.props);

    free(vertex_bytecode);
    free(fragment_bytecode);

    if (!impl->vertex)
    {
        shader_destroy_impl(impl);
        object_destroy(cache_obj);
        return nullptr;
    }

    return (shader_t)cache_obj;
}

// Shader property getter functions with new naming convention
SDL_GPUShader* shader_gpu_vertex_shader(shader_t shader)
{
    if (!shader) return nullptr;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->vertex;
}

SDL_GPUShader* shader_gpu_fragment_shader(shader_t shader)
{
    if (!shader) return nullptr;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->fragment;
}

SDL_GPUCullMode shader_gpu_cull_mode(shader_t shader)
{
    if (!shader) return SDL_GPU_CULLMODE_NONE;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->cull;
}

bool shader_blend_enabled(shader_t shader)
{
    if (!shader) return false;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return (impl->flags & shader_flags_blend) != 0;
}

SDL_GPUBlendFactor shader_gpu_src_blend(shader_t shader)
{
    if (!shader) return SDL_GPU_BLENDFACTOR_ONE;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->src_blend;
}

SDL_GPUBlendFactor shader_gpu_dst_blend(shader_t shader)
{
    if (!shader) return SDL_GPU_BLENDFACTOR_ZERO;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->dst_blend;
}

bool shader_depth_test_enabled(shader_t shader)
{
    if (!shader) return false;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return (impl->flags & shader_flags_depth_test) != 0;
}

bool shader_depth_write_enabled(shader_t shader)
{
    if (!shader) return false;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return (impl->flags & shader_flags_depth_write) != 0;
}

int shader_vertex_uniform_count(shader_t shader)
{
    if (!shader) return 0;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->vertex_uniform_count;
}

int shader_fragment_uniform_count(shader_t shader)
{
    if (!shader) return 0;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->fragment_uniform_count;
}

int shader_sampler_count(shader_t shader)
{
    if (!shader) return 0;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->sampler_count;
}

const char* shader_name(shader_t shader)
{
    if (!shader) return nullptr;
    shader_impl_t* impl = (shader_impl_t*)object_impl((object_t)shader, g_shader_type);
    return impl->name;
}

// Legacy function names for compatibility with existing code
SDL_GPUShader* get_gpu_vertex_shader(shader_t shader)
{
    return shader_gpu_vertex_shader(shader);
}

SDL_GPUShader* get_gpu_fragment_shader(shader_t shader)
{
    return shader_gpu_fragment_shader(shader);
}

const char* get_name(shader_t shader)
{
    return shader_name(shader);
}

SDL_GPUCullMode get_gpu_cull_mode(shader_t shader)
{
    return shader_gpu_cull_mode(shader);
}

bool is_depth_test_enabled(shader_t shader)
{
    return shader_depth_test_enabled(shader);
}

bool is_depth_write_enabled(shader_t shader)
{
    return shader_depth_write_enabled(shader);
}

bool is_blend_enabled(shader_t shader)
{
    return shader_blend_enabled(shader);
}

SDL_GPUBlendFactor get_gpu_src_blend(shader_t shader)
{
    return shader_gpu_src_blend(shader);
}

SDL_GPUBlendFactor get_gpu_dst_blend(shader_t shader)
{
    return shader_gpu_dst_blend(shader);
}

int get_vertex_uniform_count(shader_t shader)
{
    return shader_vertex_uniform_count(shader);
}

int get_fragment_uniform_count(shader_t shader)
{
    return shader_fragment_uniform_count(shader);
}

int get_sampler_count(shader_t shader)
{
    return shader_sampler_count(shader);
}

void shader_init(const renderer_traits* traits, SDL_GPUDevice* device)
{
    g_shader_type = object_type_create("shader", sizeof(shader_impl_t));
    g_device = device;
    g_shader_cache = object_registry_create(g_shader_type, sizeof(shader_impl_t), traits->max_shaders);
}

void shader_uninit()
{
    assert(g_device);
    object_destroy((object_t)g_shader_cache);
    g_device = nullptr;
	g_shader_cache = nullptr;
    g_shader_type = nullptr;
}