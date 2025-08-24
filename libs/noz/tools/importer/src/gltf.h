//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// Forward declarations
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;
struct cgltf_animation;
struct cgltf_animation_channel;
struct cgltf_skin;
struct cgltf_primitive;
struct cgltf_accessor;

// @types
typedef struct gltf_bone
{
    name_t name;
    int index;
    int parent_index;
    mat4 world_to_local;
    mat4 local_to_world;
    vec3 position;
    quat rotation;
    vec3 scale;
    float length;
    vec3 direction;
} gltf_bone_t;

typedef struct gltf_animation
{
    int frame_count;
    int frame_stride;
    list_t* tracks;  // list of animation_track_t
    float* data;
    size_t data_size;
} gltf_animation_t;

typedef struct gltf_mesh
{
    vec3* positions;
    size_t position_count;
    vec3* normals;
    size_t normal_count;
    vec2* uvs;
    size_t uv_count;
    uint32_t* bone_indices;
    size_t bone_index_count;
    uint16_t* indices;
    size_t index_count;
} gltf_mesh_t;

typedef struct gltf_bone_filter
{
    list_t* exclude_bones;  // list of name_t
    bool keep_leaf_bones;
} gltf_bone_filter_t;

typedef struct gltf
{
    struct cgltf_data* data;
    path_t path;
} gltf_t;

// @init
gltf_t* gltf_alloc(allocator_t* allocator);
void gltf_free(gltf_t* gltf);

// @file
bool gltf_open(gltf_t* gltf, path_t* path);
void gltf_close(gltf_t* gltf);

// @filter
gltf_bone_filter_t* gltf_bone_filter_alloc(allocator_t* allocator);
void gltf_bone_filter_free(gltf_bone_filter_t* filter);
gltf_bone_filter_t* gltf_bone_filter_from_meta_file(gltf_bone_filter_t* filter, path_t* meta_path);

// @bones
list_t* gltf_read_bones(gltf_t* gltf, gltf_bone_filter_t* filter, allocator_t* allocator);

// @animation
gltf_animation_t* gltf_read_animation(gltf_t* gltf, list_t* bones, name_t* animation_name, allocator_t* allocator);
void gltf_animation_free(gltf_animation_t* animation);

// @mesh
gltf_mesh_t* gltf_read_mesh(gltf_t* gltf, list_t* bones, allocator_t* allocator);
void gltf_mesh_free(gltf_mesh_t* mesh);