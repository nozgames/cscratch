//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/asset.h>
#include <noz/noz.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

static bool CompileAndWriteShader(
    const std::string& vertex_source,
    const std::string& fragment_source,
    Stream* output_stream,
    const fs::path& include_dir)
{
    // Make sure include directory is absolute
    fs::path absolute_include_dir = fs::absolute(include_dir);
    std::string include_dir_str = absolute_include_dir.string();
    
    // Setup HLSL info for vertex shader
    SDL_ShaderCross_HLSL_Info vertex_info = {};
    vertex_info.source = vertex_source.c_str();
    vertex_info.entrypoint = "vs";  // Vertex shader entry point
    vertex_info.include_dir = include_dir_str.c_str();
    vertex_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    vertex_info.enable_debug = false;
    
    // Compile vertex shader to SPIRV
    size_t vertex_spirv_size = 0;
    void* vertex_spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&vertex_info, &vertex_spirv_size);
    if (!vertex_spirv)
    {
        throw std::runtime_error(std::string("Failed to compile vertex shader: ") + SDL_GetError());
    }
    
    // Setup HLSL info for fragment shader
    SDL_ShaderCross_HLSL_Info fragment_info = {};
    fragment_info.source = fragment_source.c_str();
    fragment_info.entrypoint = "ps";  // Pixel/fragment shader entry point
    fragment_info.include_dir = include_dir_str.c_str();
    fragment_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    fragment_info.enable_debug = false;
    
    // Compile fragment shader to SPIRV
    size_t fragment_spirv_size = 0;
    void* fragment_spirv = SDL_ShaderCross_CompileSPIRVFromHLSL(&fragment_info, &fragment_spirv_size);
    if (!fragment_spirv)
    {
        SDL_free(vertex_spirv);
        throw std::runtime_error(std::string("Failed to compile fragment shader: ") + SDL_GetError());
    }
    
    // For now, we'll just use the SPIRV bytecode directly
    size_t vertex_size = vertex_spirv_size;
    size_t fragment_size = fragment_spirv_size;
    void* vertex_bytecode = vertex_spirv;
    void* fragment_bytecode = fragment_spirv;
    
    // Calculate runtime size
    uint32_t runtime_size = (uint32_t)(vertex_size + fragment_size + 64); // Add overhead
    
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_SHADER;
    header.runtime_size = runtime_size;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(output_stream, &header);
    
    // Write bytecode sizes and data
    WriteU32(output_stream, (uint32_t)vertex_size);
    WriteBytes(output_stream, vertex_bytecode, vertex_size);
    WriteU32(output_stream, (uint32_t)fragment_size);
    WriteBytes(output_stream, fragment_bytecode, fragment_size);
    
    // Write resource counts (simplified - would need parsing to get actual counts)
    WriteI32(output_stream, 1); // vertex_uniform_count
    WriteI32(output_stream, 1); // fragment_uniform_count
    WriteI32(output_stream, 0); // sampler_count
    
    // Write shader metadata
    WriteU8(output_stream, shader_flags_depth_test | shader_flags_depth_write);
    WriteU32(output_stream, SDL_GPU_BLENDFACTOR_SRC_ALPHA);
    WriteU32(output_stream, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    WriteU32(output_stream, SDL_GPU_CULLMODE_BACK);
    
    // Clean up
    SDL_free(vertex_spirv);
    SDL_free(fragment_spirv);
    
    return true;
}

void ImportShader(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta_props)
{
    fs::path src_path = source_path;
    
    // Read source file
    std::ifstream file(src_path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader source file");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    
    // Get the directory of the source file for includes
    fs::path include_dir = src_path.parent_path();
    
    // Compile and write shader to output stream
    if (!CompileAndWriteShader(source, source, output_stream, include_dir))
    {
        throw std::runtime_error("Failed to compile shader");
    }
}

bool DoesShaderDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // For now, shader files don't have dependencies
    // This could be extended to check for #include files
    return false;
}

static const char* g_shader_extensions[] = {
    ".hlsl",
    nullptr
};

static AssetImporterTraits g_shader_importer_traits = {
    .type_name = "Shader",
    .type = TYPE_SHADER,
    .signature = ASSET_SIGNATURE_SHADER,
    .file_extensions = g_shader_extensions,
    .import_func = ImportShader,
    .does_depend_on = DoesShaderDependOn
};

AssetImporterTraits* GetShaderImporterTraits()
{
    return &g_shader_importer_traits;
}