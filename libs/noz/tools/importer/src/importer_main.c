
#include <noz/noz.h>
#include <noz/file_watcher.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

asset_importer_traits_t* shader_importer_create();

typedef struct import_job
{
	path_t source_path;
	asset_importer_traits_t* importer;
} import_job_t;

static array_t* g_import_queue = NULL;
static props_t* g_config = NULL;

void process_file_change(const path_t* file_path, file_change_type_t change_type, asset_importer_traits_t** importers);
void process_import_queue(asset_importer_traits_t** importers);

static volatile bool g_running = true;

void signal_handler(int sig)
{
	if (sig == SIGINT)
	{
		printf("\nShutting down...\n");
		g_running = false;
	}
}

const char* change_type_to_string(file_change_type_t type)
{
	switch (type)
	{
		case file_change_type_added: return "ADDED";
		case file_change_type_modified: return "MODIFIED";
		case file_change_type_deleted: return "DELETED";
		default: return "UNKNOWN";
	}
}

int main(int argc, char* argv[])
{
	// Initialize importers array
	asset_importer_traits_t* importers[] = {
		shader_importer_create(),
		NULL
	};

	// Set up signal handler for Ctrl-C
	signal(SIGINT, signal_handler);

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
		object_free(g_config);
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
		object_free(g_config);
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
	object_free(g_config);

	// Clean up import queue
	if (g_import_queue)
		array_free(g_import_queue);

	return 0;
}

void process_file_change(const path_t* file_path, file_change_type_t change_type, asset_importer_traits_t** importers)
{
	if (change_type == file_change_type_deleted)
		return; // Don't process deleted files
	
	// Debug: show what file was detected
	printf("File changed: %s\n", file_path->value);

	if (!g_import_queue)
	{
		g_import_queue = array_create(sizeof(import_job_t), 16);
		if (!g_import_queue)
		{
			printf("Failed to create import queue\n");
			return;
		}
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
			printf("  -> Can import with shader importer\n");
			break;
		}
	}

	if (selected_importer)
	{
		// Check if this file is already in the queue
		bool already_queued = false;
		for (size_t i = 0; i < array_length(g_import_queue); i++)
		{
			import_job_t* job = (import_job_t*)array_get(g_import_queue, i);
			if (path_eq(&job->source_path, &source_path))
			{
				already_queued = true;
				break;
			}
		}

		if (!already_queued)
		{
			import_job_t* new_job = (import_job_t*)array_push(g_import_queue);
			if (new_job)
			{
				new_job->source_path = source_path;
				new_job->importer = selected_importer;
				// Silent - will print when actually imported
			}
		}
	}
}

void process_import_queue(asset_importer_traits_t** importers)
{
	if (!g_import_queue || array_is_empty(g_import_queue))
		return;

	// Get output directory from config
	const char* output_dir = props_get_string(g_config, "output.directory", "assets");
	
	// Ensure output directory exists
#ifdef _WIN32
	if (!CreateDirectoryA(output_dir, NULL)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			printf("Failed to create directory: %s\n", output_dir);
		}
	}
#else
	mkdir(output_dir, 0755);
#endif

	path_t output_path;
	path_set(&output_path, output_dir);
	array_t* remaining_jobs = array_create(sizeof(import_job_t), array_length(g_import_queue));
	bool made_progress = true;

	// Keep processing until no more progress is made
	while (made_progress && !array_is_empty(g_import_queue))
	{
		made_progress = false;
		array_clear(remaining_jobs);

		for (size_t i = 0; i < array_length(g_import_queue); i++)
		{
			import_job_t* job = (import_job_t*)array_get(g_import_queue, i);
			bool can_import_now = true;

			// Check dependencies if the importer supports it
			if (job->importer->does_depend_on)
			{
				// Check if any files this one depends on are still in the queue
				for (size_t j = 0; j < array_length(g_import_queue); j++)
				{
					if (i == j) continue; // Don't check against self

					import_job_t* other_job = (import_job_t*)array_get(g_import_queue, j);
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
				import_job_t* remaining_job = (import_job_t*)array_push(remaining_jobs);
				if (remaining_job)
				{
					*remaining_job = *job;
				}
			}
		}

		// Swap arrays
		array_t* temp = g_import_queue;
		g_import_queue = remaining_jobs;
		remaining_jobs = temp;
		array_clear(remaining_jobs);
	}

	// Clean up any remaining jobs (circular dependencies or errors)
	for (size_t i = 0; i < array_length(g_import_queue); i++)
	{
		import_job_t* job = (import_job_t*)array_get(g_import_queue, i);
		printf("WARNING: Could not import %s (possible circular dependency)\n", job->source_path.value);
	}

	array_clear(g_import_queue);
	array_free(remaining_jobs);
}
