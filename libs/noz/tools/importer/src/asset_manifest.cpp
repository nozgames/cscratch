//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "asset_manifest.h"
#include <noz/platform.h>

typedef struct asset_entry
{
    OBJECT_BASE;
    path_t path;
    uint32_t signature;
    size_t runtime_size;
    size_t file_size;
    name_t var_name;
} asset_entry_t;

typedef struct manifest_generator
{
    list_t* asset_entries;
    size_t total_memory;
    path_t output_dir;
    stream_t* manifest_stream;
} manifest_generator_t;

// Forward declarations
static void scan_asset_file(path_t* file_path, file_stat_t* stat, void* user_data);
static bool read_asset_header(char* file_path, uint32_t* signature, size_t* runtime_size);
static void generate_manifest_code(manifest_generator_t* generator);
static void write_asset_structure(manifest_generator_t* generator);
static void generate_folder_structure(manifest_generator_t* generator);
static void path_to_var_name(name_t* dst, char* path);
static void write_asset_type_struct(stream_t* stream, list_t* assets, type_t type);
static void organize_assets_by_type(manifest_generator_t* generator);

bool asset_manifest_generate(char* output_directory, char* manifest_output_path)
{
    if (!output_directory || !manifest_output_path)
    {
        printf("ERROR: Invalid parameters for manifest generation\n");
        return false;
    }

    // Initialize manifest generator
    manifest_generator_t generator = {0};
    path_set(&generator.output_dir, output_directory);
    
    generator.asset_entries = list_alloc(NULL, 64);
    if (!generator.asset_entries)
    {
        printf("ERROR: Failed to create asset entries array\n");
        return false;
    }

    generator.manifest_stream = stream_alloc(NULL, 1024);
    if (!generator.manifest_stream)
    {
        printf("ERROR: Failed to create manifest stream\n");
        object_free(generator.asset_entries);
        return false;
    }

    generator.total_memory = 0;

    // Check if output directory exists
    file_stat_t dir_stat;
    if (!file_stat(&generator.output_dir, &dir_stat))
    {
        
        // Generate an empty manifest
        generate_manifest_code(&generator);
        
        // Save the manifest
        path_t manifest_path;
        path_set(&manifest_path, manifest_output_path);
        bool success = stream_save_to_file(generator.manifest_stream, &manifest_path);
        
        // Clean up
        object_free(generator.manifest_stream);
        object_free(generator.asset_entries);
        
        return success;
    }

    if (!dir_stat.is_directory)
    {
        printf("ERROR: '%s' is not a directory\n", output_directory);
        object_free(generator.manifest_stream);
        object_free(generator.asset_entries);
        return false;
    }

    // Scan all files in the output directory recursively
    if (!directory_enum_files(&generator.output_dir, scan_asset_file, &generator))
    {
        printf("ERROR: Failed to enumerate files in directory: %s\n", output_directory);
        object_free(generator.manifest_stream);
        object_free(generator.asset_entries);
        return false;
    }


    // Generate the manifest C code
    generate_manifest_code(&generator);

    // Save the manifest to file
    path_t manifest_path;
    path_set(&manifest_path, manifest_output_path);
    bool success = stream_save_to_file(generator.manifest_stream, &manifest_path);
    if (!success)
    {
        printf("ERROR: Failed to save manifest to: %s\n", manifest_output_path);
    }

    // Clean up
    object_free(generator.manifest_stream);
    object_free(generator.asset_entries);

    return success;
}

static void scan_asset_file(path_t* file_path, file_stat_t* stat, void* user_data)
{
    manifest_generator_t* generator = (manifest_generator_t*)user_data;

    // Skip directories
    if (stat->is_directory)
        return;

    // Check for known asset extensions using path API (without dot)
    bool is_asset = path_has_extension(file_path, "nzt") ||   // NoZ Texture
                    path_has_extension(file_path, "nzm") ||   // NoZ Mesh
                    path_has_extension(file_path, "nzs") ||   // NoZ Sound
                    path_has_extension(file_path, "nzsh") ||  // NoZ Shader
                    path_has_extension(file_path, "nzmt") ||  // NoZ Material
                    path_has_extension(file_path, "nzf");     // NoZ Font

    if (!is_asset)
        return;

    // Make path relative to output_dir first
    path_t relative_path;
    path_make_relative(&relative_path, file_path, &generator->output_dir);
    
    // Remove extension for comparison and storage
    path_set_extension(&relative_path, "");
    
    // Check if this asset is already in the list (compare without extensions)
    for (size_t i = 0; i < list_count(generator->asset_entries); i++)
    {
        asset_entry_t* existing = (asset_entry_t*)list_get(generator->asset_entries, i);
        if (path_eq(&existing->path, &relative_path))
        {
            // Asset already in list, skip it
            return;
        }
    }

    asset_entry_t* entry = (asset_entry_t*)object_alloc(object_allocator(generator->asset_entries), sizeof(asset_entry_t), type_unknown);
    if (!entry)
    {
        printf("error: out_of_memory\n");
        return;
    }

	list_add(generator->asset_entries, (object_t*)entry);

    // Copy the relative path we already computed (extension already removed)
    path_copy(&entry->path, &relative_path);
    
    entry->file_size = stat->size;

    // Generate variable name from path (without extension)
    path_to_var_name(&entry->var_name, entry->path.value);

    // Read asset header to get signature and runtime size
    if (!read_asset_header(file_path->value, &entry->signature, &entry->runtime_size))
    {
        printf("WARNING: Failed to read asset header for: %s\n", file_path->value);
        entry->signature = 0;
        entry->runtime_size = stat->size; // Fallback to file size
    }

    // Add to total memory requirement
    generator->total_memory += entry->runtime_size;
}

static bool read_asset_header(char* file_path, uint32_t* signature, size_t* runtime_size)
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
    stream_write_raw_cstr(stream, 
        "//\n"
        "// Auto-generated asset manifest - DO NOT EDIT MANUALLY\n"
        "// Generated by NoZ Game Engine Asset Importer\n"
        "//\n\n"
        "// @includes\n"
        "#include <noz/noz.h>\n\n");

    // Write memory requirement constant and allocator pointer
    stream_write_raw_cstr(stream, "// @constants\n");
    stream_write_raw_cstr(stream, "#define ASSET_TOTAL_MEMORY %zu\n\n", generator->total_memory);
    
    stream_write_raw_cstr(stream, "// @globals\n");
    stream_write_raw_cstr(stream, "static arena_allocator_t* g_asset_allocator = NULL;\n\n");

    // Generate the g_assets structure
    organize_assets_by_type(generator);

    // Write load all function
    stream_write_raw_cstr(stream, 
        "// @init\n"
        "bool assets_init(void)\n"
        "{\n"
        "    if (g_asset_allocator != NULL)\n"
        "        return false; // Already initialized\n\n"
        "    g_asset_allocator = arena_allocator_create(ASSET_TOTAL_MEMORY);\n"
        "    if (!g_asset_allocator)\n"
        "        return false;\n\n");
    
    // Generate load calls for each asset
    for (size_t i = 0; i < list_count(generator->asset_entries); i++) 
    {
        asset_entry_t* entry = (asset_entry_t*)list_get(generator->asset_entries, i);
        type_t asset_type = asset_signature_to_type(entry->signature);        
        if (asset_type == type_invalid)
			continue;

        const char* type_name = asset_type_to_string(asset_type);
        assert(type_name);
        stream_write_raw_cstr(
            stream,
            "    NOZ_ASSET_LOAD(%s, \"%s\", g_assets.%ss.%s);\n",
            type_name,
            entry->path.value,
            type_name,
            entry->var_name.value);
    }
    
    stream_write_raw_cstr(stream, "\n    return true;\n}\n\n");
    
    // Write assets_uninit function
    stream_write_raw_cstr(stream,
        "// @uninit\n"
        "void assets_uninit(void)\n"
        "{\n"
        "    if (g_asset_allocator != NULL)\n"
        "    {\n"
        "        arena_allocator_destroy(g_asset_allocator);\n"
        "        g_asset_allocator = NULL;\n"
        "        \n"
        "        // Clear all asset pointers\n"
        "        memset(&g_assets, 0, sizeof(g_assets));\n"
        "    }\n"
        "}\n");
}

static void write_asset_structure(manifest_generator_t* generator)
{
    stream_t* stream = generator->manifest_stream;

    // Write asset info structure
    stream_write_raw_cstr(stream, 
        "typedef struct asset_info\n"
        "{\n"
        "    char* path;\n"
        "    uint32_t signature;\n"
        "    size_t runtime_size;\n"
        "    size_t file_size;\n"
        "} asset_info_t;\n\n");

    if (list_count(generator->asset_entries) == 0)
    {
        stream_write_raw_cstr(stream, 
            "// No assets found\n"
            "static asset_info_t g_assets[] = { {0} };\n\n");
        return;
    }

    // Write assets array
    stream_write_raw_cstr(stream, "static asset_info_t g_assets[] = {\n");

    for (size_t i = 0; i < list_count(generator->asset_entries); i++)
    {
        asset_entry_t* entry = (asset_entry_t*)list_get(generator->asset_entries, i);
        char buffer[512];

        snprintf(buffer, sizeof(buffer), 
            "    { \"%s\", 0x%08X, %zu, %zu },\n",
            entry->path.value,
            entry->signature,
            entry->runtime_size,
            entry->file_size);

        stream_write_raw_cstr(stream, "%s", buffer);
    }

    stream_write_raw_cstr(stream, "};\n\n");
}

static void generate_folder_structure(manifest_generator_t* generator)
{
    // This could be expanded to generate a hierarchical structure
    // For now, we'll just provide a function to access assets by path
    stream_t* stream = generator->manifest_stream;

    stream_write_raw_cstr(stream, "// Asset lookup functions\n");
    stream_write_raw_cstr(stream, "asset_info_t* asset_find_by_path(char* path)\n");
    stream_write_raw_cstr(stream, "{\n");
    stream_write_raw_cstr(stream, "    if (!path) return NULL;\n\n");

    if (list_count(generator->asset_entries) > 0)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    for (size_t i = 0; i < %zu; i++)\n", list_count(generator->asset_entries));
        stream_write_raw_cstr(stream, buffer);
        stream_write_raw_cstr(stream, "    {\n");
        stream_write_raw_cstr(stream, "        if (strcmp(g_assets[i].path, path) == 0)\n");
        stream_write_raw_cstr(stream, "            return &g_assets[i];\n");
        stream_write_raw_cstr(stream, "    }\n\n");
    }

    stream_write_raw_cstr(stream, "    return NULL;\n");
    stream_write_raw_cstr(stream, "}\n\n");

    // Add function to get asset by index
    stream_write_raw_cstr(stream, "asset_info_t* asset_get_by_index(size_t index)\n");
    stream_write_raw_cstr(stream, "{\n");

    if (list_count(generator->asset_entries) > 0)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "    if (index >= %zu) return NULL;\n", list_count(generator->asset_entries));
        stream_write_raw_cstr(stream, buffer);
        stream_write_raw_cstr(stream, "    return &g_assets[index];\n");
    }
    else
    {
        stream_write_raw_cstr(stream, "    return NULL;\n");
    }

    stream_write_raw_cstr(stream, "}\n");
}


static void path_to_var_name(name_t* dst, char* path_str)
{
    assert(dst);
    
    if (!path_str)
    {
        name_set(dst, "unknown");
        return;
    }
    
    // Convert to path_t and get filename without extension
    path_t path;
    path_set(&path, path_str);
    
    name_t filename;
    path_filename_without_extension(&path, &filename);
    
    // If filename is empty, use "unknown"
    if (name_empty(&filename))
    {
        name_set(dst, "unknown");
        return;
    }
    
    // Convert filename to valid C variable name
    char* src = filename.value;
    char* out = dst->value;
    size_t out_len = 0;
    
    // Copy and convert to valid C identifier
    while (*src && out_len < sizeof(dst->value) - 1)
    {
        if (isalnum(*src))
            out[out_len++] = tolower(*src);
        else
            out[out_len++] = '_';
        src++;
    }
    
    out[out_len] = '\0';
    dst->length = out_len;
    
    // Check for C keywords and add underscore prefix if needed
    if (strcmp(dst->value, "default") == 0 ||
        strcmp(dst->value, "switch") == 0 ||
        strcmp(dst->value, "case") == 0 ||
        strcmp(dst->value, "break") == 0 ||
        strcmp(dst->value, "continue") == 0 ||
        strcmp(dst->value, "return") == 0 ||
        strcmp(dst->value, "if") == 0 ||
        strcmp(dst->value, "else") == 0 ||
        strcmp(dst->value, "for") == 0 ||
        strcmp(dst->value, "while") == 0 ||
        strcmp(dst->value, "do") == 0 ||
        strcmp(dst->value, "goto") == 0 ||
        strcmp(dst->value, "void") == 0 ||
        strcmp(dst->value, "int") == 0 ||
        strcmp(dst->value, "float") == 0 ||
        strcmp(dst->value, "double") == 0 ||
        strcmp(dst->value, "char") == 0 ||
        strcmp(dst->value, "const") == 0 ||
        strcmp(dst->value, "static") == 0 ||
        strcmp(dst->value, "struct") == 0 ||
        strcmp(dst->value, "union") == 0 ||
        strcmp(dst->value, "enum") == 0 ||
        strcmp(dst->value, "typedef") == 0)
    {
        // Add underscore prefix for C keywords
        memmove(dst->value + 1, dst->value, dst->length + 1);
        dst->value[0] = '_';
        dst->length++;
    }
}

static void write_asset_type_struct(stream_t* stream, list_t* assets, type_t type)
{
    if (list_count(assets) == 0)
        return;
    
    const char* type_name = asset_type_to_string(type);
    if (type_name == NULL)
        return;
   
    // Write struct opening
    stream_write_raw_cstr(stream, "    struct\n    {\n");
    
    // Write each asset field
    for (size_t i = 0, c = list_count(assets); i < c; i++)
    {
        asset_entry_t* entry = (asset_entry_t*)list_get(assets, i);
        stream_write_raw_cstr(stream, "        %s_t* %s;\n", type_name, entry->var_name.value);
    }
    
    // Write struct closing with collection name (plural)
    stream_write_raw_cstr(stream, "    } %ss;\n", type_name);
}

static void organize_assets_by_type(manifest_generator_t* generator)
{
    stream_t* stream = generator->manifest_stream;
    
    // Create arrays for each asset type
    list_t* textures = list_alloc(NULL, 16);
    list_t* meshes = list_alloc(NULL, 16);
    list_t* sounds = list_alloc(NULL, 16);
    list_t* shaders = list_alloc(NULL, 16);
    list_t* materials = list_alloc(NULL, 16);
    list_t* fonts = list_alloc(NULL, 16);
    
    // Sort assets by type
    for (size_t i = 0, c = list_count(generator->asset_entries); i < c; i++)
    {
        asset_entry_t* entry = (asset_entry_t*)list_get(generator->asset_entries, i);
        type_t asset_type = asset_signature_to_type(entry->signature);
        
        switch (asset_type) {
            case type_texture:
                list_add(textures, (object_t*)entry);
                break;
            case type_mesh:
                list_add(meshes, (object_t*)entry);
                break;
            case type_sound:
                list_add(sounds, (object_t*)entry);
                break;
            case type_shader:
                list_add(shaders, (object_t*)entry);
                break;
            case type_material:
                list_add(materials, (object_t*)entry);
                break;
            case type_font:
                list_add(fonts, (object_t*)entry);
                break;
            default:
                // Unknown type, skip
                break;
        }
    }
    
    // Generate the g_assets structure
    stream_write_raw_cstr(stream, "// @assets\n");
    stream_write_raw_cstr(stream, "struct\n{\n");
    
    // Check if we have any assets at all
    bool has_any_assets = list_count(textures) > 0 || list_count(meshes) > 0 || 
                          list_count(sounds) > 0 || list_count(shaders) > 0 || 
                          list_count(materials) > 0 || list_count(fonts) > 0;
    
    if (has_any_assets)
    {
        // Generate structures for each asset type
        write_asset_type_struct(stream, textures, type_texture);
        write_asset_type_struct(stream, meshes, type_mesh);
        write_asset_type_struct(stream, sounds, type_sound);
        write_asset_type_struct(stream, shaders, type_shader);
        write_asset_type_struct(stream, materials, type_material);
        write_asset_type_struct(stream, fonts, type_font);
    }
    else
    {
        // Add dummy member to avoid empty struct error
        stream_write_raw_cstr(stream, "    void* _dummy; // Placeholder for empty asset structure\n");
    }
    
    stream_write_raw_cstr(stream, "} g_assets;\n\n");
    
    // Clean up
    object_free(textures);
    object_free(meshes);
    object_free(sounds);
    object_free(shaders);
    object_free(materials);
    object_free(fonts);
}