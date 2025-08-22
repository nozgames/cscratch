#pragma once

// @types
typedef struct texture* texture_t;
typedef struct material* material_t;
typedef struct mesh* mesh_t;
typedef struct mesh_builder* mesh_builder_t;
typedef struct material* material_t;
typedef struct font* font_t;
typedef struct animation* animation_t;
typedef struct shader* shader_t;
typedef struct camera* camera_t;

// @renderer_traits
typedef struct renderer_traits
{
    size_t max_textures;
    size_t max_shaders;
    size_t max_samplers;
    size_t max_meshes;
    size_t max_fonts;
    uint32_t shadow_map_size;
    size_t max_frame_commands;
    size_t max_frame_objects;
    size_t max_frame_transforms;

} renderer_traits;

// @renderer

// @texture
typedef struct texture* texture_t;

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

texture_t texture_create_raw(const uint8_t* data, size_t width, size_t height, texture_format_t format, const char* name);
texture_t texture_create_render_target(int width, int height, texture_format_t format, const char* name);
int texture_bytes_per_pixel(texture_format_t format);
ivec2_t texture_size(texture_t texture);

// @shader
shader_t shader_load(const char* name);

// @material
material_t material_create(shader_t shader, const char* name);
const char* material_name(material_t material);
shader_t material_shader(material_t material);
void material_set_texture(material_t material, texture_t texture, size_t index);

// @mesh
mesh_t mesh_create_raw(
    mesh_builder_t builder,
    const char* name,
	size_t vertex_count,
    vec3_t* positions,
    vec3_t* normals,
    vec2_t* uvs,
	uint8_t* bone_indices,
    size_t index_count,
	uint16_t* indices);

// @mesh_builder
mesh_builder_t mesh_builder_create(int max_vertices, int max_indices);
void mesh_builder_destroy(mesh_builder_t builder);
void mesh_builder_clear(mesh_builder_t builder);
mesh_t mesh_builder_to_mesh(mesh_builder_t builder, const char* name);
const vec3_t* mesh_builder_positions(mesh_builder_t builder);
const vec3_t* mesh_builder_normals(mesh_builder_t builder);
const vec2_t* mesh_builder_uv0(mesh_builder_t builder);
const uint8_t* mesh_builder_bones(mesh_builder_t builder);
const uint16_t* mesh_builder_indices(mesh_builder_t builder);
size_t mesh_builder_vertex_count(mesh_builder_t builder);
size_t mesh_builder_index_count(mesh_builder_t builder);
void mesh_builder_add_index(mesh_builder_t builder, uint16_t index);
void mesh_builder_add_triangle_indices(mesh_builder_t builder, uint16_t a, uint16_t b, uint16_t c);
void mesh_builder_add_triangle(mesh_builder_t builder, vec3_t a, vec3_t b, vec3_t c, uint8_t bone_index);
void mesh_builder_add_pyramid(mesh_builder_t builder, vec3_t start, vec3_t end, float size, uint8_t bone_index);
void mesh_builder_add_cube(mesh_builder_t builder, vec3_t center, vec3_t size, uint8_t bone_index);
void mesh_builder_add_raw(
    mesh_builder_t builder,
    size_t vertex_count,
    vec3_t* positions,
    vec3_t* normals,
    vec2_t* uv0,
    uint8_t bone_index,
    size_t index_count,
    uint16_t* indices);
void mesh_builder_add_quad(
    mesh_builder_t builder,
    vec3_t forward,
    vec3_t right,
    float width,
    float height,
    vec2_t color_uv);
void mesh_builder_add_quad_points(
    mesh_builder_t builder,
    vec3_t a,
    vec3_t b,
    vec3_t c,
    vec3_t d,
    vec2_t uv_color,
    vec3_t normal,
    uint8_t bone_index);

// @render_buffer
void render_buffer_clear();
void render_buffer_begin_pass(bool clear, color_t clear_color, bool msaa, texture_t target);
void render_buffer_begin_shadow_pass(mat4_t light_view, mat4_t light_projection);
void render_buffer_bind_default_texture(int texture_index);
void render_buffer_bind_camera(camera_t camera);
void render_buffer_bind_camera_matrices(mat4_t view, mat4_t projection);
void render_buffer_bind_transform(mat4_t transform);
void render_buffer_bind_material(material_t material);
void render_buffer_render_mesh(mesh_t mesh);
void render_buffer_end_pass();