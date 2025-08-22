//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _WIN32

#include <noz/noz.h>
#include <windows.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

void thread_sleep_ms(int milliseconds)
{
    Sleep(milliseconds);
}

bool platform_get_file_stat(const char* file_path, file_stat_t* stat)
{
    struct _stat st;
    if (_stat(file_path, &st) != 0)
        return false;
    
    stat->size = st.st_size;
    stat->mtime = st.st_mtime;
    stat->is_directory = (st.st_mode & _S_IFDIR) != 0;
    stat->is_regular_file = (st.st_mode & _S_IFREG) != 0;
    
    return true;
}

bool platform_enum_files(const char* dir_path, platform_file_callback_t callback, void* user_data)
{
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    char search_path[1024];
    char full_path[1024];
    
    // Create search pattern
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);
    
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE)
        return false;
    
    do
    {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;
        
        // Create full path
        snprintf(full_path, sizeof(full_path), "%s\\%s", dir_path, find_data.cFileName);
        
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
        
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return true;
}

#endif