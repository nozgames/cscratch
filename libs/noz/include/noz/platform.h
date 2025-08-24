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
typedef void (*directory_enum_files_callback_t)(path_t* file_path, file_stat_t* stat, void* user_data);

// @file
bool file_stat(path_t* file_path, file_stat_t* stat);
bool file_exists(path_t* file_path);
bool file_delete(path_t* file_path);
bool file_copy(path_t* source, path_t* dest);
bool file_move(path_t* source, path_t* dest);

// @directory
bool directory_create(path_t* dir_path);
bool directory_create_recursive(path_t* dir_path);
bool directory_exists(path_t* dir_path);
bool directory_delete(path_t* dir_path);
bool directory_delete_recursive(path_t* dir_path);
bool directory_enum_files(path_t* dir_path, directory_enum_files_callback_t callback, void* user_data);

// @path
bool path_current_directory(path_t* dst);
bool path_executable_directory(path_t* dst);
bool path_user_directory(path_t* dst);
bool path_temp_directory(path_t* dst);

// @thread
void thread_sleep_ms(int milliseconds);
