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
    texture_filter_t min_filter;
    texture_filter_t mag_filter;
    texture_clamp_t clamp_u;
    texture_clamp_t clamp_v;
    texture_clamp_t clamp_w;
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
SDL_GPUTextureFormat texture_format_to_sdl(texture_format_t format);

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
void object_init(void);
void object_uninit(void);

// @renderer
void renderer_init(renderer_traits* traits, SDL_Window* window);
void renderer_uninit(void);
void renderer_begin_frame(void);
void renderer_end_frame(void);
SDL_GPURenderPass* renderer_begin_pass(bool clear, color_t clear_color, bool msaa, texture_t* target);
SDL_GPURenderPass* renderer_begin_gamma_pass(void);
SDL_GPURenderPass* renderer_begin_shadow_pass(void);
void renderer_end_pass(void);
void renderer_bind_texture(SDL_GPUCommandBuffer* cb, texture_t* texture, int index);
void renderer_bind_material(material_t* material);
void renderer_bind_default_texture(int texture_index);

// @render_buffer
void render_buffer_init(renderer_traits* traits);
void render_buffer_uninit();
void render_buffer_begin_gamma_pass();
void render_buffer_clear();
void render_buffer_execute(SDL_GPUCommandBuffer* cb);

// @sampler_factory
void sampler_factory_init(renderer_traits* traits, SDL_GPUDevice* device);
void sampler_factory_uninit();
SDL_GPUSampler* sampler_factory_sampler(texture_t* texture);

// @pipeline_factory
void pipeline_factory_init(SDL_Window* window, SDL_GPUDevice* device);
void pipeline_factory_uninit();
SDL_GPUGraphicsPipeline* pipeline_factory_pipeline(shader_t* shader, bool msaa, bool shadow);

// @material
void material_bind_gpu(material_t* material, SDL_GPUCommandBuffer* cb);

// @mesh
void mesh_init(renderer_traits* traits, SDL_GPUDevice* device);
void mesh_uninit();
void mesh_render(mesh_t* mesh, SDL_GPURenderPass* pass);

// @texture
void texture_init(renderer_traits* traits, SDL_GPUDevice* dev);
void texture_uninit();
SDL_GPUTexture* texture_gpu_texture(texture_t* texture);
sampler_options_t texture_sampler_options(texture_t* texture);

// @shader
void shader_init(renderer_traits* traits, SDL_GPUDevice* device);
void shader_uninit();

// New naming convention
SDL_GPUShader* shader_gpu_vertex_shader(shader_t* shader);
SDL_GPUShader* shader_gpu_fragment_shader(shader_t* shader);
SDL_GPUCullMode shader_gpu_cull_mode(shader_t* shader);
bool shader_blend_enabled(shader_t* shader);
SDL_GPUBlendFactor shader_gpu_src_blend(shader_t* shader);
SDL_GPUBlendFactor shader_gpu_dst_blend(shader_t* shader);
bool shader_depth_test_enabled(shader_t* shader);
bool shader_depth_write_enabled(shader_t* shader);
int shader_vertex_uniform_count(shader_t* shader);
int shader_fragment_uniform_count(shader_t* shader);
int shader_sampler_count(shader_t* shader);
name_t* shader_name(shader_t* shader);

// @font
void font_init(renderer_traits* traits, SDL_GPUDevice* device);
void font_uninit();
material_t* font_material(font_t* font);

// @camera
void camera_init();
void camera_uninit();

// @animation
void animation_evaluate_frame(
    animation_t* animation,
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
