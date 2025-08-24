#include <noz/noz.h>
#include <file_watcher.h>
#include <noz/platform.h>
#include <csignal>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

AssetImporterTraits* GetShaderImporterTraits();
AssetImporterTraits* GetTextureImporterTraits();

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
void ProcessImportQueue(std::vector<AssetImporterTraits*>& importers);

void signal_handler(int sig)
{
    if (sig != SIGINT)
        return;

    printf("\nShutting down...\n");
    g_running = false;
}

int main(int argc, char* argv[])
{
    // Initialize importers array - now including texture importer
    std::vector importers = {
        GetShaderImporterTraits(), 
        GetTextureImporterTraits()
    };

    // Set up signal handler for Ctrl-C
    signal(SIGINT, signal_handler);

    Path config_path;
    path_set(&config_path, "./importer.cfg");
    g_config = LoadProps(nullptr, &config_path);
    if (!g_config)
    {
        printf("missing configuration '%s'\n", config_path.value);
        return 1;
    }

    printf("loaded configuration '%s'\n", config_path.value);

    // Initialize file watcher
    InitFileWatcher(500);

    // Get source directories from config
    if (!HasKey(g_config, "source"))
    {
        printf("No [source] section found in config\n");
        FreeObject(g_config);
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
        ProcessImportQueue(importers);

        // Sleep briefly to avoid busy waiting
        thread_sleep_ms(100); // 100ms
    }

    // Clean up
    ShutdownFileWatcher();
    FreeObject(g_config);
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

    // Find an importer that can handle this file
    AssetImporterTraits* selected_importer = nullptr;

    for (auto* importer : importers)
    {
        if (importer && importer->can_import && importer->can_import(file_path))
        {
            selected_importer = importer;
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

void ProcessImportQueue(std::vector<AssetImporterTraits*>& importers)
{
    if (g_import_queue.empty())
        return;

    // Get output directory from config
    const char* output_dir = GetString(g_config, "output.directory", "assets");
    
    // Convert to filesystem::path
    fs::path output_path = fs::absolute(fs::path(output_dir));

    // Ensure output directory exists
    fs::create_directories(output_path);

    std::vector<ImportJob> remaining_jobs;
    bool made_progress = true;

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
                    job.importer->import_func(job.source_path, output_path, g_config);
                }
                made_progress = true;
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
}