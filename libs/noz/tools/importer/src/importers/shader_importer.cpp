//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

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
        std::cerr << "Failed to compile vertex shader: " << SDL_GetError() << std::endl;
        return false;
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
        std::cerr << "Failed to compile fragment shader: " << SDL_GetError() << std::endl;
        SDL_free(vertex_spirv);
        return false;
    }
    
    // For now, we'll just use the SPIRV bytecode directly
    size_t vertex_size = vertex_spirv_size;
    size_t fragment_size = fragment_spirv_size;
    void* vertex_bytecode = vertex_spirv;
    void* fragment_bytecode = fragment_spirv;
    
    // Calculate runtime size
    uint32_t runtime_size = (uint32_t)(vertex_size + fragment_size + 64); // Add overhead
    
    // Write asset header
    asset_header_t header = {};
    header.signature = NOZ_SHADER_SIG;
    header.runtime_size = runtime_size;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(output_stream, &header);
    
    // Write SHDR signature for shader-specific data
    WriteFileSignature(output_stream, "SHDR", 4);
    WriteU32(output_stream, 1); // version
    
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

void ImportShader(const fs::path& source_path, const fs::path& output_path, Props* config)
{
    fs::path src_path = source_path;
    fs::path out_path = output_path;
    
    // Get source directories from config
    size_t source_count = GetListCount(config, "source");
    std::vector<fs::path> source_dirs;
    
    for (size_t i = 0; i < source_count; i++)
    {
        const char* dir = GetListElement(config, "source", i, "");
        if (dir && *dir)
        {
            source_dirs.push_back(fs::path(dir));
        }
    }
    
    // Find relative path from source directories
    fs::path relative_path;
    bool found_relative = false;
    
    for (const auto& base_dir : source_dirs)
    {
        try
        {
            if (src_path.string().find(base_dir.string()) == 0)
            {
                relative_path = fs::relative(src_path, base_dir);
                found_relative = true;
                break;
            }
        }
        catch (...)
        {
            // Continue if relative path can't be computed
        }
    }
    
    if (!found_relative)
    {
        // If we couldn't find relative path, just use the filename
        relative_path = src_path.filename();
    }
    
    // Read source file
    std::ifstream file(src_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader source: " << src_path << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Create output stream
    Stream* output_stream = CreateStream(nullptr, 4096);
    if (!output_stream)
    {
        return;
    }
    
    // Get the directory of the source file for includes
    fs::path include_dir = src_path.parent_path();
    
    // Compile and write shader
    if (CompileAndWriteShader(source, source, output_stream, include_dir))
    {
        // Build output file path preserving directory structure
        fs::path final_path = out_path / relative_path;
        
        // Replace extension with .nzsh
        final_path.replace_extension(".nzsh");
        
        // Ensure the output directory exists
        fs::create_directories(final_path.parent_path());
        
        // Save the stream to file
        // We need to convert to Path for SaveStream API
        Path save_path;
        std::string path_str = final_path.string();
        strncpy(save_path.value, path_str.c_str(), sizeof(save_path.value) - 1);
        save_path.value[sizeof(save_path.value) - 1] = '\0';
        
        if (!SaveStream(output_stream, &save_path))
        {
            std::cerr << "Failed to save shader: " << final_path << std::endl;
        }
        else
        {
            // Remove extension for clean output
            fs::path clean_path = relative_path;
            clean_path.replace_extension("");
            std::cout << "Imported 'shaders/" << clean_path.filename().string() << "'" << std::endl;
        }
    }
    
    Destroy(output_stream);
}

bool CanImportAsShader(const fs::path& path)
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".hlsl";
}

bool DoesShaderDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // For now, shader files don't have dependencies
    // This could be extended to check for #include files
    return false;
}

static AssetImporterTraits g_shader_importer_traits = {
    .can_import = CanImportAsShader,
    .import_func = ImportShader,
    .does_depend_on = DoesShaderDependOn
};

AssetImporterTraits* GetShaderImporterTraits()
{
    return &g_shader_importer_traits;
}