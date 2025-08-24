//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include "asset_manifest.h"
#include <noz/platform.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

struct AssetEntry
{
    std::string path;
    uint32_t signature;
    size_t runtime_size;
    size_t file_size;
    std::string var_name;
};

struct ManifestGenerator
{
    std::vector<AssetEntry> asset_entries;
    size_t total_memory;
    fs::path output_dir;
    Stream* manifest_stream;
};

static void ScanAssetFile(Path* file_path, file_stat_t* stat, void* user_data);
static bool ReadAssetHeader(const fs::path& file_path, uint32_t* signature, size_t* runtime_size);
static void GenerateManifestCode(ManifestGenerator* generator);
static std::string PathToVarName(const std::string& path);
static void WriteAssetTypeStruct(Stream* stream, const std::vector<AssetEntry>& assets, type_t type);
static void OrganizeAssetsByType(ManifestGenerator* generator);

bool GenerateAssetManifest(const fs::path& output_directory, const fs::path& manifest_output_path)
{
    if (output_directory.empty() || manifest_output_path.empty())
    {
        printf("ERROR: Invalid parameters for manifest generation\n");
        return false;
    }

    // Initialize manifest generator
    ManifestGenerator generator = {};
    generator.output_dir = output_directory;
    generator.asset_entries.reserve(64);

    generator.manifest_stream = CreateStream(nullptr, 1024);
    if (!generator.manifest_stream)
    {
        printf("ERROR: Failed to create manifest stream\n");
        return false;
    }

    generator.total_memory = 0;

    // Check if output directory exists
    if (!fs::exists(generator.output_dir))
    {
        
        // Generate an empty manifest
        GenerateManifestCode(&generator);
        
        // Save the manifest
        Path manifest_path;
        std::string path_str = manifest_output_path.string();
        strncpy(manifest_path.value, path_str.c_str(), sizeof(manifest_path.value) - 1);
        manifest_path.value[sizeof(manifest_path.value) - 1] = '\0';
        bool success = SaveStream(generator.manifest_stream, &manifest_path);
        
        // Clean up
        Destroy(generator.manifest_stream);
        
        return success;
    }

    if (!fs::is_directory(generator.output_dir))
    {
        printf("ERROR: '%s' is not a directory\n", generator.output_dir.string().c_str());
        Destroy(generator.manifest_stream);
        return false;
    }

    // Scan all files in the output directory recursively
    Path output_path;
    std::string output_str = generator.output_dir.string();
    strncpy(output_path.value, output_str.c_str(), sizeof(output_path.value) - 1);
    output_path.value[sizeof(output_path.value) - 1] = '\0';
    
    if (!directory_enum_files(&output_path, ScanAssetFile, &generator))
    {
        printf("ERROR: Failed to enumerate files in directory: %s\n", generator.output_dir.string().c_str());
        Destroy(generator.manifest_stream);
        return false;
    }


    // Generate the manifest C code
    GenerateManifestCode(&generator);

    // Save the manifest to file
    Path manifest_path;
    std::string manifest_str = manifest_output_path.string();
    strncpy(manifest_path.value, manifest_str.c_str(), sizeof(manifest_path.value) - 1);
    manifest_path.value[sizeof(manifest_path.value) - 1] = '\0';
    bool success = SaveStream(generator.manifest_stream, &manifest_path);
    if (!success)
    {
        printf("ERROR: Failed to save manifest to: %s\n", manifest_output_path.string().c_str());
    }

    // Clean up
    Destroy(generator.manifest_stream);

    return success;
}

static void ScanAssetFile(Path* file_path, file_stat_t* stat, void* user_data)
{
    auto* generator = (ManifestGenerator*)user_data;

    // Skip directories
    if (stat->is_directory)
        return;

    // Check for known asset extensions
    fs::path asset_path(file_path->value);
    std::string ext = asset_path.extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);
    
    bool is_asset = ext == ".nzt" ||   // NoZ Texture
                    ext == ".nzm" ||   // NoZ Mesh
                    ext == ".nzs" ||   // NoZ Sound
                    ext == ".nzsh" ||  // NoZ Shader
                    ext == ".nzmt" ||  // NoZ Material
                    ext == ".nzf";     // NoZ Font

    if (!is_asset)
        return;

    // Make path relative to output_dir first
    fs::path relative_path = fs::relative(asset_path, generator->output_dir);
    
    // Remove extension for comparison and storage
    relative_path.replace_extension("");
    std::string relative_str = relative_path.string();
    
    // Check if this asset is already in the list (compare without extensions)
    for (const auto& existing : generator->asset_entries)
    {
        if (existing.path == relative_str)
        {
            // Asset already in list, skip it
            return;
        }
    }

    AssetEntry entry = {};
    
    // Copy the relative path we already computed (extension already removed)
    entry.path = relative_str;
    
    entry.file_size = stat->size;

    // Generate variable name from path (without extension)
    entry.var_name = PathToVarName(entry.path);

    // Read asset header to get signature and runtime size
    if (!ReadAssetHeader(asset_path, &entry.signature, &entry.runtime_size))
    {
        printf("WARNING: Failed to read asset header for: %s\n", asset_path.string().c_str());
        entry.signature = 0;
        entry.runtime_size = stat->size; // Fallback to file size
    }

    // Add to total memory requirement
    generator->total_memory += entry.runtime_size;
    
    // Add entry to the list
    generator->asset_entries.push_back(entry);
}

static bool ReadAssetHeader(const fs::path& file_path, uint32_t* signature, size_t* runtime_size)
{
    Path asset_path;
    std::string path_str = file_path.string();
    strncpy(asset_path.value, path_str.c_str(), sizeof(asset_path.value) - 1);
    asset_path.value[sizeof(asset_path.value) - 1] = '\0';
    Stream* stream = LoadStream(nullptr, &asset_path);
    if (!stream)
        return false;

    // Read asset header (16 bytes)
    if (GetSize(stream) < 16)
    {
        Destroy(stream);
        return false;
    }

    // Read header fields
    *signature = ReadU32(stream);
    *runtime_size = ReadU32(stream);
    // Skip version and flags for manifest generation
    
    Destroy(stream);
    return true;
}

static void GenerateManifestCode(ManifestGenerator* generator)
{
    Stream* stream = generator->manifest_stream;

    // Write header
    WriteCSTR(stream,
        "//\n"
        "// Auto-generated asset manifest - DO NOT EDIT MANUALLY\n"
        "// Generated by NoZ Game Engine Asset Importer\n"
        "//\n\n"
        "// @includes\n"
        "#include <noz/noz.h>\n\n");

    // Write memory requirement constant and allocator pointer
    WriteCSTR(stream, "// @constants\n");
    WriteCSTR(stream, "#define ASSET_TOTAL_MEMORY %zu\n\n", generator->total_memory);
    
    WriteCSTR(stream, "// @globals\n");
    WriteCSTR(stream, "static arena_allocator_t* g_asset_allocator = nullptr;\n\n");

    // Generate the g_assets structure
    OrganizeAssetsByType(generator);

    // Write load all function
    WriteCSTR(stream,
        "// @init\n"
        "bool assets_init(void)\n"
        "{\n"
        "    if (g_asset_allocator != nullptr)\n"
        "        return false; // Already initialized\n\n"
        "    g_asset_allocator = arena_allocator_create(ASSET_TOTAL_MEMORY);\n"
        "    if (!g_asset_allocator)\n"
        "        return false;\n\n");
    
    // Generate load calls for each asset
    for (const auto& entry : generator->asset_entries)
    {
        type_t asset_type = ToType(entry.signature);
        if (asset_type == type_invalid)
			continue;

        const char* type_name = ToString(asset_type);
        assert(type_name);
        WriteCSTR(
            stream,
            "    NOZ_ASSET_LOAD(%s, \"%s\", g_assets.%ss.%s);\n",
            type_name,
            entry.path.c_str(),
            type_name,
            entry.var_name.c_str());
    }
    
    WriteCSTR(stream, "\n    return true;\n}\n\n");
    
    // Write assets_uninit function
    WriteCSTR(stream,
        "// @uninit\n"
        "void assets_uninit(void)\n"
        "{\n"
        "    if (g_asset_allocator != nullptr)\n"
        "    {\n"
        "        arena_allocator_destroy(g_asset_allocator);\n"
        "        g_asset_allocator = nullptr;\n"
        "        \n"
        "        // Clear all asset pointers\n"
        "        memset(&g_assets, 0, sizeof(g_assets));\n"
        "    }\n"
        "}\n");
}

static void write_asset_structure(ManifestGenerator* generator)
{
    Stream* stream = generator->manifest_stream;

    // Write asset info structure
    WriteCSTR(stream,
        "typedef struct asset_info\n"
        "{\n"
        "    char* path;\n"
        "    uint32_t signature;\n"
        "    size_t runtime_size;\n"
        "    size_t file_size;\n"
        "} asset_info_t;\n\n");

    if (generator->asset_entries.empty())
    {
        WriteCSTR(stream,
            "// No assets found\n"
            "static asset_info_t g_assets[] = { {0} };\n\n");
        return;
    }

    // Write assets array
    WriteCSTR(stream, "static asset_info_t g_assets[] = {\n");

    for (const auto& entry : generator->asset_entries)
    {
        char buffer[512];

        snprintf(buffer, sizeof(buffer), 
            "    { \"%s\", 0x%08X, %zu, %zu },\n",
            entry.path.c_str(),
            entry.signature,
            entry.runtime_size,
            entry.file_size);

        WriteCSTR(stream, "%s", buffer);
    }

    WriteCSTR(stream, "};\n\n");
}

static std::string PathToVarName(const std::string& path_str)
{
    if (path_str.empty())
    {
        return "unknown";
    }
    
    // Convert to filesystem path and get filename without extension
    fs::path path(path_str);
    std::string filename = path.filename().replace_extension("").string();
    
    // If filename is empty, use "unknown"
    if (filename.empty())
    {
        return "unknown";
    }
    
    // Convert filename to valid C variable name
    std::string result;
    
    for (char c : filename)
    {
        if (std::isalnum(c))
            result += std::tolower(c);
        else
            result += '_';
    }
    
    // Check for C keywords and add underscore prefix if needed
    static std::vector<std::string> c_keywords = {
        "default", "switch", "case", "break", "continue", "return",
        "if", "else", "for", "while", "do", "goto", "void",
        "int", "float", "double", "char", "const", "static",
        "struct", "union", "enum", "typedef"
    };
    
    if (std::find(c_keywords.begin(), c_keywords.end(), result) != c_keywords.end())
    {
        result = "_" + result;
    }
    
    return result;
}

static void WriteAssetTypeStruct(Stream* stream, const std::vector<AssetEntry>& assets, type_t type)
{
    if (assets.empty())
        return;
    
    const char* type_name = ToString(type);
    if (type_name == nullptr)
        return;
   
    // Write struct opening
    WriteCSTR(stream, "    struct\n    {\n");
    
    // Write each asset field
    for (const auto& entry : assets)
        WriteCSTR(stream, "        %s_t* %s;\n", type_name, entry.var_name.c_str());

    // Write struct closing with collection name (plural)
    WriteCSTR(stream, "    } %ss;\n", type_name);
}

static void OrganizeAssetsByType(ManifestGenerator* generator)
{
    auto* stream = generator->manifest_stream;
    
    std::vector<AssetEntry> textures;
    std::vector<AssetEntry> meshes;
    std::vector<AssetEntry> sounds;
    std::vector<AssetEntry> shaders;
    std::vector<AssetEntry> materials;
    std::vector<AssetEntry> fonts;
    
    for (const auto& entry : generator->asset_entries)
    {
        switch (ToType(entry.signature))
        {
            case type_texture:
                textures.push_back(entry);
                break;
            case type_mesh:
                meshes.push_back(entry);
                break;
            case type_sound:
                sounds.push_back(entry);
                break;
            case type_shader:
                shaders.push_back(entry);
                break;
            case type_material:
                materials.push_back(entry);
                break;
            case type_font:
                fonts.push_back(entry);
                break;
            default:
                // Unknown type, skip
                break;
        }
    }
    
    WriteCSTR(stream, "// @assets\n");
    WriteCSTR(stream, "struct\n{\n");
    
    bool has_any_assets =
        !textures.empty() ||
        !meshes.empty() ||
        !sounds.empty() ||
        !shaders.empty() ||
        !materials.empty() ||
        !fonts.empty();
    
    if (has_any_assets)
    {
        // Generate structures for each asset type
        WriteAssetTypeStruct(stream, textures, type_texture);
        WriteAssetTypeStruct(stream, meshes, type_mesh);
        WriteAssetTypeStruct(stream, sounds, type_sound);
        WriteAssetTypeStruct(stream, shaders, type_shader);
        WriteAssetTypeStruct(stream, materials, type_material);
        WriteAssetTypeStruct(stream, fonts, type_font);
    }
    else
    {
        // Add dummy member to avoid empty struct error
        WriteCSTR(stream, "    void* _dummy; // Placeholder for empty asset structure\n");
    }
    
    WriteCSTR(stream, "} g_assets;\n\n");
}