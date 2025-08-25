//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include "asset_manifest.h"
#include <noz/asset.h>
#include <noz/platform.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <map>

namespace fs = std::filesystem;

struct AssetEntry
{
    std::string path;
    uint32_t signature;
    size_t file_size;
    std::string var_name;
};

struct PathNode 
{
    std::map<std::string, std::unique_ptr<PathNode>> children;
    std::vector<AssetEntry> assets;
};

struct ManifestGenerator
{
    std::vector<AssetEntry> asset_entries;
    fs::path output_dir;
    Stream* manifest_stream;
    const std::vector<AssetImporterTraits*>* importers;
    Props* config;
};

static void GenerateManifestCode(ManifestGenerator* generator);
static void GenerateAssetsHeader(ManifestGenerator* generator, const fs::path& header_path);
static std::string PathToVarName(const std::string& path);
static void WriteAssetTypeStruct(Stream* stream, const std::vector<AssetEntry>& assets, const char* type_name);
static void OrganizeAssetsByType(ManifestGenerator* generator);
static void ScanAssetFile(const fs::path& file_path, ManifestGenerator* generator);
static type_t ToTypeFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers);
static const char* ToStringFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers);
static const char* ToMacroFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers);
static void GenerateRendererSetupCalls(ManifestGenerator* generator, Stream* stream);

bool GenerateAssetManifest(const fs::path& output_directory, const fs::path& manifest_output_path, const std::vector<AssetImporterTraits*>& importers, Props* config)
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
    generator.importers = &importers;
    generator.config = config;

    generator.manifest_stream = CreateStream(nullptr, 1024);
    if (!generator.manifest_stream)
    {
        printf("ERROR: Failed to create manifest stream\n");
        return false;
    }


    // Check if output directory exists
    if (!fs::exists(generator.output_dir))
    {
        
        // Generate an empty manifest
        GenerateManifestCode(&generator);
        
        // Save the manifest
        bool success = SaveStream(generator.manifest_stream, manifest_output_path);
        
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

    // Scan all files in the output directory recursively using std::filesystem
    try
    {
        for (const auto& entry : fs::recursive_directory_iterator(generator.output_dir))
        {
            if (entry.is_regular_file())
            {
                ScanAssetFile(entry.path(), &generator);
            }
        }
    }
    catch (const std::exception& e)
    {
        printf("ERROR: Failed to enumerate files in directory: %s - %s\n", 
               generator.output_dir.string().c_str(), e.what());
        Destroy(generator.manifest_stream);
        return false;
    }


    // Generate the manifest C code
    GenerateManifestCode(&generator);

    // Generate header file (change .cpp to .h)
    fs::path header_path = manifest_output_path;
    header_path.replace_extension(".h");
    GenerateAssetsHeader(&generator, header_path);

    // Save the manifest to file
    bool success = SaveStream(generator.manifest_stream, manifest_output_path);
    if (!success)
    {
        printf("ERROR: Failed to save manifest to: %s\n", manifest_output_path.string().c_str());
    }

    // Clean up
    Destroy(generator.manifest_stream);

    return success;
}

static void WriteHeaderNestedStructs(Stream* stream, const PathNode& node, const std::vector<AssetImporterTraits*>& importers, int indent_level = 1)
{
    std::string indent(indent_level * 4, ' ');
    
    // Write child directories as nested structs
    for (const auto& [name, child] : node.children)
    {
        WriteCSTR(stream, "%sstruct\n%s{\n", indent.c_str(), indent.c_str());
        WriteHeaderNestedStructs(stream, *child, importers, indent_level + 1);
        WriteCSTR(stream, "%s} %s;\n", indent.c_str(), name.c_str());
    }
    
    // Write assets as forward declarations
    for (const auto& entry : node.assets)
    {
        const char* type_name = ToStringFromSignature(entry.signature, importers);
        if (type_name)
        {
            std::string var_name = PathToVarName(fs::path(entry.path).filename().replace_extension("").string());
            WriteCSTR(stream, "%s%s* %s;\n", indent.c_str(), type_name, var_name.c_str());
        }
    }
}

static void GenerateAssetsHeader(ManifestGenerator* generator, const fs::path& header_path)
{
    Stream* header_stream = CreateStream(nullptr, 1024);
    if (!header_stream)
        return;
        
    WriteCSTR(header_stream,
        "//\n"
        "// Auto-generated asset header - DO NOT EDIT MANUALLY\n"
        "// Generated by NoZ Game Engine Asset Importer\n"
        "//\n\n");
    
    // Generate asset listing comment block
    if (!generator->asset_entries.empty())
    {
        // Group assets by type
        std::map<std::string, std::vector<std::string>> assets_by_type;
        
        for (const auto& entry : generator->asset_entries)
        {
            const char* type_name = ToStringFromSignature(entry.signature, *generator->importers);
            if (type_name)
            {
                // Convert asset path to access path (e.g., "textures/icons/button" -> "LoadedAssets.textures.icons.button")
                fs::path asset_path(entry.path);
                std::string access_path = "LoadedAssets";
                
                auto parent_path = asset_path.parent_path();
                for (const auto& part : parent_path)
                {
                    access_path += "." + part.string();
                }
                
                std::string var_name = PathToVarName(asset_path.filename().string());
                access_path += "." + var_name;
                
                // Add to the appropriate type group
                std::string type_key = std::string(type_name) + "s";
                std::transform(type_key.begin(), type_key.end(), type_key.begin(), ::tolower);
                assets_by_type[type_key].push_back(access_path);
            }
        }
        
        // Write the comment block
        for (const auto& [type_name, asset_list] : assets_by_type)
        {
            WriteCSTR(header_stream, "// @%s\n", type_name.c_str());
            for (const auto& asset_path : asset_list)
            {
                WriteCSTR(header_stream, "// %s\n", asset_path.c_str());
            }
            WriteCSTR(header_stream, "//\n");
        }
        WriteCSTR(header_stream, "\n");
    }
    
    WriteCSTR(header_stream,
        "#pragma once\n\n"
        "// Forward declarations\n"
        "struct Shader;\n"
        "struct Texture;\n"
        "struct Mesh;\n"
        "struct Font;\n"
        "struct Material;\n"
        "struct Sound;\n\n");
    
    // Build directory tree (same as in OrganizeAssetsByType)
    PathNode root;
    for (const auto& entry : generator->asset_entries)
    {
        fs::path asset_path(entry.path);
        PathNode* current = &root;
        
        auto parent_path = asset_path.parent_path();
        for (const auto& part : parent_path)
        {
            std::string part_str = part.string();
            if (current->children.find(part_str) == current->children.end())
            {
                current->children[part_str] = std::make_unique<PathNode>();
            }
            current = current->children[part_str].get();
        }
        
        AssetEntry modified_entry = entry;
        current->assets.push_back(modified_entry);
    }
    
    // Write LoadedAssets struct
    WriteCSTR(header_stream, "struct LoadedAssets\n{\n");
    if (generator->asset_entries.empty())
    {
        WriteCSTR(header_stream, "    void* _dummy;\n");
    }
    else
    {
        WriteHeaderNestedStructs(header_stream, root, *generator->importers);
    }
    WriteCSTR(header_stream, "};\n\n");
    
    WriteCSTR(header_stream, "extern LoadedAssets Assets;\n\n");
    WriteCSTR(header_stream, "bool LoadAssets(size_t arena_size = 0);\n");
    WriteCSTR(header_stream, "void UnloadAssets();\n");
    
    // Save header file
    SaveStream(header_stream, header_path);
    Destroy(header_stream);
}

static bool ReadAssetHeader(const fs::path& file_path, uint32_t* signature)
{
    Stream* stream = LoadStream(nullptr, file_path);
    if (!stream)
        return false;

    // Read asset header
    AssetHeader header;
    if (!ReadAssetHeader(stream, &header))
    {
        Destroy(stream);
        return false;
    }

    *signature = header.signature;

    Destroy(stream);
    return true;
}

static void ScanAssetFile(const fs::path& file_path, ManifestGenerator* generator)
{
    // Check for known asset extensions
    std::string ext = file_path.extension().string();
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
    fs::path relative_path = fs::relative(file_path, generator->output_dir);
    
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
    
    // Get file size using std::filesystem
    entry.file_size = fs::file_size(file_path);

    // Generate variable name from path (without extension)
    entry.var_name = PathToVarName(entry.path);

    // Read asset header to get signature
    if (!ReadAssetHeader(file_path, &entry.signature))
    {
        printf("WARNING: Failed to read asset header for: %s\n", file_path.string().c_str());
        entry.signature = 0;
    }
    
    // Add entry to the list
    generator->asset_entries.push_back(entry);
}

static void GenerateManifestCode(ManifestGenerator* generator)
{
    auto stream = generator->manifest_stream;

    WriteCSTR(stream,
        "//\n"
        "// Auto-generated asset manifest - DO NOT EDIT MANUALLY\n"
        "// Generated by NoZ Game Engine Asset Importer\n"
        "//\n\n"
        "// @includes\n"
        "#include <noz/noz.h>\n"
        "#include \"assets.h\"\n\n");

    WriteCSTR(stream, "// @globals\n");
    WriteCSTR(stream, "static Allocator* g_asset_allocator = nullptr;\n\n");
    WriteCSTR(stream, "// @assets\n");
    WriteCSTR(stream, "LoadedAssets Assets = {};\n\n");

    OrganizeAssetsByType(generator);

    WriteCSTR(stream,
        "// @init\n"
        "bool LoadAssets(size_t arena_size)\n"
        "{\n"
        "    if (g_asset_allocator != nullptr)\n"
        "        return false; // Already initialized\n\n"
        "    if (arena_size > 0)\n"
        "    {\n"
        "        g_asset_allocator = CreateArenaAllocator(arena_size);\n"
        "        if (!g_asset_allocator)\n"
        "            return false;\n"
        "    }\n\n");
    
    for (const auto& entry : generator->asset_entries)
    {
        const char* macro_name = ToMacroFromSignature(entry.signature, *generator->importers);
        if (!macro_name)
            continue;

        // Convert backslashes to forward slashes for the asset path
        std::string normalized_path = entry.path;
        std::replace(normalized_path.begin(), normalized_path.end(), '\\', '/');

        // Build nested access path (e.g., Assets.textures.icons.myicon)
        fs::path asset_path(entry.path);
        std::string access_path = "Assets";
        
        auto parent_path = asset_path.parent_path();
        for (const auto& part : parent_path)
        {
            access_path += "." + part.string();
        }
        
        std::string var_name = PathToVarName(asset_path.filename().replace_extension("").string());
        access_path += "." + var_name;

        WriteCSTR(
            stream,
            "    %s(\"%s\", %s);\n",
            macro_name,
            normalized_path.c_str(),
            access_path.c_str());
    }
    
    // Generate renderer setup calls if config is provided
    GenerateRendererSetupCalls(generator, stream);
    
    WriteCSTR(stream, "\n    return true;\n}\n\n");
    
    // Write UnloadAssets function
    WriteCSTR(stream,
        "// @uninit\n"
        "void UnloadAssets()\n"
        "{\n"
        "    if (g_asset_allocator != nullptr)\n"
        "    {\n"
        "        Destroy(g_asset_allocator);\n"
        "        g_asset_allocator = nullptr;\n"
        "        \n"
        "        // Clear all asset pointers\n"
        "        memset(&Assets, 0, sizeof(Assets));\n"
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

        // Convert backslashes to forward slashes for the asset path
        std::string normalized_path = entry.path;
        std::replace(normalized_path.begin(), normalized_path.end(), '\\', '/');

        snprintf(buffer, sizeof(buffer), 
            "    { \"%s\", 0x%08X, %zu },\n",
            normalized_path.c_str(),
            entry.signature,
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
    
    // Convert full path to filesystem path and remove extension
    fs::path path(path_str);
    path.replace_extension("");
    std::string full_path = path.string();
    
    // If path is empty, use "unknown"
    if (full_path.empty())
    {
        return "unknown";
    }
    
    // Convert full path to valid C variable name
    std::string result;
    
    for (char c : full_path)
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

static void WriteAssetTypeStruct(Stream* stream, const std::vector<AssetEntry>& assets, const char* type_name)
{
    if (assets.empty() || !type_name)
        return;
   
    // Write struct opening
    WriteCSTR(stream, "    struct\n    {\n");
    
    // Write each asset field
    for (const auto& entry : assets)
        WriteCSTR(stream, "        %s* %s;\n", type_name, entry.var_name.c_str());

    // Write struct closing with collection name (plural, lowercase)
    std::string collection_name = std::string(type_name) + "s";
    std::transform(collection_name.begin(), collection_name.end(), collection_name.begin(), ::tolower);
    WriteCSTR(stream, "    } %s;\n", collection_name.c_str());
}


static void WriteNestedStructs(Stream* stream, const PathNode& node, const std::vector<AssetImporterTraits*>& importers, int indent_level = 1)
{
    std::string indent(indent_level * 4, ' ');
    
    // Write child directories as nested structs
    for (const auto& [name, child] : node.children)
    {
        WriteCSTR(stream, "%sstruct\n%s{\n", indent.c_str(), indent.c_str());
        WriteNestedStructs(stream, *child, importers, indent_level + 1);
        WriteCSTR(stream, "%s} %s;\n", indent.c_str(), name.c_str());
    }
    
    // Write assets as pointers
    for (const auto& entry : node.assets)
    {
        const char* type_name = ToStringFromSignature(entry.signature, importers);
        if (type_name)
        {
            WriteCSTR(stream, "%s%s* %s;\n", indent.c_str(), type_name, entry.var_name.c_str());
        }
    }
}

static void OrganizeAssetsByType(ManifestGenerator* generator)
{
    auto* stream = generator->manifest_stream;
    
    // Build directory tree
    PathNode root;
    
    for (const auto& entry : generator->asset_entries)
    {
        fs::path asset_path(entry.path);
        PathNode* current = &root;
        
        // Navigate/create directory structure
        auto parent_path = asset_path.parent_path();
        for (const auto& part : parent_path)
        {
            std::string part_str = part.string();
            if (current->children.find(part_str) == current->children.end())
            {
                current->children[part_str] = std::make_unique<PathNode>();
            }
            current = current->children[part_str].get();
        }
        
        // Update var_name to just be the filename
        AssetEntry modified_entry = entry;
        modified_entry.var_name = PathToVarName(asset_path.filename().replace_extension("").string());
        
        // Add asset to the final directory
        current->assets.push_back(modified_entry);
    }
    
    // Struct definition is now in the header file
}

static type_t ToTypeFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers)
{
    for (const auto* importer : importers)
    {
        if (importer && importer->signature == signature)
        {
            return importer->type;
        }
    }
    return TYPE_UNKNOWN;
}

static const char* ToMacroFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers)
{
    static std::map<asset_signature_t, std::string> macro_cache;
    
    for (const auto* importer : importers)
    {
        if (importer && importer->signature == signature)
        {
            // Check cache first
            if (macro_cache.find(signature) != macro_cache.end())
            {
                return macro_cache[signature].c_str();
            }
            
            // Build macro name: NOZ_LOAD_ + uppercase type name
            std::string type_name = importer->type_name;
            std::string macro_name = "NOZ_LOAD_";
            
            // Convert type name to uppercase and handle special cases
            for (char c : type_name)
            {
                if (std::islower(c))
                    macro_name += std::toupper(c);
                else if (std::isupper(c))
                {
                    // Insert underscore before uppercase letters (except first)
                    if (macro_name.size() > 9) // 9 = length of "NOZ_LOAD_"
                        macro_name += '_';
                    macro_name += c;
                }
                else
                    macro_name += c;
            }
            
            // Cache and return
            macro_cache[signature] = macro_name;
            return macro_cache[signature].c_str();
        }
    }
    return nullptr;
}

static const char* ToStringFromSignature(asset_signature_t signature, const std::vector<AssetImporterTraits*>& importers)
{
    for (const auto* importer : importers)
    {
        if (importer && importer->signature == signature)
        {
            return importer->type_name;
        }
    }
    return nullptr;
}

static void GenerateRendererSetupCalls(ManifestGenerator* generator, Stream* stream)
{
    if (!generator->config)
        return;
    
    // Check if [noz] section exists
    if (!generator->config->HasGroup("noz"))
        return;
        
    WriteCSTR(stream, "\n    // Setup renderer globals from config\n");
    
    static std::vector<std::pair<std::string, std::string>> globals = {
        { "shadow_shader", "SetShadowPassShader" },
        { "gamma_shader", "SetGammaPassShader" }
    };

    for (auto& global : globals)
    {
        if (!generator->config->HasKey("noz", global.first.c_str()))
            continue;

        auto asset_path = generator->config->GetString("noz", global.first.c_str(), "");
        if (asset_path.empty())
            continue;

        // Convert asset path to access path (e.g., "shaders/shadow" -> "Assets.shaders.shadow")
        fs::path path(asset_path);
        std::string access_path = "Assets";

        auto parent_path = path.parent_path();
        for (const auto& part : parent_path)
            access_path += "." + part.string();

        std::string var_name = PathToVarName(path.filename().string());
        access_path += "." + var_name;
                
        WriteCSTR(stream, "    %s(%s);\n", global.second.c_str(), access_path.c_str());
    }
}