#include <algorithm>
#include <csignal>
#include <file_watcher.h>
#include <filesystem>
#include <iostream>
#include <noz/noz.h>
#include <noz/platform.h>
#include <string>
#include <vector>
#include "asset_manifest.h"

namespace fs = std::filesystem;

AssetImporterTraits* GetShaderImporterTraits();
AssetImporterTraits* GetTextureImporterTraits();
AssetImporterTraits* GetFontImporterTraits();
AssetImporterTraits* GetMeshImporterTraits();
AssetImporterTraits* GetStyleSheetImporterTraits();

struct ImportJob
{
    fs::path source_path;
    AssetImporterTraits* importer;
    
    ImportJob(const fs::path& path, AssetImporterTraits* imp)
        : source_path(path), importer(imp) {}
};

static std::vector<ImportJob> g_import_queue;
static Props* g_config = nullptr;
static volatile bool g_running = true;

void ProcessFileChange(const fs::path& file_path, FileChangeType change_type, std::vector<AssetImporterTraits*>& importers);
bool ProcessImportQueue(std::vector<AssetImporterTraits*>& importers);

// Helper function to derive file extension from asset signature
std::string GetExtensionFromSignature(asset_signature_t signature)
{
    // Convert signature to 4 character string (little endian to big endian)
    char sig_chars[5] = {};
    sig_chars[0] = (signature >> 24) & 0xFF;
    sig_chars[1] = (signature >> 16) & 0xFF;
    sig_chars[2] = (signature >> 8) & 0xFF;
    sig_chars[3] = signature & 0xFF;
    
    // Convert to lowercase extension
    std::string ext = ".";
    for (int i = 0; i < 4; i++)
    {
        ext += std::tolower(sig_chars[i]);
    }
    
    return ext;
}

void signal_handler(int sig)
{
    if (sig != SIGINT)
        return;

    printf("\nShutting down...\n");
    g_running = false;
}

int main(int argc, char* argv[])
{
    // Initialize importers array - now including all asset importers
    std::vector importers = {
        GetShaderImporterTraits(), 
        GetTextureImporterTraits(),
        GetFontImporterTraits(),
        GetMeshImporterTraits(),
        GetStyleSheetImporterTraits()
    };

    // Set up signal handler for Ctrl-C
    signal(SIGINT, signal_handler);

    std::filesystem::path config_path = "./importer.cfg";
    Stream* config_stream = LoadStream(nullptr, config_path);
    if (config_stream)
    {
        g_config = LoadProps(nullptr, config_stream);
        Destroy(config_stream);
    }
    else
    {
        g_config = nullptr;
    }
    if (!g_config)
    {
        printf("missing configuration '%s'\n", config_path.string().c_str());
        return 1;
    }

    printf("loaded configuration '%s'\n", config_path.string().c_str());

    // Initialize file watcher
    InitFileWatcher(500);

    // Get source directories from config
    if (!HasKey(g_config, "source"))
    {
        printf("No [source] section found in config\n");
        Destroy(g_config);
        ShutdownFileWatcher();
        return 1;
    }

    // Add directories to watch (file watcher will auto-start when first directory is added)
    printf("Adding directories to watch:\n");
    size_t source_count = GetListCount(g_config, "source");
    for (size_t i = 0; i < source_count; i++)
    {
        const char* dir = GetListElement(g_config, "source", i, "");
        printf("  - %s\n", dir);
        if (!WatchDirectory(fs::path(dir)))
        {
            printf("    WARNING: Failed to add directory '%s'\n", dir);
        }
    }

    printf("\nWatching for file changes... Press Ctrl-C to exit\n\n");

    // Main loop - watch for file changes
    while (g_running)
    {
        FileChangeEvent event;
        while (GetFileChangeEvent(&event))
        {
            // Process file changes for import (silently)
            ProcessFileChange(event.path, event.type, importers);
        }

        // Process any queued imports
        bool imports_processed = ProcessImportQueue(importers);
        
        // Generate asset manifest if any imports were processed
        if (imports_processed)
        {
            const char* output_dir = GetString(g_config, "output.directory", "assets");
            const char* manifest_path = GetString(g_config, "output.manifest", "src/assets.cpp");
            
            if (GenerateAssetManifest(fs::path(output_dir), fs::path(manifest_path), importers, g_config))
            {
                std::cout << "Generated asset manifest: " << manifest_path << std::endl;
            }
            else
            {
                std::cerr << "Failed to generate asset manifest" << std::endl;
            }
        }

        // Sleep briefly to avoid busy waiting
        thread_sleep_ms(100); // 100ms
    }

    // Clean up
    ShutdownFileWatcher();
    Destroy(g_config);
    g_import_queue.clear();
    return 0;
}

void ProcessFileChange(const fs::path& file_path, FileChangeType change_type, std::vector<AssetImporterTraits*>& importers)
{
    if (change_type == FILE_CHANGE_TYPE_DELETED)
        return; // Don't process deleted files

    // Check if this is a .meta file
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".meta")
    {
        // Remove .meta extension to get the asset file path
        fs::path asset_path = file_path;
        asset_path.replace_extension("");
        
        // Check if the associated asset file exists
        if (fs::exists(asset_path) && fs::is_regular_file(asset_path))
        {
            ProcessFileChange(asset_path, change_type, importers);
        }
        return;
    }

    // Find an importer that can handle this file based on extension
    AssetImporterTraits* selected_importer = nullptr;
    
    std::string file_ext = file_path.extension().string();
    std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), ::tolower);

    for (auto* importer : importers)
    {
        if (importer && importer->file_extensions)
        {
            // Check if this importer supports the file extension
            for (const char** ext_ptr = importer->file_extensions; *ext_ptr != nullptr; ++ext_ptr)
            {
                if (file_ext == *ext_ptr)
                {
                    selected_importer = importer;
                    break;
                }
            }
            if (selected_importer)
                break;
        }
    }

    // has an importer?
    if (!selected_importer)
        return;

    // Check if already in the queue
    auto it = std::find_if(g_import_queue.begin(), g_import_queue.end(),
        [&file_path](const ImportJob& job) {
            return job.source_path == file_path;
        });
    
    if (it != g_import_queue.end())
        return; // Already in queue

    // Add new job to queue
    g_import_queue.emplace_back(file_path, selected_importer);
}

bool ProcessImportQueue(std::vector<AssetImporterTraits*>& importers)
{
    if (g_import_queue.empty())
        return false;

    // Get output directory from config
    const char* output_dir = GetString(g_config, "output.directory", "assets");
    
    // Convert to filesystem::path
    fs::path output_path = fs::absolute(fs::path(output_dir));

    // Ensure output directory exists
    fs::create_directories(output_path);

    std::vector<ImportJob> remaining_jobs;
    bool made_progress = true;
    bool any_imports_processed = false;

    // Keep processing until no more progress is made
    while (made_progress && !g_import_queue.empty())
    {
        made_progress = false;
        remaining_jobs.clear();

        for (const auto& job : g_import_queue)
        {
            bool can_import_now = true;

            // Check dependencies if the importer supports it
            if (job.importer->does_depend_on)
            {
                // Check if any files this one depends on are still in the queue
                for (const auto& other_job : g_import_queue)
                {
                    if (&job == &other_job)
                        continue; // Don't check against self

                    if (job.importer->does_depend_on(job.source_path, other_job.source_path))
                    {
                        can_import_now = false;
                        break;
                    }
                }
            }

            if (can_import_now)
            {
                // Import this file
                if (job.importer->import_func)
                {
                    try
                    {
                        // Create output stream
                        Stream* output_stream = CreateStream(nullptr, 4096);
                        if (!output_stream)
                        {
                            std::cout << job.source_path.string() << ": error: Failed to create output stream" << std::endl;
                            continue;
                        }
                        
                        // Load .meta file or create default props
                        fs::path meta_path = fs::path(job.source_path.string() + ".meta");
                        Props* meta_props = nullptr;
                        
                        if (fs::exists(meta_path))
                        {
                            Stream* meta_stream = LoadStream(nullptr, meta_path);
                            if (meta_stream)
                            {
                                meta_props = LoadProps(nullptr, meta_stream);
                                Destroy(meta_stream);
                            }
                        }
                        
                        // Create default props if meta file failed to load
                        if (!meta_props)
                        {
                            meta_props = CreateProps(nullptr);
                        }
                        
                        // Call the importer
                        job.importer->import_func(job.source_path, output_stream, g_config, meta_props);
                        
                        // Clean up meta props
                        Destroy(meta_props);
                        
                        // Build output file path with correct extension
                        fs::path relative_path;
                        bool found_relative = false;
                        
                        // Get source directories from config and find the relative path
                        size_t source_count = GetListCount(g_config, "source");
                        for (size_t i = 0; i < source_count; i++)
                        {
                            const char* source_dir_str = GetListElement(g_config, "source", i, "");
                            if (source_dir_str[0] != '\0')
                            {
                                fs::path source_dir(source_dir_str);
                                std::error_code ec;
                                relative_path = fs::relative(job.source_path, source_dir, ec);
                                if (!ec && !relative_path.empty() && relative_path.string().find("..") == std::string::npos)
                                {
                                    found_relative = true;
                                    break;
                                }
                            }
                        }
                        
                        if (!found_relative)
                        {
                            relative_path = job.source_path.filename();
                        }
                        
                        // Build final output path with extension derived from signature
                        fs::path final_path = output_path / relative_path;
                        std::string derived_extension = GetExtensionFromSignature(job.importer->signature);
                        final_path.replace_extension(derived_extension);
                        
                        // Ensure output directory exists
                        fs::create_directories(final_path.parent_path());
                        
                        // Save the output stream
                        if (!SaveStream(output_stream, final_path))
                        {
                            Destroy(output_stream);
                            throw std::runtime_error("Failed to save output file");
                        }
                        
                        Destroy(output_stream);
                        
                        // Print success message using the relative path we computed
                        fs::path asset_path = relative_path;
                        asset_path.replace_extension("");
                        std::string asset_name = asset_path.string();
                        std::replace(asset_name.begin(), asset_name.end(), '\\', '/');
                        std::cout << "Imported '" << asset_name << "'" << std::endl;
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << job.source_path.string() << ": error: " << e.what() << std::endl;
                        continue; // Skip to next job
                    }
                }
                made_progress = true;
                any_imports_processed = true;
            }
            else
            {
                // Keep this job for next iteration
                remaining_jobs.push_back(job);
            }
        }

        // Swap the queues - remaining_jobs becomes the new import queue
        g_import_queue = std::move(remaining_jobs);
    }
    
    return any_imports_processed;
}