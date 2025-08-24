//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <noz/noz.h>
#include <noz/color.h>

// @mesh
typedef struct mesh_vertex
{
    vec3 position;
    vec2 uv0;
    vec3 normal;
    float bone;
} mesh_vertex;


typedef struct bone_transform
{
    vec3 position;
    vec3 scale;
    quat rotation;
} bone_transform_t;

typedef struct bone
{
    char* name;
    int index;
    int parentIndex;
    mat4 world_to_local;
    mat4 local_to_world;
    bone_transform_t transform;
    float length;
    vec3 direction;
} bone_t;

typedef struct sampler_options
{
    TextureFilter min_filter;
    TextureFilter mag_filter;
    TextureClamp clamp_u;
    TextureClamp clamp_v;
    TextureClamp clamp_w;
    SDL_GPUCompareOp compare_op;
} sampler_options_t;

// Function to compare sampler options
bool sampler_options_equals(sampler_options_t* a, sampler_options_t* b);

// Shader flags enum (C99 version)
typedef enum shader_flags 
{
    shader_flags_none = 0,
    shader_flags_depth_test = 1 << 0,
    shader_flags_depth_write = 1 << 1,
    shader_flags_blend = 1 << 2
} shader_flags_t;

// Register enums (C99 versions)
typedef enum vertex_register 
{
    vertex_register_camera = 0,
    vertex_register_object = 1,
    vertex_register_bone = 2,
    vertex_register_user0 = 3,
    vertex_register_user1 = 4,
    vertex_register_user2 = 5,
    vertex_register_count
} vertex_register_t;

typedef enum fragment_register 
{
    fragment_register_color = 0,
    fragment_register_light = 1,
    fragment_register_user0 = 2,
    fragment_register_user1 = 3,
    fragment_register_user2 = 4,
    fragment_register_count
} fragment_register_t;

typedef enum sampler_register 
{
    sampler_register_shadow_map = 0,
    sampler_register_user0 = 1,
    sampler_register_user1 = 2,
    sampler_register_user2 = 3,
    sampler_register_count
} sampler_register_t;

// Function to convert texture format to SDL format
SDL_GPUTextureFormat texture_format_to_sdl(TextureFormat format);

typedef enum animation_track_type
{
    animation_track_type_translation = 0, // position (vec3)
    animation_track_type_rotation = 1,    // rotation (quat)
    animation_track_type_scale = 2        // scale (vec3)
} animation_track_type_t;

typedef struct animation_track
{
    uint8_t bone;
    animation_track_type_t type;
    int data_offset;
} animation_track_t;

// @object
void InitObject();
void ShutdownObject();

// @entity
void InitEntity();
void ShutdownEntity();

// @renderer
void InitRenderer(RendererTraits* traits, SDL_Window* window);
void ShutdownRenderer();
void renderer_begin_frame();
void renderer_end_frame();
SDL_GPURenderPass* renderer_begin_pass(bool clear, color_t clear_color, bool msaa, Texture* target);
SDL_GPURenderPass* renderer_begin_gamma_pass();
SDL_GPURenderPass* renderer_begin_shadow_pass();
void renderer_end_pass();
void renderer_bind_texture(SDL_GPUCommandBuffer* cb, Texture* texture, int index);
void renderer_bind_material(Material* material);
void renderer_bind_default_texture(int texture_index);

// @render_buffer
void InitRenderBuffer(RendererTraits* traits);
void ShutdownRenderBuffer();
void render_buffer_begin_gamma_pass();
void render_buffer_clear();
void render_buffer_execute(SDL_GPUCommandBuffer* cb);

// @sampler_factory
void InitSamplerFactory(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownSamplerFactory();
SDL_GPUSampler* sampler_factory_sampler(Texture* texture);

// @pipeline_factory
void InitPipelineFactory(SDL_Window* window, SDL_GPUDevice* device);
void ShutdownPipelineFactory();
SDL_GPUGraphicsPipeline* pipeline_factory_pipeline(Shader* shader, bool msaa, bool shadow);

// @material
void material_bind_gpu(Material* material, SDL_GPUCommandBuffer* cb);

// @mesh
void InitMesh(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownMesh();
void RenderMesh(Mesh* mesh, SDL_GPURenderPass* pass);

// @texture
void InitTexture(RendererTraits* traits, SDL_GPUDevice* dev);
void ShutdownTexture();
SDL_GPUTexture* GetGPUTexture(Texture* texture);
sampler_options_t GetSamplerOptions(Texture* texture);

// @shader
void InitShader(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownShader();

// New naming convention
SDL_GPUShader* shader_gpu_vertex_shader(Shader* shader);
SDL_GPUShader* shader_gpu_fragment_shader(Shader* shader);
SDL_GPUCullMode shader_gpu_cull_mode(Shader* shader);
bool shader_blend_enabled(Shader* shader);
SDL_GPUBlendFactor shader_gpu_src_blend(Shader* shader);
SDL_GPUBlendFactor shader_gpu_dst_blend(Shader* shader);
bool shader_depth_test_enabled(Shader* shader);
bool shader_depth_write_enabled(Shader* shader);
int shader_vertex_uniform_count(Shader* shader);
int shader_fragment_uniform_count(Shader* shader);
int shader_sampler_count(Shader* shader);
const name_t* GetName(Shader* shader);

// @font
void InitFont(RendererTraits* traits, SDL_GPUDevice* device);
void ShutdownFont();
Material* font_material(Font* font);

// @camera
void InitCamera();
void ShutdownCamera();

// @animation
void animation_evaluate_frame(
    Animation* animation,
    float time,
    bone_t* bones,
    size_t bone_count,
    bone_transform_t* transforms,
    size_t transform_count);

// @helpers
inline SDL_FColor color_to_sdl(color_t color) 
{
    SDL_FColor result;
    result.r = color.r;
    result.g = color.g;
    result.b = color.b;
    result.a = color.a;
    return result;
}
//inline b2Vec2 vec2o_b2(vec2& v) { return b2Vec2(v.x, v.y); }
