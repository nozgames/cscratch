//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "internal.h"
#include "noz/stream.h"
#include <string.h>

bool ReadAssetHeader(Stream* stream, asset_header_t* header)
{
    if (!stream || !header) return false;
    
    // Read header fields
    header->signature = ReadU32(stream);
    header->runtime_size = ReadU32(stream);
    header->version = ReadU32(stream);
    header->flags = ReadU32(stream);
    
    return !IsEOS(stream);
}

bool WriteAssetHeader(Stream* stream, asset_header_t* header)
{
    if (!stream || !header) return false;
    
    // Write header fields
    WriteU32(stream, header->signature);
    WriteU32(stream, header->runtime_size);
    WriteU32(stream, header->version);
    WriteU32(stream, header->flags);
    
    return true;
}

bool ValidateAssetHeader(asset_header_t* header, uint32_t expected_signature)
{
    if (!header) return false;
    return header->signature == expected_signature;
}

const char* asset_signature_to_string(uint32_t signature)
{
    static char sig_str[5];
    sig_str[0] = (signature >> 24) & 0xFF;
    sig_str[1] = (signature >> 16) & 0xFF;
    sig_str[2] = (signature >> 8) & 0xFF;
    sig_str[3] = signature & 0xFF;
    sig_str[4] = '\0';
    return sig_str;
}

type_t ToType(uint32_t signature)
{
    switch (signature) {
        case NOZ_TEXTURE_SIG:  return type_texture;
        case NOZ_MESH_SIG:     return type_mesh;
        case NOZ_SOUND_SIG:    return type_sound;
        case NOZ_SHADER_SIG:   return type_shader;
        case NOZ_MATERIAL_SIG: return type_material;
        case NOZ_FONT_SIG:     return type_font;
        default:               return type_unknown;
    }
}

const char* ToString(type_t type)
{
    switch (type) {
        case type_texture:  return "texture";
        case type_mesh:     return "mesh";
        case type_sound:    return "sound";
        case type_shader:   return "shader";
        case type_material: return "material";
        case type_font:     return "font";
        default:            return NULL;
    }
}

void SetAssetPath(Path* dst, const name_t* name, const char* ext)
{
    assert(dst);
    assert(name);
    assert(ext);

    const char* base_path = SDL_GetBasePath();
    
    if (!base_path)
    {
        path_set(dst, "assets");
    } 
    else 
    {
        path_set(dst, base_path);
        path_append(dst, "assets");
    }
    
    // Append the name and extension
    path_append(dst, name->value);
    path_set_extension(dst, ext);
}
