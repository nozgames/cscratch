
#include <noz/noz.h>
#include <noz/file_watcher.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

asset_importer_traits_t* shader_importer_create();

typedef struct import_job
{
	path_t source_path;
	asset_importer_traits_t* importer;
} import_job_t;

static array_t* g_import_queue = NULL;
static props_t g_config = NULL;

void process_file_change(const char* file_path, file_change_type_t change_type);
void process_import_queue();

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
	asset_importer_traits_t* importers[] = {
		shader_importer_create(),
		NULL
	};

	// Set up signal handler for Ctrl-C
	signal(SIGINT, signal_handler);

	// Load importer configuration
	g_config = props_create();
	if (!g_config)
	{
		printf("Failed to create props object\n");
		return 1;
	}

	printf("Loading importer configuration from ./importer.cfg\n");
	if (!props_load_from_file(g_config, "./importer.cfg"))
	{
		printf("Failed to load ./importer.cfg (file may not exist)\n");
		props_destroy(g_config);
		return 1;
	}

	printf("Configuration loaded successfully!\n");

	// Initialize file watcher
	file_watcher_init(500); // Check every 500ms

	// Get source directories from config
	if (!props_has_key(g_config, "source"))
	{
		printf("No [source] section found in config\n");
		props_destroy(g_config);
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
		props_destroy(g_config);
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
			printf("[%s] %s\n", 
				change_type_to_string(event.type), 
				event.path.data);
			
			// Process file changes for import
			process_file_change(event.path.data, event.type);
		}
		
		// Process any queued imports
		process_import_queue();
		
		// Sleep briefly to avoid busy waiting
		thread_sleep_ms(100); // 100ms
	}

	// Clean up
	file_watcher_stop();
	file_watcher_shutdown();
	props_destroy(g_config);

	// Clean up import queue
	if (g_import_queue)
		array_destroy(g_import_queue);

	return 0;
}

void process_file_change(const char* file_path, file_change_type_t change_type)
{
	if (change_type == file_change_type_deleted)
		return; // Don't process deleted files

	if (!g_import_queue)
	{
		g_import_queue = array_create(sizeof(import_job_t), 16);
		if (!g_import_queue)
		{
			printf("Failed to create import queue\n");
			return;
		}
	}

	// Get importers array
	asset_importer_traits_t* importers[] = {
		shader_importer_create(),
		NULL
	};

	// Find an importer that can handle this file
	path_t source_path;
	path_set(&source_path, file_path);
	asset_importer_traits_t* selected_importer = NULL;

	for (int i = 0; importers[i] != NULL; i++)
	{
		if (importers[i]->can_import && importers[i]->can_import(&source_path))
		{
			selected_importer = importers[i];
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
			if (strcmp(job->source_path.data, source_path.data) == 0)
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
				printf("Queued for import: %s\n", file_path);
			}
		}
	}
}

void process_import_queue()
{
	if (!g_import_queue || array_is_empty(g_import_queue))
		return;

	// Get output directory from config
	const char* output_dir = "assets"; // Default
	if (g_config && props_has_key(g_config, "output_dir"))
	{
		output_dir = props_get_string(g_config, "output_dir", "assets");
	}

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
				printf("Importing: %s\n", job->source_path.data);
				if (job->importer->import_func)
				{
					job->importer->import_func(&job->source_path, &output_path);
				}
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
		printf("WARNING: Could not import %s (possible circular dependency)\n", job->source_path.data);
	}

	array_clear(g_import_queue);
	array_destroy(remaining_jobs);
}
