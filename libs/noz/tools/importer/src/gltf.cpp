//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "gltf.h"
#include "../../../src/internal.h"
#include <noz/allocator.h>
#include <noz/object.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

// Forward declarations
static void read_bones_recursive(
    gltf_t* gltf,
    struct cgltf_node* node,
    List* bones,
    int parent_index,
    gltf_bone_filter_t* filter,
    Allocator* allocator);

static bool read_bone(
    gltf_t* gltf,
    struct cgltf_node* node,
    List* bones,
    int parent_index,
    gltf_bone_filter_t* filter,
    Allocator* allocator);

static bool is_bone_leaf(struct cgltf_node* node, List* exclude_bones);
static void update_parent_bone_length(gltf_bone_t* bone, gltf_bone_t* parent_bone);
static int read_frame_count(struct cgltf_animation* animation, List* bones);
static bool is_track_defaults(struct cgltf_accessor* accessor, animation_track_type_t track_type, gltf_bone_t* bone);

// Buffer access helpers
static void* buffer_data(struct cgltf_accessor* accessor);
static size_t buffer_stride(struct cgltf_accessor* accessor);

// Conversion helpers
static mat4 convert_matrix(float* matrix);
static vec3 convert_vector3(float* vector);
static vec2 convert_vector2(float* vector2);
static vec2 convert_uv(float* vector2);
static quat convert_quaternion(float* quaternion);

// @init
gltf_t* gltf_alloc(Allocator* allocator)
{
    gltf_t* gltf = (gltf_t*)Alloc(allocator, sizeof(gltf_t));
    if (!gltf)
        return NULL;
    
    memset(gltf, 0, sizeof(gltf_t));
    return gltf;
}

void gltf_free(gltf_t* gltf)
{
    if (!gltf)
        return;
    
    gltf_close(gltf);
    // Note: We don't free the gltf itself as it was allocated by the allocator
}

// @file
bool gltf_open(gltf_t* gltf, Path* path)
{
    if (!gltf || !path)
        return false;
    
    gltf_close(gltf);
    
    cgltf_options options = {};
    struct cgltf_data* data = NULL;
    
    cgltf_result result = cgltf_parse_file(&options, path->value, &data);
    if (result != cgltf_result_success)
        return false;
    
    result = cgltf_load_buffers(&options, data, path->value);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }
    
    gltf->data = data;
    path_copy(&gltf->path, path);
    
    return true;
}

void gltf_close(gltf_t* gltf)
{
    if (!gltf || !gltf->data)
        return;
    
    cgltf_free(gltf->data);
    gltf->data = NULL;
    path_clear(&gltf->path);
}

// @filter
gltf_bone_filter_t* gltf_bone_filter_alloc(Allocator* allocator)
{
    gltf_bone_filter_t* filter = (gltf_bone_filter_t*)Alloc(allocator, sizeof(gltf_bone_filter_t));
    if (!filter)
        return NULL;
    
    filter->exclude_bones = CreateList(allocator, 16);
    filter->keep_leaf_bones = false;
    return filter;
}

void gltf_bone_filter_free(gltf_bone_filter_t* filter)
{
    if (!filter)
        return;
    
    // The list and filter will be freed by the allocator
}

gltf_bone_filter_t* gltf_bone_filter_from_meta_file(gltf_bone_filter_t* filter, Path* meta_path)
{
    // TODO: Implement meta file parsing when the meta_file API is available in C
    // For now, return the filter unchanged
    (void)meta_path;
    return filter;
}

// @bones
List* gltf_read_bones(gltf_t* gltf, gltf_bone_filter_t* filter, Allocator* allocator)
{
    if (!gltf || !gltf->data || !allocator)
        return NULL;
    
    List* bones = CreateList(allocator, 64);
    if (!bones)
        return NULL;
    
    struct cgltf_data* data = (struct cgltf_data*)gltf->data;
    if (!data->nodes_count)
        return bones;
    
    // Find the root node
    struct cgltf_node* root = NULL;
    for (size_t i = 0; i < data->nodes_count && !root; ++i)
    {
        struct cgltf_node* node = &data->nodes[i];
        if (node->name && strcmp(node->name, "root") == 0)
            root = node;
    }
    
    if (!root)
        return bones;
    
    read_bones_recursive(gltf, root, bones, -1, filter, allocator);
    
    return bones;
}

static void read_bones_recursive(
    gltf_t* gltf,
    struct cgltf_node* node,
    List* bones,
    int parent_index,
    gltf_bone_filter_t* filter,
    Allocator* allocator)
{
    if (!node)
        return;
    
    // Check if this bone should be excluded
    if (filter && filter->exclude_bones)
    {
        for (size_t i = 0; i < GetCount(filter->exclude_bones); ++i)
        {
            name_t* exclude_name = (name_t*)GetAt(filter->exclude_bones, i);
            if (exclude_name && node->name && name_eq_cstr(exclude_name, node->name))
                return;
        }
    }
    
    // Check if this is a leaf bone and we don't want to keep them
    if (filter && !filter->keep_leaf_bones && is_bone_leaf(node, filter ? filter->exclude_bones : NULL))
        return;
    
    if (read_bone(gltf, node, bones, parent_index, filter, allocator))
    {
        // Get the index of the bone we just added
        int bone_index = (int)GetCount(bones) - 1;
        
        // Process children
        for (size_t i = 0; i < node->children_count; ++i)
            read_bones_recursive(gltf, node->children[i], bones, bone_index, filter, allocator);
    }
}

static bool read_bone(
    gltf_t* gltf,
    struct cgltf_node* node,
    List* bones,
    int parent_index,
    gltf_bone_filter_t* filter,
    Allocator* allocator)
{
    (void)gltf;
    (void)filter;
    
    assert(node);
    
    gltf_bone_t* bone = (gltf_bone_t*)Alloc(allocator, sizeof(gltf_bone_t));
    if (!bone)
        return false;
    
    memset(bone, 0, sizeof(gltf_bone_t));
    
    if (node->name)
        name_set(&bone->name, node->name);
    else
        name_set(&bone->name, "");
    
    bone->index = (int)GetCount(bones);
    bone->parent_index = parent_index;
    bone->position = convert_vector3(node->translation);
    bone->rotation = convert_quaternion(node->rotation);
    bone->scale = convert_vector3(node->scale);
    
    // Build local_to_world transform
    bone->local_to_world =
        glm::translate(glm::mat4(1.0f), bone->position) *
        glm::mat4_cast(bone->rotation) *
        glm::scale(glm::mat4(1.0f), bone->scale);

    if (parent_index >= 0)
        bone->local_to_world = ((gltf_bone_t*)GetAt(bones, parent_index))->local_to_world * bone->local_to_world;

    bone->world_to_local = inverse(bone->world_to_local);
    bone->length = 0.0f;
    bone->direction = VEC3_UP;
    
    Add(bones, bone);
    
    return true;
}

static bool is_bone_leaf(struct cgltf_node* node, List* exclude_bones)
{
    (void)exclude_bones;
    
    assert(node);
    if (!node->name || node->name[0] == 0)
        return false;
    
    // Check if the name ends in "_leaf"
    const char* leaf_suffix = "_leaf";
    size_t name_len = strlen(node->name);
    size_t suffix_len = strlen(leaf_suffix);
    
    if (name_len >= suffix_len)
    {
        const char* name_end = node->name + name_len - suffix_len;
        if (strcmp(name_end, leaf_suffix) == 0)
            return true;
    }
    
    return false;
}

static void update_parent_bone_length(gltf_bone_t* bone, gltf_bone_t* parent_bone)
{
    vec3 diff = bone->position - parent_bone->position;
    parent_bone->direction = normalize(diff);
    parent_bone->length = length(diff);
}

// @animation
gltf_animation_t* gltf_read_animation(gltf_t* gltf, List* bones, name_t* animation_name, Allocator* allocator)
{
    (void)animation_name; // TODO: Use animation name to select specific animation
    
    if (!gltf || !gltf->data || !bones || !allocator)
        return NULL;
    
    struct cgltf_data* data = (struct cgltf_data*)gltf->data;
    if (!data->animations_count)
        return NULL;
    
    // For now, just read the first animation
    struct cgltf_animation* cgltf_anim = &data->animations[0];
    
    gltf_animation_t* animation = (gltf_animation_t*)Alloc(allocator, sizeof(gltf_animation_t));
    if (!animation)
        return NULL;
    
    memset(animation, 0, sizeof(gltf_animation_t));
    
    animation->frame_count = read_frame_count(cgltf_anim, bones);
    animation->tracks = CreateList(allocator, 32);
    
    // First pass: determine tracks and calculate data size
    size_t data_offset = 0;
    for (size_t i = 0; i < cgltf_anim->channels_count; ++i)
    {
        struct cgltf_animation_channel* channel = &cgltf_anim->channels[i];
        if (!channel->target_node || !channel->sampler)
            continue;
        
        // Find bone index
        const char* bone_name = channel->target_node->name ? channel->target_node->name : "";
        int bone_index = -1;
        for (size_t j = 0; j < GetCount(bones); ++j)
        {
            gltf_bone_t* bone = (gltf_bone_t*)GetAt(bones, j);
            if (bone && name_eq_cstr(&bone->name, bone_name))
            {
                bone_index = (int)j;
                break;
            }
        }
        
        if (bone_index == -1)
            continue;
        
        // Determine track type
        animation_track_type_t track_type = animation_track_type_translation;
        if (channel->target_path == cgltf_animation_path_type_rotation)
            track_type = animation_track_type_rotation;
        else if (channel->target_path == cgltf_animation_path_type_scale)
            track_type = animation_track_type_scale;
        
        // Check if this track matches bone's local values
        struct cgltf_accessor* accessor = channel->sampler->output;
        gltf_bone_t* bone = (gltf_bone_t*)GetAt(bones, (size_t)bone_index);
        if (is_track_defaults(accessor, track_type, bone))
            continue;
        
        // Create track
        animation_track_t* track = (animation_track_t*)Alloc(allocator, sizeof(animation_track_t));
        if (!track)
        {
            gltf_animation_free(animation);
            return NULL;
        }
        
        track->bone = (uint8_t)bone_index;
        track->type = track_type;
        track->data_offset = (int)data_offset;
        
        data_offset += track_type == animation_track_type_rotation ? 4 : 3;
        
        Add(animation->tracks, track);
    }
    
    animation->frame_stride = (int)data_offset;
    animation->data_size = data_offset * animation->frame_count;
    animation->data = (float*)Alloc(allocator, animation->data_size * sizeof(float));
    
    if (!animation->data)
    {
        gltf_animation_free(animation);
        return NULL;
    }
    
    // Second pass: read track data
    size_t track_idx = 0;
    for (size_t i = 0; i < cgltf_anim->channels_count; ++i)
    {
        struct cgltf_animation_channel* channel = &cgltf_anim->channels[i];
        if (!channel->target_node || !channel->sampler)
            continue;
        
        // Skip if we didn't create a track for this channel
        if (track_idx >= GetCount(animation->tracks))
            break;
        
        animation_track_t* track = (animation_track_t*)GetAt(animation->tracks, track_idx);
        if (!track)
            continue;
        
        struct cgltf_accessor* accessor = channel->sampler->output;
        void* data = buffer_data(accessor);
        size_t stride = buffer_stride(accessor);
        size_t count = accessor->count;
        size_t component_count = track->type == animation_track_type_rotation ? 4 : 3;
        
        for (size_t j = 0; j < count; ++j)
        {
            float* src = (float*)((char*)data + j * stride);
            float* dst = animation->data + track->data_offset + j * animation->frame_stride;
            memcpy(dst, src, sizeof(float) * component_count);
        }
        
        track_idx++;
    }
    
    return animation;
}

void gltf_animation_free(gltf_animation_t* animation)
{
    // Memory will be freed by the allocator
    (void)animation;
}

// @mesh
gltf_mesh_t* gltf_read_mesh(gltf_t* gltf, List* bones, Allocator* allocator)
{
    if (!gltf || !gltf->data || !allocator)
        return NULL;
    
    struct cgltf_data* data = (struct cgltf_data*)gltf->data;
    if (!data->meshes_count)
        return NULL;
    
    struct cgltf_mesh* cgltf_mesh = &data->meshes[0];
    struct cgltf_skin* skin = data->skins_count > 0 ? &data->skins[0] : NULL;
    
    gltf_mesh_t* mesh = (gltf_mesh_t*)Alloc(allocator, sizeof(gltf_mesh_t));
    if (!mesh)
        return NULL;
    
    memset(mesh, 0, sizeof(gltf_mesh_t));
    
    // Build joint to bone index mapping
    int* joint_to_bone_index = NULL;
    if (skin && bones && GetCount(bones) > 0)
    {
        joint_to_bone_index = (int*)Alloc(allocator, skin->joints_count * sizeof(int));
        if (joint_to_bone_index)
        {
            for (size_t i = 0; i < skin->joints_count; ++i)
            {
                joint_to_bone_index[i] = -1;
                struct cgltf_node* joint_node = skin->joints[i];
                if (joint_node && joint_node->name)
                {
                    for (size_t j = 0; j < GetCount(bones); ++j)
                    {
                        gltf_bone_t* bone = (gltf_bone_t*)GetAt(bones, j);
                        if (bone && name_eq_cstr(&bone->name, joint_node->name))
                        {
                            joint_to_bone_index[i] = (int)j;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    if (!cgltf_mesh->primitives_count)
        return mesh;
    
    // Read first primitive
    struct cgltf_primitive* primitive = &cgltf_mesh->primitives[0];
    
    // Read attributes
    for (size_t i = 0; i < primitive->attributes_count; ++i)
    {
        struct cgltf_attribute* attr = &primitive->attributes[i];
        
        if (attr->type == cgltf_attribute_type_position)
        {
            struct cgltf_accessor* accessor = attr->data;
            mesh->position_count = accessor->count;
            mesh->positions = (vec3*)Alloc(allocator, mesh->position_count * sizeof(vec3));
            
            if (mesh->positions)
            {
                void* data = buffer_data(accessor);
                size_t stride = buffer_stride(accessor);
                
                for (size_t j = 0; j < mesh->position_count; ++j)
                {
                    float* vertex_data = (float*)((char*)data + j * stride);
                    mesh->positions[j] = convert_vector3(vertex_data);
                }
            }
        }
        else if (attr->type == cgltf_attribute_type_normal)
        {
            struct cgltf_accessor* accessor = attr->data;
            mesh->normal_count = accessor->count;
            mesh->normals = (vec3*)Alloc(allocator, mesh->normal_count * sizeof(vec3));
            
            if (mesh->normals)
            {
                void* data = buffer_data(accessor);
                size_t stride = buffer_stride(accessor);
                
                for (size_t j = 0; j < mesh->normal_count; ++j)
                {
                    float* vertex_data = (float*)((char*)data + j * stride);
                    mesh->normals[j] = convert_vector3(vertex_data);
                }
            }
        }
        else if (attr->type == cgltf_attribute_type_texcoord)
        {
            struct cgltf_accessor* accessor = attr->data;
            mesh->uv_count = accessor->count;
            mesh->uvs = (vec2*)Alloc(allocator, mesh->uv_count * sizeof(vec2));
            
            if (mesh->uvs)
            {
                void* data = buffer_data(accessor);
                size_t stride = buffer_stride(accessor);
                
                for (size_t j = 0; j < mesh->uv_count; ++j)
                {
                    float* vertex_data = (float*)((char*)data + j * stride);
                    mesh->uvs[j] = convert_uv(vertex_data);
                }
            }
        }
        else if (attr->type == cgltf_attribute_type_joints)
        {
            struct cgltf_accessor* accessor = attr->data;
            mesh->bone_index_count = accessor->count;
            mesh->bone_indices = (uint32_t*)Alloc(allocator, mesh->bone_index_count * sizeof(uint32_t));
            
            if (mesh->bone_indices)
            {
                void* data = buffer_data(accessor);
                size_t stride = buffer_stride(accessor);
                
                for (size_t j = 0; j < mesh->bone_index_count; ++j)
                {
                    uint32_t joint_index = 0;
                    
                    if (accessor->component_type == cgltf_component_type_r_8u)
                    {
                        uint8_t* joint_data = (uint8_t*)((char*)data + j * stride);
                        joint_index = (uint32_t)joint_data[0];
                    }
                    else if (accessor->component_type == cgltf_component_type_r_16u)
                    {
                        uint16_t* joint_data = (uint16_t*)((char*)data + j * stride);
                        joint_index = (uint32_t)joint_data[0];
                    }
                    else if (accessor->component_type == cgltf_component_type_r_32u)
                    {
                        uint32_t* joint_data = (uint32_t*)((char*)data + j * stride);
                        joint_index = joint_data[0];
                    }
                    
                    // Map joint index to bone index
                    if (joint_to_bone_index && joint_index < skin->joints_count)
                    {
                        int bone_idx = joint_to_bone_index[joint_index];
                        mesh->bone_indices[j] = (bone_idx >= 0) ? (uint32_t)bone_idx : 0;
                    }
                    else
                    {
                        mesh->bone_indices[j] = joint_index;
                    }
                }
            }
        }
    }
    
    // Read indices
    if (primitive->indices)
    {
        struct cgltf_accessor* accessor = primitive->indices;
        mesh->index_count = accessor->count;
        mesh->indices = (uint16_t*)Alloc(allocator, mesh->index_count * sizeof(uint16_t));
        
        if (mesh->indices)
        {
            void* data = buffer_data(accessor);
            size_t stride = buffer_stride(accessor);
            
            for (size_t i = 0; i < mesh->index_count; ++i)
            {
                if (accessor->component_type == cgltf_component_type_r_16u)
                {
                    uint16_t* index_data = (uint16_t*)((char*)data + i * stride);
                    mesh->indices[i] = *index_data;
                }
                else if (accessor->component_type == cgltf_component_type_r_32u)
                {
                    uint32_t* index_data = (uint32_t*)((char*)data + i * stride);
                    mesh->indices[i] = (uint16_t)(*index_data);
                }
            }
        }
    }
    
    return mesh;
}

void gltf_mesh_free(gltf_mesh_t* mesh)
{
    // Memory will be freed by the allocator
    (void)mesh;
}

// Helper functions
static int read_frame_count(struct cgltf_animation* animation, List* bones)
{
    (void)bones;
    
    if (!animation)
        return 0;
    
    int max_frames = 0;
    
    for (size_t i = 0; i < animation->channels_count; ++i)
    {
        struct cgltf_animation_channel* channel = &animation->channels[i];
        if (channel->sampler && channel->sampler->input)
        {
            int frames = (int)channel->sampler->input->count;
            if (frames > max_frames)
                max_frames = frames;
        }
    }
    
    return max_frames;
}

static bool is_track_defaults(struct cgltf_accessor* accessor, animation_track_type_t track_type, gltf_bone_t* bone)
{
    if (!accessor || !bone)
        return true;
    
    void* data = buffer_data(accessor);
    if (!data)
        return true;
    
    size_t stride = buffer_stride(accessor);
    size_t count = accessor->count;
    
    // Check if all values match the bone's local transform
    for (size_t i = 0; i < count; ++i)
    {
        float* values = (float*)((char*)data + i * stride);
        
        switch (track_type)
        {
        case animation_track_type_translation:
            if (fabsf(values[0] - bone->position.x) > 0.0001f ||
                fabsf(values[1] - bone->position.y) > 0.0001f ||
                fabsf(values[2] - bone->position.z) > 0.0001f)
            {
                return false;
            }
            break;
            
        case animation_track_type_rotation:
            if (fabsf(values[0] - bone->rotation.x) > 0.0001f ||
                fabsf(values[1] - bone->rotation.y) > 0.0001f ||
                fabsf(values[2] - bone->rotation.z) > 0.0001f ||
                fabsf(values[3] - bone->rotation.w) > 0.0001f)
            {
                return false;
            }
            break;
            
        case animation_track_type_scale:
            if (fabsf(values[0] - bone->scale.x) > 0.0001f ||
                fabsf(values[1] - bone->scale.y) > 0.0001f ||
                fabsf(values[2] - bone->scale.z) > 0.0001f)
            {
                return false;
            }
            break;
        }
    }
    
    return true;
}

static void* buffer_data(struct cgltf_accessor* accessor)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer)
        return NULL;
    
    struct cgltf_buffer_view* view = accessor->buffer_view;
    struct cgltf_buffer* buffer = view->buffer;
    
    return (char*)buffer->data + view->offset + accessor->offset;
}

static size_t buffer_stride(struct cgltf_accessor* accessor)
{
    if (!accessor || !accessor->buffer_view)
        return 0;
    
    if (accessor->buffer_view->stride)
        return accessor->buffer_view->stride;
    
    // Calculate default stride
    size_t component_size = 0;
    switch (accessor->component_type)
    {
    case cgltf_component_type_r_8:
    case cgltf_component_type_r_8u:
        component_size = 1;
        break;
    case cgltf_component_type_r_16:
    case cgltf_component_type_r_16u:
        component_size = 2;
        break;
    case cgltf_component_type_r_32u:
    case cgltf_component_type_r_32f:
        component_size = 4;
        break;
    default:
        break;
    }
    
    size_t num_components = 0;
    switch (accessor->type)
    {
    case cgltf_type_scalar:
        num_components = 1;
        break;
    case cgltf_type_vec2:
        num_components = 2;
        break;
    case cgltf_type_vec3:
        num_components = 3;
        break;
    case cgltf_type_vec4:
        num_components = 4;
        break;
    case cgltf_type_mat2:
        num_components = 4;
        break;
    case cgltf_type_mat3:
        num_components = 9;
        break;
    case cgltf_type_mat4:
        num_components = 16;
        break;
    default:
        break;
    }
    
    return component_size * num_components;
}

static mat4 convert_matrix(float* matrix)
{
    if (!matrix)
        return mat4(1.0f);

    return mat4(
        matrix[0], matrix[1], matrix[2], matrix[3],
        matrix[4], matrix[5], matrix[6], matrix[7],
        matrix[8], matrix[9], matrix[10], matrix[11],
        matrix[12], matrix[13], matrix[14], matrix[15]
    );
}

static vec3 convert_vector3(float* vector)
{
    if (!vector)
        return vec3(0.0f);

    return vec3(vector[0], vector[2], vector[1]);
}

static vec2 convert_vector2(float* vector2)
{
    if (!vector2)
        return vec2(0.0f);

    return vec2(vector2[0], vector2[1]);
}

static vec2 convert_uv(float* vector2)
{
    if (!vector2)
        return vec2(0.0f);

    return vec2(vector2[0], vector2[1]);
}

static quat convert_quaternion(float* quaternion)
{
    if (!quaternion)
        return quat(1.0f, 0.0f, 0.0f, 0.0f);

    return quat(quaternion[3], quaternion[0], quaternion[1], quaternion[2]);
}