//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct Camera;
struct Texture : Object {};
struct Material : Object {};
struct Mesh : Object {};
struct Font : Object {};
struct Shader : Object {};
struct MeshBuilder : Object {};
struct Animation : Object {};

// @renderer_traits
struct RendererTraits
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
};

// @texture

enum TextureFilter
{
    texture_filter_nearest,
    texture_filter_linear
};

enum TextureClamp
{
    texture_clamp_repeat,
    texture_clamp_clamp,
    texture_clamp_repeat_mirrored
};

enum TextureFormat
{
    texture_format_rgba8,
    texture_format_rgba16f,
    texture_format_r8
};

Texture* LoadTexture(Allocator* allocator, name_t* path);
Texture* AllocTexture(Allocator* allocator, void* data, size_t width, size_t height, TextureFormat format, const name_t* name);
Texture* AllocTexture(Allocator* allocator, int width, int height, TextureFormat format, const name_t* name);
int texture_bytes_per_pixel(TextureFormat format);
ivec2 GetSize(Texture* texture);

// @font
Font* font_load(Allocator* allocator, name_t* name);

// @shader
Shader* LoadShader(Allocator* allocator, const name_t* name);
const name_t* GetName(Shader* shader);

// @material
Material* material_alloc(Allocator* allocator, Shader* shader, name_t* name);
name_t* material_name(Material* material);
Shader* material_shader(Material* material);
void material_set_texture(Material* material, Texture* texture, size_t index);

// @mesh
Mesh* mesh_alloc_raw(
	Allocator* allocator,
	size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uvs,
	uint8_t* bone_indices,
    size_t index_count,
	uint16_t* indices,
    name_t* name);
Mesh* mesh_alloc_from_mesh_builder(Allocator* allocator, MeshBuilder* builder, name_t* name);

// @mesh_builder
MeshBuilder* mesh_builder_alloc(Allocator* allocator, int max_vertices, int max_indices);
void mesh_builder_clear(MeshBuilder* builder);
vec3* mesh_builder_positions(MeshBuilder* builder);
vec3* mesh_builder_normals(MeshBuilder* builder);
vec2* mesh_builder_uv0(MeshBuilder* builder);
uint8_t* mesh_builder_bones(MeshBuilder* builder);
uint16_t* mesh_builder_indices(MeshBuilder* builder);
size_t mesh_builder_vertex_count(MeshBuilder* builder);
size_t mesh_builder_index_count(MeshBuilder* builder);
void mesh_builder_add_index(MeshBuilder* builder, uint16_t index);
void mesh_builder_add_triangle_indices(MeshBuilder* builder, uint16_t a, uint16_t b, uint16_t c);
void mesh_builder_add_triangle(MeshBuilder* builder, vec3 a, vec3 b, vec3 c, uint8_t bone_index);
void mesh_builder_add_pyramid(MeshBuilder* builder, vec3 start, vec3 end, float size, uint8_t bone_index);
void mesh_builder_add_cube(MeshBuilder* builder, vec3 center, vec3 size, uint8_t bone_index);
void mesh_builder_add_raw(
    MeshBuilder* builder,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uv0,
    uint8_t bone_index,
    size_t index_count,
    uint16_t* indices);
void mesh_builder_add_quad(
    MeshBuilder* builder,
    vec3 forward,
    vec3 right,
    float width,
    float height,
    vec2 color_uv);
void mesh_builder_add_quad_points(
    MeshBuilder* builder,
    vec3 a,
    vec3 b,
    vec3 c,
    vec3 d,
    vec2 uv_color,
    vec3 normal,
    uint8_t bone_index);

// @render_buffer
void render_buffer_clear(void);
void BeginRenderPass(bool clear, color_t clear_color, bool msaa, Texture* target);
void render_buffer_begin_shadow_pass(mat4 light_view, mat4 light_projection);
void render_buffer_bind_default_texture(int texture_index);
void render_buffer_bind_camera(Camera* camera);
void render_buffer_bind_camera_matrices(mat4 view, mat4 projection);
void render_buffer_bind_transform(mat4 transform);
void render_buffer_bind_material(Material* material);
void render_buffer_render_mesh(Mesh* mesh);
void EndRenderPass(void);