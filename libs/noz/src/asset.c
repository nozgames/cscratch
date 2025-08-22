//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

const char* asset_path(const char* name, const char* ext)
{
    static path_t path_buffer;
    const char* base_path = SDL_GetBasePath();
    
    if (!base_path) {
        path_set(&path_buffer, "assets");
    } else {
        path_set(&path_buffer, base_path);
        path_append(&path_buffer, "assets");
    }
    
    // Append the name and extension
    path_append(&path_buffer, name);
    
    // Add extension if provided
    if (ext && *ext) {
        size_t len = path_buffer.length;
        if (len + strlen(ext) + 2 < sizeof(path_buffer.data)) {
            path_buffer.data[len] = '.';
            strcpy(path_buffer.data + len + 1, ext);
            path_buffer.length = len + 1 + strlen(ext);
        }
    }
    
    return path_buffer.data;
}
