//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <filesystem>


typedef u32 asset_signature_t;

constexpr asset_signature_t ASSET_SIGNATURE_TEXTURE     = 0x4E5A5458;  // 'NZTX'
constexpr asset_signature_t ASSET_SIGNATURE_MESH        = 0x4E5A4D53;  // 'NZMS'
constexpr asset_signature_t ASSET_SIGNATURE_SOUND       = 0x4E5A534E;  // 'NZSN'
constexpr asset_signature_t ASSET_SIGNATURE_SHADER      = 0x4E5A5348;  // 'NZSH'
constexpr asset_signature_t ASSET_SIGNATURE_MATERIAL    = 0x4E5A4D54;  // 'NZMT'
constexpr asset_signature_t ASSET_SIGNATURE_FONT        = 0x4E5A4654;  // 'NZFT'
constexpr asset_signature_t ASSET_SIGNATURE_STYLE_SHEET = 0x4E5A5354;  // 'NZST'

struct AssetHeader
{
    asset_signature_t signature;
    uint32_t version;
    uint32_t flags;
} ;

typedef Object* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);

bool ReadAssetHeader(Stream* stream, AssetHeader* header);
bool WriteAssetHeader(Stream* stream, AssetHeader* header);
bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
type_t ToType(asset_signature_t signature);
const char* GetExtensionFromSignature(asset_signature_t signature);
void SetAssetPath(Path* dst, const name_t* name, const char* ext);
Object* LoadAsset(Allocator* allocator, const char* asset_name, asset_signature_t signature, AssetLoaderFunc loader);


// @loaders
Object* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);
Object* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);
Object* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);
Object* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);
Object* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name);

// @macros
#define NOZ_LOAD_SHADER(path, member) \
    member = (Shader*)LoadAsset(g_asset_allocator, path, ASSET_SIGNATURE_SHADER, LoadShader);

#define NOZ_LOAD_TEXTURE(path, member) \
    member = (Texture*)LoadAsset(g_asset_allocator, path, ASSET_SIGNATURE_TEXTURE, LoadTexture);

#define NOZ_LOAD_STYLE_SHEET(path, member) \
    member = (StyleSheet*)LoadAsset(g_asset_allocator, path, ASSET_SIGNATURE_STYLE_SHEET, LoadStyleSheet);

#define NOZ_LOAD_MESH(path, member) \
    member = (Mesh*)LoadAsset(g_asset_allocator, path, ASSET_SIGNATURE_MESH, LoadMesh);

#define NOZ_LOAD_FONT(path, member) \
    member = (Font*)LoadAsset(g_asset_allocator, path, ASSET_SIGNATURE_FONT, LoadFont);