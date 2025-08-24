//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _WIN32

#include <noz/noz.h>
#include <noz/platform.h>
#include <windows.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

void thread_sleep_ms(int milliseconds)
{
    Sleep(milliseconds);
}

bool file_stat(const path_t* file_path, file_stat_t* stat)
{
    struct _stat st;
    if (_stat(file_path->value, &st) != 0)
        return false;
    
    stat->size = st.st_size;
    stat->modified_time = st.st_mtime;
    stat->is_directory = (st.st_mode & _S_IFDIR) != 0;
    stat->is_regular_file = (st.st_mode & _S_IFREG) != 0;
    
    return true;
}

bool directory_create(const path_t* dir_path)
{
    // Try to create the directory
    if (_mkdir(dir_path->value) == 0)
        return true;
    
    // If it failed because directory already exists, that's success
    if (errno == EEXIST)
    {
        // Verify it's actually a directory
        return directory_exists(dir_path);
    }
    
    return false;
}

bool directory_create_recursive(const path_t* dir_path)
{
    path_t parent;
    path_dir(dir_path, &parent);
    
    // If parent is not the same as current (not root), create parent first
    if (!path_eq(&parent, dir_path))
    {
        if (!directory_exists(&parent))
        {
            if (!directory_create_recursive(&parent))
                return false;
        }
    }
    
    return directory_create(dir_path);
}

bool directory_exists(const path_t* dir_path)
{
    file_stat_t stat;
    return file_stat(dir_path, &stat) && stat.is_directory;
}

bool path_current_directory(path_t* dst)
{
    if (!dst)
        return false;
    
    char buffer[1024];
    if (_getcwd(buffer, sizeof(buffer)) == NULL)
        return false;
    
    path_set(dst, buffer);
    return true;
}

bool directory_enum_files(const path_t* dir_path, directory_enum_files_callback_t callback, void* user_data)
{
    WIN32_FIND_DATAA find_data;
    HANDLE find_handle;
    path_t search_path;
    path_t full_path;
    
    // Create search pattern
    path_copy(&search_path, dir_path);
    path_append(&search_path, "*");
    
    find_handle = FindFirstFileA(search_path.value, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE)
        return false;
    
    do
    {
        // Skip . and ..
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;
        
        // Create full path
        path_copy(&full_path, dir_path);
        path_append(&full_path, find_data.cFileName);
        
        // Get file stats
        file_stat_t stat;
        if (file_stat(&full_path, &stat))
        {
            // Call the callback
            callback(&full_path, &stat, user_data);
            
            // If it's a directory, recurse
            if (stat.is_directory)
            {
                directory_enum_files(&full_path, callback, user_data);
            }
        }
        
    } while (FindNextFileA(find_handle, &find_data));
    
    FindClose(find_handle);
    return true;
}

NOZ_WARNINGS_DISABLE()

extern int main(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;

    char* args[1] = { ".exe" };
    return main(1, args);
}

NOZ_WARNINGS_ENABLE()

#endif
