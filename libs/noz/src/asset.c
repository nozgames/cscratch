//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

const char* asset_path(const char* name, const char* ext)
{
    static char path_buffer[4096];
    const char* base_path = SDL_GetBasePath();
    if (!base_path)
        snprintf(path_buffer, sizeof(path_buffer), "assets/%s.%s", name, ext);
    else
        snprintf(path_buffer, sizeof(path_buffer), "%sassets/%s.%s", base_path, name, ext);
    
    return path_buffer;
}
