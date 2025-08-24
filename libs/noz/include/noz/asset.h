//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


// Asset file signatures - all start with 'NZ'
#define NOZ_TEXTURE_SIG  0x4E5A5458  // 'NZTX'
#define NOZ_MESH_SIG     0x4E5A4D53  // 'NZMS'  
#define NOZ_SOUND_SIG    0x4E5A534E  // 'NZSN'
#define NOZ_SHADER_SIG   0x4E5A5348  // 'NZSH'
#define NOZ_MATERIAL_SIG 0x4E5A4D54  // 'NZMT'
#define NOZ_FONT_SIG     0x4E5A4654  // 'NZFT'

// Asset header structure - common to all asset files
typedef struct asset_header 
{
    uint32_t signature;      // 4-byte signature identifying asset type
    uint32_t runtime_size;   // Estimated runtime memory requirement
    uint32_t version;        // Asset format version
    uint32_t flags;          // Asset-specific flags
} asset_header_t;

bool ReadAssetHeader(Stream* stream, asset_header_t* header);
bool WriteAssetHeader(Stream* stream, asset_header_t* header);
bool ValidateAssetHeader(asset_header_t* header, uint32_t expected_signature);
const char* asset_signature_to_string(uint32_t signature);
type_t ToType(uint32_t signature);
const char* ToString(type_t type);
void SetAssetPath(Path* dst, const name_t* name, const char* ext);

// Asset loading macro - creates a name_t and calls the appropriate load function
// Usage: NOZ_ASSET_LOAD(shader, "shaders/border_effect", g_assets.shaders.border_effect);
#define NOZ_ASSET_LOAD(type, path, member) \
    do { \
        name_t asset_name; \
        name_set(&asset_name, path); \
        member = Load##type##(g_asset_allocator, &asset_name); \
    } while(0)