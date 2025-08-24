
#include "asset_manifest.h"
#include <noz/file_watcher.h>
#include <noz/noz.h>
#include <noz/platform.h>
#include <signal.h>

asset_importer_traits_t* GetShaderImporterTraits();

typedef struct import_job
{
    OBJECT_BASE;
    path_t source_path;
    asset_importer_traits_t* importer;
} import_job_t;

static list_t* g_import_queue = NULL;
static Props* g_config = NULL;

void process_file_change(path_t* file_path, file_change_type_t change_type, asset_importer_traits_t** importers);
void process_import_queue(asset_importer_traits_t** importers);
import_job_t* to_import_job_impl(void* job)
{
    return (import_job_t*)to_object((Object*)job, type_unknown);
}

static volatile bool g_running = true;

void signal_handler(int sig)
{
    if (sig != SIGINT)
        return;

    printf("\nShutting down...\n");
    g_running = false;
}

int main(int argc, char* argv[])
{
    // Initialize importers array
    asset_importer_traits_t* importers[] = {GetShaderImporterTraits(), NULL};

    // Set up signal handler for Ctrl-C
    signal(SIGINT, signal_handler);

    g_import_queue = list_alloc(NULL, 1024);

    path_t path;
    path_set(&path, "./importer.cfg");
    g_config = props_load_from_file(NULL, &path);
    if (!g_config)
    {
        printf("missing configuration '%s'\n", path.value);
        return 1;
    }

    printf("loaded configuration '%s'\n", path.value);

    // Initialize file watcher
    file_watcher_init(500);

    // Get source directories from config
    if (!props_has_key(g_config, "source"))
    {
        printf("No [source] section found in config\n");
        Free(g_config);
        file_watcher_shutdown();
        return 1;
    }

    // Add directories to watch
    printf("Adding directories to watch:\n");
    size_t source_count = props_get_list_count(g_config, "source");
    for (size_t i = 0; i < source_count; i++)
    {
        const char* dir = props_get_list_item(g_config, "source", i, "");
        printf("  - %s\n", dir);
        if (!file_watcher_add_directory(dir))
        {
            printf("    WARNING: Failed to add directory '%s'\n", dir);
        }
    }

    // Start watching
    if (!file_watcher_start())
    {
        printf("Failed to start file watcher\n");
        Free(g_config);
        file_watcher_shutdown();
        return 1;
    }

    printf("\nWatching for file changes... Press Ctrl-C to exit\n\n");

    // Main loop - watch for file changes
    while (g_running)
    {
        file_change_event_t event;
        while (file_watcher_poll(&event))
        {
            // Process file changes for import (silently)
            process_file_change(&event.path, event.type, importers);
        }

        // Process any queued imports
        process_import_queue(importers);

        // Sleep briefly to avoid busy waiting
        thread_sleep_ms(100); // 100ms
    }

    // Clean up
    file_watcher_stop();
    file_watcher_shutdown();
    Free(g_config);
    Free(g_import_queue);
    return 0;
}

static bool import_job_path_predicate(void* o, void* data)
{
    path_t* path = (path_t*)data;
    import_job_t* job = to_import_job_impl(o);
    return path_eq(&job->source_path, path);
}

void process_file_change(path_t* file_path, file_change_type_t change_type, asset_importer_traits_t** importers)
{
    if (change_type == file_change_type_deleted)
        return; // Don't process deleted files

    // Check if this is a .meta file
    if (path_has_extension(file_path, "meta"))
    {
        // Remove .meta extension to get the asset file path
        path_t asset_path;
        path_copy(&asset_path, file_path);

        // Remove the .meta extension
        const char* ext = path_extension(&asset_path);
        if (ext && strcmp(ext, "meta") == 0)
        {
            // Find the last dot and truncate there
            char* last_dot = strrchr(asset_path.value, '.');
            if (last_dot)
            {
                *last_dot = '\0';
                asset_path.length = strlen(asset_path.value);

                // Process the associated asset file
                file_stat_t stat;
                if (file_stat(&asset_path, &stat) && stat.is_regular_file)
                {
                    process_file_change(&asset_path, change_type, importers);
                }
            }
        }
        return;
    }

    // Find an importer that can handle this file
    path_t source_path;
    path_copy(&source_path, file_path);
    asset_importer_traits_t* selected_importer = NULL;

    for (int i = 0; importers[i] != NULL; i++)
    {
        if (importers[i]->can_import && importers[i]->can_import(&source_path))
        {
            selected_importer = importers[i];
            break;
        }
    }

    // has an importer?
    if (!selected_importer)
        return;

    // is in the list already?
    if (list_find_predicate(g_import_queue, import_job_path_predicate, &source_path) >= 0)
        return;

    import_job_t* new_job = (import_job_t*)Alloc(NULL, sizeof(import_job_t), type_unknown);
    if (!new_job)
        return;

    new_job->source_path = source_path;
    new_job->importer = selected_importer;
    list_add(g_import_queue, (Object*)new_job);
}

void process_import_queue(asset_importer_traits_t** importers)
{
    if (!g_import_queue || list_empty(g_import_queue))
        return;

    // Get output directory from config
    const char* output_dir = props_get_string(g_config, "output.directory", "assets");

    // Ensure output directory exists (make it absolute)
    path_t output_path, output_path_rel;
    path_set(&output_path_rel, output_dir);
    path_make_absolute(&output_path, &output_path_rel);

    if (!directory_create_recursive(&output_path))
    {
        printf("Failed to create directory: %s\n", output_path.value);
    }
    list_t* remaining_jobs = list_alloc(NULL, list_count(g_import_queue));
    bool made_progress = true;

    // Keep processing until no more progress is made
    while (made_progress && !list_empty(g_import_queue))
    {
        made_progress = false;
        list_clear(remaining_jobs);

        for (size_t i = 0; i < list_count(g_import_queue); i++)
        {
            import_job_t* job = (import_job_t*)list_get(g_import_queue, i);
            bool can_import_now = true;

            // Check dependencies if the importer supports it
            if (job->importer->does_depend_on)
            {
                // Check if any files this one depends on are still in the queue
                for (size_t j = 0; j < list_count(g_import_queue); j++)
                {
                    if (i == j)
                        continue; // Don't check against self

                    import_job_t* other_job = (import_job_t*)list_get(g_import_queue, j);
                    if (job->importer->does_depend_on(&job->source_path, &other_job->source_path))
                    {
                        can_import_now = false;
                        break;
                    }
                }
            }

            if (can_import_now)
            {
                // Import this file
                if (job->importer->import_func)
                    job->importer->import_func(&job->source_path, &output_path, g_config);
                made_progress = true;
            }
            else
            {
                // Keep this job for next iteration
                list_add(remaining_jobs, (Object*)job);
            }
        }

        // Swap lists
        list_t* temp = g_import_queue;
        g_import_queue = remaining_jobs;
        remaining_jobs = temp;
        list_clear(remaining_jobs);
    }

    // Clean up any remaining jobs (circular dependencies or errors)
    for (size_t i = 0; i < list_count(g_import_queue); i++)
    {
        import_job_t* job = (import_job_t*)list_get(g_import_queue, i);
        printf("WARNING: Could not import %s (possible circular dependency)\n", job->source_path.value);
    }

    list_clear(g_import_queue);
    Free(remaining_jobs);

    // Generate asset manifest after processing imports if enabled
    bool manifest_enabled = props_get_bool(g_config, "manifest.enabled", false);
    if (manifest_enabled)
    {
        const char* manifest_path = props_get_string(g_config, "manifest.output_file", "./src/assets.c");

        // Create absolute path for manifest
        path_t manifest_abs_path, manifest_rel_path;
        path_set(&manifest_rel_path, manifest_path);
        path_make_absolute(&manifest_abs_path, &manifest_rel_path);

        // Ensure the manifest directory exists
        path_t manifest_dir;
        path_dir(&manifest_abs_path, &manifest_dir);
        if (!directory_create_recursive(&manifest_dir))
        {
            printf("WARNING: Failed to create manifest directory: %s\n", manifest_dir.value);
        }

        if (!asset_manifest_generate(output_path.value, manifest_abs_path.value))
        {
            printf("WARNING: Failed to generate asset manifest\n");
        }
    }
}
