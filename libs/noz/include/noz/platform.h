//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
typedef struct file_stat 
{
    size_t size;             // File size in bytes
    time_t modified_time;    // Last modification time
    bool is_directory;       // True if this is a directory
    bool is_regular_file;    // True if this is a regular file
} file_stat_t;

// @callback
typedef void (*directory_enum_files_callback_t)(Path* file_path, file_stat_t* stat, void* user_data);

// @file
bool file_stat(Path* file_path, file_stat_t* stat);
bool file_exists(Path* file_path);
bool file_delete(Path* file_path);
bool file_copy(Path* source, Path* dest);
bool file_move(Path* source, Path* dest);

// @directory
bool directory_create(Path* dir_path);
bool directory_create_recursive(Path* dir_path);
bool directory_exists(Path* dir_path);
bool directory_delete(Path* dir_path);
bool directory_delete_recursive(Path* dir_path);
bool directory_enum_files(Path* dir_path, directory_enum_files_callback_t callback, void* user_data);

// @path
bool path_current_directory(Path* dst);
bool path_executable_directory(Path* dst);
bool path_user_directory(Path* dst);
bool path_temp_directory(Path* dst);

// @thread
void thread_sleep_ms(int milliseconds);
