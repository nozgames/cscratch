//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <gltf.h>
#include <algorithm>
#include <filesystem>
#include <noz/asset.h>
#include <noz/noz.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace noz;

static void FlattenMesh(gltf_mesh_t* mesh)
{
    // Create a vector of triangle indices with their max z values
    struct TriangleInfo
    {
        float maxZ;
        uint16_t i0, i1, i2;
    };
    std::vector<TriangleInfo> triangles;

    // Process each triangle (3 consecutive indices)
    for (size_t i = 0; i < mesh->index_count; i += 3)
    {
        uint16_t idx0 = mesh->indices[i];
        uint16_t idx1 = mesh->indices[i + 1];
        uint16_t idx2 = mesh->indices[i + 2];

        // Find the maximum y value in this triangle
        float maxZ = std::max({
            -mesh->positions[idx0].y,
            -mesh->positions[idx1].y,
            -mesh->positions[idx2].y
        });

        triangles.push_back({maxZ, idx0, idx1, idx2});
    }

    // Sort triangles by max z value (back to front - highest z first)
    std::sort(triangles.begin(), triangles.end(),
        [](const TriangleInfo& a, const TriangleInfo& b)
        {
            return a.maxZ > b.maxZ;
        });

    // Rebuild the indices array with sorted triangles
    for (size_t t = 0; t < triangles.size(); t++)
    {
        const auto& tri = triangles[t];
        auto ii = t;
        
        mesh->indices[t * 3 + 0] = tri.i0;
        mesh->indices[t * 3 + 1] = tri.i1;
        mesh->indices[t * 3 + 2] = tri.i2;

        mesh->positions[tri.i0].y = ii * 0.001f;
        mesh->positions[tri.i1].y = ii * 0.001f;
        mesh->positions[tri.i2].y = ii * 0.001f;
    }
}

static void WriteMeshData(
    Stream* stream,
    const gltf_mesh_t* mesh,
    Props* meta)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_MESH;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);

    // Calculate bounds from positions
    bounds3 bounds = {};
    if (mesh->position_count > 0)
    {
        bounds.min = bounds.max = mesh->positions[0];
        for (size_t i = 1; i < mesh->position_count; i++)
        {
            const vec3& pos = mesh->positions[i];
            bounds.min.x = std::min(bounds.min.x, pos.x);
            bounds.min.y = std::min(bounds.min.y, pos.y);
            bounds.min.z = std::min(bounds.min.z, pos.z);
            bounds.max.x = std::max(bounds.max.x, pos.x);
            bounds.max.y = std::max(bounds.max.y, pos.y);
            bounds.max.z = std::max(bounds.max.z, pos.z);
        }
    }
    WriteBytes(stream, &bounds, sizeof(bounds3));
    
    // Write vertex data
    WriteU32(stream, static_cast<uint32_t>(mesh->position_count));
    for (size_t i = 0; i < mesh->position_count; ++i)
    {
        mesh_vertex vertex = {};
        vertex.position = mesh->positions[i];
        vertex.uv0 = vec2(0, 0);
        vertex.bone = 0;
        vertex.normal = vec3(0, 1, 0);
        
        if (mesh->normal_count == mesh->position_count && i < mesh->normal_count)
            vertex.normal = mesh->normals[i];
            
        if (mesh->uv_count == mesh->position_count && i < mesh->uv_count)
            vertex.uv0 = mesh->uvs[i];
            
        if (mesh->bone_index_count == mesh->position_count && i < mesh->bone_index_count)
            vertex.bone = static_cast<float>(mesh->bone_indices[i]);
            
        WriteBytes(stream, &vertex, sizeof(mesh_vertex));
    }
    
    // Write index data
    WriteU32(stream, static_cast<uint32_t>(mesh->index_count));
    WriteBytes(stream, mesh->indices, mesh->index_count * sizeof(uint16_t));
}

void ImportMesh(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    const fs::path& src_path = source_path;
    
    // Check if mesh import is enabled in meta file
    if (meta->GetBool("mesh", "skip_mesh", false))
        return;

    // Load GLTF/GLB file
    gltf_t* gltf = gltf_alloc(nullptr);
    if (!gltf_open(gltf, src_path))
    {
        gltf_free(gltf);
        throw std::runtime_error("Failed to open GLTF/GLB file");
    }
    
    // Create bone filter from meta file
    gltf_bone_filter_t* bone_filter = gltf_bone_filter_alloc(nullptr);
    fs::path meta_path = fs::path(src_path.string() + ".meta");
    gltf_bone_filter_from_meta_file(bone_filter, meta_path);
    
    // Read bones and mesh
    List* bones = gltf_read_bones(gltf, bone_filter, nullptr);
    gltf_mesh_t* mesh = gltf_read_mesh(gltf, bones, nullptr);
    
    if (!mesh || mesh->position_count == 0)
    {
        if (mesh) gltf_mesh_free(mesh);
        if (bones) Destroy(bones);
        gltf_bone_filter_free(bone_filter);
        gltf_close(gltf);
        gltf_free(gltf);
        throw std::runtime_error("No mesh data found");
    }
    
    // Apply flatten if requested
    if (meta->GetBool("mesh", "flatten", false))
        FlattenMesh(mesh);

    // Write mesh data to stream
    WriteMeshData(output_stream, mesh, meta);
    
    // Clean up
    gltf_mesh_free(mesh);
    if (bones) Destroy(bones);
    gltf_bone_filter_free(bone_filter);
    gltf_close(gltf);
    gltf_free(gltf);
}

bool DoesMeshDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // Check if dependency is the meta file for this mesh
    fs::path meta_path = fs::path(source_path.string() + ".meta");
    
    return meta_path == dependency_path;
}

static const char* g_mesh_extensions[] = {
    ".gltf",
    ".glb",
    nullptr
};

static AssetImporterTraits g_mesh_importer_traits = {
    .type_name = "Mesh",
    .type = TYPE_MESH,
    .signature = ASSET_SIGNATURE_MESH,
    .file_extensions = g_mesh_extensions,
    .import_func = ImportMesh,
    .does_depend_on = DoesMeshDependOn
};

AssetImporterTraits* GetMeshImporterTraits()
{
    return &g_mesh_importer_traits;
}