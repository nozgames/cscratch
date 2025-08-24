//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
typedef struct object_impl texture_t;
typedef struct object_impl material_t;
typedef struct object_impl mesh_t;
typedef struct object_impl mesh_builder_t;
typedef struct object_impl material_t;
typedef struct object_impl font_t;
typedef struct object_impl animation_t;
typedef struct object_impl shader_t;
typedef struct object_impl camera_t;

// @renderer_traits
typedef struct renderer_traits
{
    size_t max_textures;
    size_t max_shaders;
    size_t max_samplers;
    size_t max_meshes;
    size_t max_fonts;
    size_t max_frame_commands;
    size_t max_frame_objects;
    size_t max_frame_transforms;
    uint32_t shadow_map_size;

} renderer_traits;

// @renderer


// @texture

typedef enum texture_filter
{
    texture_filter_nearest,
    texture_filter_linear
} texture_filter_t;

typedef enum texture_clamp
{
    texture_clamp_repeat,
    texture_clamp_clamp,
    texture_clamp_repeat_mirrored
} texture_clamp_t;

typedef enum texture_format
{
    texture_format_rgba8,
    texture_format_rgba16f,
    texture_format_r8
} texture_format_t;

texture_t* texture_load(allocator_t* allocator, name_t* path);
texture_t* texture_alloc_raw(allocator_t* allocator, void* data, size_t width, size_t height, texture_format_t format, name_t* name);
texture_t* texture_alloc_render_target(allocator_t* allocator, int width, int height, texture_format_t format, name_t* name);
int texture_bytes_per_pixel(texture_format_t format);
ivec2 texture_size(texture_t* texture);

// @font
font_t* font_load(allocator_t* allocator, name_t* name);

// @shader
shader_t* shader_load(allocator_t* allocator, name_t* name);
name_t* shader_name(shader_t* shader);

// @material
material_t* material_alloc(allocator_t* allocator, shader_t* shader, name_t* name);
name_t* material_name(material_t* material);
shader_t* material_shader(material_t* material);
void material_set_texture(material_t* material, texture_t* texture, size_t index);

// @mesh
mesh_t* mesh_alloc_raw(
	allocator_t* allocator,
	size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uvs,
	uint8_t* bone_indices,
    size_t index_count,
	uint16_t* indices,
    name_t* name);
mesh_t* mesh_alloc_from_mesh_builder(allocator_t* allocator, mesh_builder_t* builder, name_t* name);

// @mesh_builder
mesh_builder_t* mesh_builder_alloc(allocator_t* allocator, int max_vertices, int max_indices);
void mesh_builder_clear(mesh_builder_t* builder);
vec3* mesh_builder_positions(mesh_builder_t* builder);
vec3* mesh_builder_normals(mesh_builder_t* builder);
vec2* mesh_builder_uv0(mesh_builder_t* builder);
uint8_t* mesh_builder_bones(mesh_builder_t* builder);
uint16_t* mesh_builder_indices(mesh_builder_t* builder);
size_t mesh_builder_vertex_count(mesh_builder_t* builder);
size_t mesh_builder_index_count(mesh_builder_t* builder);
void mesh_builder_add_index(mesh_builder_t* builder, uint16_t index);
void mesh_builder_add_triangle_indices(mesh_builder_t* builder, uint16_t a, uint16_t b, uint16_t c);
void mesh_builder_add_triangle(mesh_builder_t* builder, vec3 a, vec3 b, vec3 c, uint8_t bone_index);
void mesh_builder_add_pyramid(mesh_builder_t* builder, vec3 start, vec3 end, float size, uint8_t bone_index);
void mesh_builder_add_cube(mesh_builder_t* builder, vec3 center, vec3 size, uint8_t bone_index);
void mesh_builder_add_raw(
    mesh_builder_t* builder,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uv0,
    uint8_t bone_index,
    size_t index_count,
    uint16_t* indices);
void mesh_builder_add_quad(
    mesh_builder_t* builder,
    vec3 forward,
    vec3 right,
    float width,
    float height,
    vec2 color_uv);
void mesh_builder_add_quad_points(
    mesh_builder_t* builder,
    vec3 a,
    vec3 b,
    vec3 c,
    vec3 d,
    vec2 uv_color,
    vec3 normal,
    uint8_t bone_index);

// @render_buffer
void render_buffer_clear(void);
void render_buffer_begin_pass(bool clear, color_t clear_color, bool msaa, texture_t* target);
void render_buffer_begin_shadow_pass(mat4 light_view, mat4 light_projection);
void render_buffer_bind_default_texture(int texture_index);
void render_buffer_bind_camera(camera_t* camera);
void render_buffer_bind_camera_matrices(mat4 view, mat4 projection);
void render_buffer_bind_transform(mat4 transform);
void render_buffer_bind_material(material_t* material);
void render_buffer_render_mesh(mesh_t* mesh);
void render_buffer_end_pass(void);