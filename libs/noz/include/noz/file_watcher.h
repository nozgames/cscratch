//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <noz/string.h>

// Type of file system change
typedef enum file_change_type
{
    file_change_type_added,
    file_change_type_modified,
    file_change_type_deleted
} file_change_type_t;

// File change event
typedef struct file_change_event
{
    path_t path;               // Full path to the file
    file_change_type_t type;   // Type of change
    uint64_t timestamp;        // When the change was detected
} file_change_event_t;

// Initialize the file watcher system
// poll_interval_ms: How often to check for changes (milliseconds)
void file_watcher_init(int poll_interval_ms);

// Shutdown the file watcher system
void file_watcher_shutdown(void);

// Add a directory to watch (recursive)
// Returns true if successfully added
bool file_watcher_add_directory(const char* directory);

// Remove a directory from watching
bool file_watcher_remove_directory(const char* directory);

// Start watching (spawns thread)
bool file_watcher_start(void);

// Stop watching (stops thread)
void file_watcher_stop(void);

// Poll for file changes (non-blocking)
// Returns true if an event was retrieved, false if queue is empty
bool file_watcher_poll(file_change_event_t* event);

// Get the number of pending events in the queue
size_t file_watcher_pending_count(void);

// Clear all pending events
void file_watcher_clear_queue(void);

// Check if the watcher is currently running
bool file_watcher_is_running(void);