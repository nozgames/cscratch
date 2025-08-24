//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>


typedef u32 asset_signature_t;

constexpr asset_signature_t ASSET_SIGNATURE_TEXTURE    = 0x4E5A5458;  // 'NZTX'
constexpr asset_signature_t ASSET_SIGNATURE_MESH       = 0x4E5A4D53;  // 'NZMS'
constexpr asset_signature_t ASSET_SIGNATURE_SOUND      = 0x4E5A534E;  // 'NZSN'
constexpr asset_signature_t ASSET_SIGNATURE_SHADER     = 0x4E5A5348;  // 'NZSH'
constexpr asset_signature_t ASSET_SIGNATURE_MATERIAL   = 0x4E5A4D54;  // 'NZMT'
constexpr asset_signature_t ASSET_SIGNATURE_FONT       = 0x4E5A4654;  // 'NZFT'
constexpr asset_signature_t ASSET_SIGNATURE_STYLESHEET = 0x4E5A5354;  // 'NZST'

struct AssetHeader
{
    asset_signature_t signature;
    uint32_t runtime_size;
    uint32_t version;
    uint32_t flags;
} ;

bool ReadAssetHeader(Stream* stream, AssetHeader* header);
bool WriteAssetHeader(Stream* stream, AssetHeader* header);
bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
type_t ToType(asset_signature_t signature);
void SetAssetPath(Path* dst, const name_t* name, const char* ext);
Stream* LoadAssetStream(Allocator* allocator, const char* asset_name);

// Asset loading macro - creates a name_t and calls the appropriate load function
// Usage: NOZ_ASSET_LOAD(shader, "shaders/border_effect", g_assets.shaders.border_effect);
#define NOZ_ASSET_LOAD(type, path, member) \
    do { \
        name_t asset_name; \
        name_set(&asset_name, path); \
        member = Load##type##(g_asset_allocator, &asset_name); \
    } while(0)