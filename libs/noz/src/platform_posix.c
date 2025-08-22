//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifndef _WIN32

#include <noz/noz.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

void thread_sleep_ms(int milliseconds)
{
    usleep(milliseconds * 1000);
}

bool platform_get_file_stat(const char* file_path, file_stat_t* stat)
{
    struct stat st;
    if (stat(file_path, &st) != 0)
        return false;
    
    stat->size = st.st_size;
    stat->mtime = st.st_mtime;
    stat->is_directory = S_ISDIR(st.st_mode);
    stat->is_regular_file = S_ISREG(st.st_mode);
    
    return true;
}

bool platform_enum_files(const char* dir_path, platform_file_callback_t callback, void* user_data)
{
    DIR* dir = opendir(dir_path);
    if (!dir)
        return false;
    
    struct dirent* entry;
    char full_path[1024];
    
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        // Create full path
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        // Get file stats using our cross-platform function
        file_stat_t stat;
        if (platform_get_file_stat(full_path, &stat))
        {
            // Call the callback
            callback(full_path, &stat, user_data);
            
            // If it's a directory, recurse
            if (stat.is_directory)
            {
                platform_enum_files(full_path, callback, user_data);
            }
        }
    }
    
    closedir(dir);
    return true;
}

#endif