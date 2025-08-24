//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef enum file_change_type
{
    file_change_type_added,
    file_change_type_modified,
    file_change_type_deleted
} file_change_type_t;

// File change event
typedef struct file_change_event
{
    Path path;
    file_change_type_t type;
    uint64_t timestamp;
} file_change_event_t;

void file_watcher_init(int poll_interval_ms);
void file_watcher_shutdown(void);
bool file_watcher_add_directory(const char* directory);
bool file_watcher_remove_directory(const char* directory);
bool file_watcher_start(void);
void file_watcher_stop(void);
bool file_watcher_poll(file_change_event_t* event);
size_t file_watcher_pending_count(void);
void file_watcher_clear_queue(void);
bool file_watcher_is_running(void);
