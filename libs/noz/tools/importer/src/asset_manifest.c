//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "asset_manifest.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct asset_entry
{
    path_t path;             // Relative path from assets directory
    uint32_t signature;      // 4-byte signature
    size_t runtime_size;     // Size needed at runtime
    size_t file_size;        // Size of the file on disk
    char* var_name;          // Variable name for C struct
    char* folder_path;       // Folder hierarchy (e.g., "textures/terrain")
} asset_entry_t;

typedef struct manifest_generator
{
    array_t* asset_entries;  // Array of asset_entry_t
    size_t total_memory;     // Total memory requirement
    path_t output_dir;       // Output directory path
    stream_t* manifest_stream; // C code output stream
} manifest_generator_t;

// Forward declarations
static void scan_asset_file(const char* file_path, const file_stat_t* stat, void* user_data);
static bool read_asset_header(const char* file_path, uint32_t* signature, size_t* runtime_size);
static void generate_manifest_code(manifest_generator_t* generator);
static void write_asset_structure(manifest_generator_t* generator);
static const char* get_relative_path(const char* full_path, const char* base_path);
static void generate_folder_structure(manifest_generator_t* generator);
static char* path_to_var_name(const char* path);
static const char* get_asset_type_name(uint32_t signature);
static void organize_assets_by_type(manifest_generator_t* generator);

bool asset_manifest_generate(const char* output_directory, const char* manifest_output_path)
{
    if (!output_directory || !manifest_output_path)
    {
        printf("ERROR: Invalid parameters for manifest generation\n");
        return false;
    }

    printf("Generating asset manifest from: %s\n", output_directory);
    printf("Output manifest to: %s\n", manifest_output_path);

    // Initialize manifest generator
    manifest_generator_t generator = {0};
    path_set(&generator.output_dir, output_directory);
    
    generator.asset_entries = array_create(sizeof(asset_entry_t), 64);
    if (!generator.asset_entries)
    {
        printf("ERROR: Failed to create asset entries array\n");
        return false;
    }

    generator.manifest_stream = stream_alloc(NULL, 1024);
    if (!generator.manifest_stream)
    {
        printf("ERROR: Failed to create manifest stream\n");
        array_free(generator.asset_entries);
        return false;
    }

    generator.total_memory = 0;

    // Check if output directory exists
    file_stat_t dir_stat;
    if (!platform_get_file_stat(output_directory, &dir_stat))
    {
        printf("WARNING: Output directory '%s' does not exist or is not accessible\n", output_directory);
        printf("This may be expected if no assets have been imported yet.\n");
        
        // Generate an empty manifest
        generate_manifest_code(&generator);
        
        // Save the manifest
        path_t manifest_path;
        path_set(&manifest_path, manifest_output_path);
        bool success = stream_save(generator.manifest_stream, &manifest_path);
        
        // Clean up
        object_free(generator.manifest_stream);
        array_free(generator.asset_entries);
        
        return success;
    }

    if (!dir_stat.is_directory)
    {
        printf("ERROR: '%s' is not a directory\n", output_directory);
        object_free(generator.manifest_stream);
        array_free(generator.asset_entries);
        return false;
    }

    // Scan all files in the output directory recursively
    if (!platform_enum_files(output_directory, scan_asset_file, &generator))
    {
        printf("ERROR: Failed to enumerate files in directory: %s\n", output_directory);
        object_free(generator.manifest_stream);
        array_free(generator.asset_entries);
        return false;
    }

    printf("Found %zu asset files\n", array_length(generator.asset_entries));
    printf("Total runtime memory required: %zu bytes\n", generator.total_memory);

    // Generate the manifest C code
    generate_manifest_code(&generator);

    // Save the manifest to file
    path_t manifest_path;
    path_set(&manifest_path, manifest_output_path);
    bool success = stream_save(generator.manifest_stream, &manifest_path);
    if (success)
    {
        printf("Asset manifest generated successfully: %s\n", manifest_output_path);
    }
    else
    {
        printf("ERROR: Failed to save manifest to: %s\n", manifest_output_path);
    }

    // Clean up
    object_free(generator.manifest_stream);
    array_free(generator.asset_entries);

    return success;
}

static void scan_asset_file(const char* file_path, const file_stat_t* stat, void* user_data)
{
    manifest_generator_t* generator = (manifest_generator_t*)user_data;

    // Skip directories
    if (stat->is_directory)
        return;

    // Skip non-asset files (you might want to add more extensions here)
    const char* ext = strrchr(file_path, '.');
    if (!ext)
        return;

    // Check for known asset extensions
    bool is_asset = false;
    if (strcmp(ext, ".nzt") == 0 ||   // NoZ Texture
        strcmp(ext, ".nzm") == 0 ||   // NoZ Mesh
        strcmp(ext, ".nzs") == 0 ||   // NoZ Sound
        strcmp(ext, ".nzsh") == 0 ||  // NoZ Shader
        strcmp(ext, ".nzmt") == 0 ||  // NoZ Material
        strcmp(ext, ".nzf") == 0)     // NoZ Font
    {
        is_asset = true;
    }

    if (!is_asset)
        return;

    printf("Processing asset: %s\n", file_path);

    // Create asset entry
    asset_entry_t* entry = (asset_entry_t*)array_push(generator->asset_entries);
    if (!entry)
    {
        printf("WARNING: Failed to add asset entry for: %s\n", file_path);
        return;
    }

    // Store relative path
    const char* rel_path = get_relative_path(file_path, generator->output_dir.value);
    path_set(&entry->path, rel_path ? rel_path : file_path);
    entry->file_size = stat->size;

    // Generate variable name from path
    entry->var_name = path_to_var_name(rel_path ? rel_path : file_path);
    
    // Extract folder path
    char folder[256] = {0};
    const char* last_slash = strrchr(entry->path.value, '/');
    if (!last_slash) last_slash = strrchr(entry->path.value, '\\');
    if (last_slash) {
        size_t folder_len = last_slash - entry->path.value;
        if (folder_len < sizeof(folder)) {
            strncpy(folder, entry->path.value, folder_len);
            folder[folder_len] = '\0';
        }
    }
    entry->folder_path = strdup(folder);

    // Read asset header to get signature and runtime size
    if (!read_asset_header(file_path, &entry->signature, &entry->runtime_size))
    {
        printf("WARNING: Failed to read asset header for: %s\n", file_path);
        entry->signature = 0;
        entry->runtime_size = stat->size; // Fallback to file size
    }

    // Add to total memory requirement
    generator->total_memory += entry->runtime_size;

    printf("  - Signature: 0x%08X (%s)\n", entry->signature, get_asset_type_name(entry->signature));
    printf("  - Runtime size: %zu bytes\n", entry->runtime_size);
    printf("  - File size: %zu bytes\n", entry->file_size);
    printf("  - Variable name: %s\n", entry->var_name);
}

static bool read_asset_header(const char* file_path, uint32_t* signature, size_t* runtime_size)
{
    path_t asset_path;
    path_set(&asset_path, file_path);
    stream_t* stream = stream_load_from_file(NULL, &asset_path);
    if (!stream)
        return false;

    // Read asset header (16 bytes)
    if (stream_size(stream) < 16)
    {
        object_free(stream);
        return false;
    }

    // Read header fields
    *signature = stream_read_uint32(stream);
    *runtime_size = stream_read_uint32(stream);
    // Skip version and flags for manifest generation
    
    object_free(stream);
    return true;
}

static void generate_manifest_code(manifest_generator_t* generator)
{
    stream_t* stream = generator->manifest_stream;

    // Write header
    stream_write_raw_string(stream, "//\n");
    stream_write_raw_string(stream, "// Auto-generated asset manifest - DO NOT EDIT MANUALLY\n");
    stream_write_raw_string(stream, "// Generated by NoZ Game Engine Asset Importer\n");
    stream_write_raw_string(stream, "//\n\n");

    stream_write_raw_string(stream, "#include <noz/noz.h>\n\n");

    // Write memory requirement constant
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "#define ASSET_TOTAL_MEMORY %zu\n\n", generator->total_memory);
    stream_write_raw_string(stream, buffer);

    // Generate the g_assets structure
    organize_assets_by_type(generator);

    // Write load all function
    stream_write_raw_string(stream, "\n// Load all assets into single arena\n");
    stream_write_raw_string(stream, "bool noz_assets_load_all(void) {\n");
    stream_write_raw_string(stream, "    arena_allocator_t* arena = arena_allocator_create(ASSET_TOTAL_MEMORY);\n");
    stream_write_raw_string(stream, "    if (!arena) return false;\n\n");
    
    // Generate load calls for each asset
    for (size_t i = 0; i < array_length(generator->asset_entries); i++) {
        asset_entry_t* entry = (asset_entry_t*)array_get(generator->asset_entries, i);
        const char* type_name = get_asset_type_name(entry->signature);
        
        snprintf(buffer, sizeof(buffer), "    g_assets.%ss.%s = noz_%s_load(\"%s\", arena);\n",
                 type_name, entry->var_name, type_name, entry->path.value);
        stream_write_raw_string(stream, buffer);
    }
    
    stream_write_raw_string(stream, "\n    return true;\n");
    stream_write_raw_string(stream, "}\n");
}

static void write_asset_structure(manifest_generator_t* generator)
{
    stream_t* stream = generator->manifest_stream;

    // Write asset info structure
    stream_write_raw_string(stream, "typedef struct asset_info\n");
    stream_write_raw_string(stream, "{\n");
    stream_write_raw_string(stream, "    const char* path;\n");
    stream_write_raw_string(stream, "    uint32_t signature;\n");
    stream_write_raw_string(stream, "    size_t runtime_size;\n");
    stream_write_raw_string(stream, "    size_t file_size;\n");
    stream_write_raw_string(stream, "} asset_info_t;\n\n");

    if (array_length(generator->asset_entries) == 0)
    {
        stream_write_raw_string(stream, "// No assets found\n");
        stream_write_raw_string(stream, "static const asset_info_t g_assets[] = { {0} };\n\n");
        return;
    }

    // Write assets array
    stream_write_raw_string(stream, "static const asset_info_t g_assets[] = {\n");

    for (size_t i = 0; i < array_length(generator->asset_entries); i++)
    {
        asset_entry_t* entry = (asset_entry_t*)array_get(generator->asset_entries, i);
        char buffer[512];

        snprintf(buffer, sizeof(buffer), 
            "    { \"%s\", 0x%08X, %zu, %zu },\n",
            entry->path.value,
            entry->signature,
            entry->runtime_size,
            entry->file_size);

        stream_write_raw_string(stream, buffer);
    }

    stream_write_raw_string(stream, "};\n\n");
}

static void generate_folder_structure(manifest_generator_t* generator)
{
    // This could be expanded to generate a hierarchical structure
    // For now, we'll just provide a function to access assets by path
    stream_t* stream = generator->manifest_stream;

    stream_write_raw_string(stream, "// Asset lookup functions\n");
    stream_write_raw_string(stream, "const asset_info_t* asset_find_by_path(const char* path)\n");
    stream_write_raw_string(stream, "{\n");
    stream_write_raw_string(stream, "    if (!path) return NULL;\n\n");

    if (array_length(generator->asset_entries) > 0)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    for (size_t i = 0; i < %zu; i++)\n", array_length(generator->asset_entries));
        stream_write_raw_string(stream, buffer);
        stream_write_raw_string(stream, "    {\n");
        stream_write_raw_string(stream, "        if (strcmp(g_assets[i].path, path) == 0)\n");
        stream_write_raw_string(stream, "            return &g_assets[i];\n");
        stream_write_raw_string(stream, "    }\n\n");
    }

    stream_write_raw_string(stream, "    return NULL;\n");
    stream_write_raw_string(stream, "}\n\n");

    // Add function to get asset by index
    stream_write_raw_string(stream, "const asset_info_t* asset_get_by_index(size_t index)\n");
    stream_write_raw_string(stream, "{\n");

    if (array_length(generator->asset_entries) > 0)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    if (index >= %zu) return NULL;\n", array_length(generator->asset_entries));
        stream_write_raw_string(stream, buffer);
        stream_write_raw_string(stream, "    return &g_assets[index];\n");
    }
    else
    {
        stream_write_raw_string(stream, "    return NULL;\n");
    }

    stream_write_raw_string(stream, "}\n");
}

static const char* get_relative_path(const char* full_path, const char* base_path)
{
    if (!full_path || !base_path)
        return full_path;

    size_t base_len = strlen(base_path);
    if (strncmp(full_path, base_path, base_len) == 0)
    {
        // Skip the base path and any trailing slash
        const char* relative = full_path + base_len;
        while (*relative == '/' || *relative == '\\')
            relative++;
        return relative;
    }

    return full_path;
}

static char* path_to_var_name(const char* path)
{
    if (!path) return strdup("unknown");
    
    // Remove extension and convert path to valid C variable name
    char* var_name = malloc(strlen(path) + 1);
    if (!var_name) return strdup("unknown");
    
    const char* src = path;
    char* dst = var_name;
    
    // Skip leading directories until we get to the filename
    const char* last_slash = strrchr(path, '/');
    if (!last_slash) last_slash = strrchr(path, '\\');
    if (last_slash) src = last_slash + 1;
    
    // Copy and convert to valid C identifier
    while (*src && *src != '.') {
        if (isalnum(*src)) {
            *dst++ = tolower(*src);
        } else {
            *dst++ = '_';
        }
        src++;
    }
    *dst = '\0';
    
    return var_name;
}

static const char* get_asset_type_name(uint32_t signature)
{
    // Map signature to type using the asset system functions
    // For now, just check the signature directly
    switch (signature) {
        case 0x4E5A5458: return "texture";  // NZTX
        case 0x4E5A4D53: return "mesh";     // NZMS
        case 0x4E5A534E: return "sound";    // NZSN
        case 0x4E5A5348: return "shader";   // NZSH
        case 0x4E5A4D54: return "material"; // NZMT
        case 0x4E5A4654: return "font";     // NZFT
        default: return "unknown";
    }
}

static void organize_assets_by_type(manifest_generator_t* generator)
{
    stream_t* stream = generator->manifest_stream;
    
    // Create arrays for each asset type
    array_t* textures = array_create(sizeof(asset_entry_t*), 16);
    array_t* meshes = array_create(sizeof(asset_entry_t*), 16);
    array_t* sounds = array_create(sizeof(asset_entry_t*), 16);
    array_t* shaders = array_create(sizeof(asset_entry_t*), 16);
    array_t* materials = array_create(sizeof(asset_entry_t*), 16);
    array_t* fonts = array_create(sizeof(asset_entry_t*), 16);
    
    // Sort assets by type
    for (size_t i = 0; i < array_length(generator->asset_entries); i++) {
        asset_entry_t* entry = (asset_entry_t*)array_get(generator->asset_entries, i);
        const char* type_name = get_asset_type_name(entry->signature);
        
        if (strcmp(type_name, "texture") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(textures);
            *ptr = entry;
        } else if (strcmp(type_name, "mesh") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(meshes);
            *ptr = entry;
        } else if (strcmp(type_name, "sound") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(sounds);
            *ptr = entry;
        } else if (strcmp(type_name, "shader") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(shaders);
            *ptr = entry;
        } else if (strcmp(type_name, "material") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(materials);
            *ptr = entry;
        } else if (strcmp(type_name, "font") == 0) {
            asset_entry_t** ptr = (asset_entry_t**)array_push(fonts);
            *ptr = entry;
        }
    }
    
    // Generate the g_assets structure
    stream_write_raw_string(stream, "// Global asset structure\n");
    stream_write_raw_string(stream, "struct {\n");
    
    // Generate texture structure
    if (array_length(textures) > 0) {
        stream_write_raw_string(stream, "    struct {\n");
        for (size_t i = 0; i < array_length(textures); i++) {
            asset_entry_t** entry_ptr = (asset_entry_t**)array_get(textures, i);
            asset_entry_t* entry = *entry_ptr;
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "        noz_texture_t* %s;\n", entry->var_name);
            stream_write_raw_string(stream, buffer);
        }
        stream_write_raw_string(stream, "    } textures;\n");
    }
    
    // Generate mesh structure
    if (array_length(meshes) > 0) {
        stream_write_raw_string(stream, "    struct {\n");
        for (size_t i = 0; i < array_length(meshes); i++) {
            asset_entry_t** entry_ptr = (asset_entry_t**)array_get(meshes, i);
            asset_entry_t* entry = *entry_ptr;
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "        noz_mesh_t* %s;\n", entry->var_name);
            stream_write_raw_string(stream, buffer);
        }
        stream_write_raw_string(stream, "    } meshes;\n");
    }
    
    // Generate sound structure
    if (array_length(sounds) > 0) {
        stream_write_raw_string(stream, "    struct {\n");
        for (size_t i = 0; i < array_length(sounds); i++) {
            asset_entry_t** entry_ptr = (asset_entry_t**)array_get(sounds, i);
            asset_entry_t* entry = *entry_ptr;
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "        noz_sound_t* %s;\n", entry->var_name);
            stream_write_raw_string(stream, buffer);
        }
        stream_write_raw_string(stream, "    } sounds;\n");
    }
    
    stream_write_raw_string(stream, "} g_assets;\n");
    
    // Clean up
    array_free(textures);
    array_free(meshes);
    array_free(sounds);
    array_free(shaders);
    array_free(materials);
    array_free(fonts);
}