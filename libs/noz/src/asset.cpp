//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

bool ReadAssetHeader(Stream* stream, AssetHeader* header)
{
    if (!stream || !header) return false;
    
    // Read header fields
    header->signature = ReadU32(stream);
    header->runtime_size = ReadU32(stream);
    header->version = ReadU32(stream);
    header->flags = ReadU32(stream);
    
    return !IsEOS(stream);
}

bool WriteAssetHeader(Stream* stream, AssetHeader* header)
{
    if (!stream || !header) return false;
    
    // Write header fields
    WriteU32(stream, header->signature);
    WriteU32(stream, header->runtime_size);
    WriteU32(stream, header->version);
    WriteU32(stream, header->flags);
    
    return true;
}

bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature)
{
    if (!header) return false;
    return header->signature == expected_signature;
}

type_t ToType(asset_signature_t signature)
{
    switch (signature)
    {
        case ASSET_SIGNATURE_TEXTURE:  return TYPE_TEXTURE;
        case ASSET_SIGNATURE_MESH:     return TYPE_MESH;
        case ASSET_SIGNATURE_SOUND:    return TYPE_SOUND;
        case ASSET_SIGNATURE_SHADER:   return TYPE_SHADER;
        case ASSET_SIGNATURE_MATERIAL: return TYPE_MATERIAL;
        case ASSET_SIGNATURE_FONT:     return TYPE_FONT;
        default:                       return TYPE_UNKNOWN;
    }
}

const char* GetExtensionFromSignature(asset_signature_t signature)
{
    // Convert signature to 4 character string (little endian to big endian)
    static char ext[6];  // ".xxxx\0"
    ext[0] = '.';
    ext[1] = tolower((signature >> 24) & 0xFF);
    ext[2] = tolower((signature >> 16) & 0xFF);
    ext[3] = tolower((signature >> 8) & 0xFF);
    ext[4] = tolower(signature & 0xFF);
    ext[5] = '\0';
    
    return ext;
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

Stream* LoadAssetStream(Allocator* allocator, const char* asset_name, asset_signature_t signature)
{
    assert(asset_name);

    const char* base_path = SDL_GetBasePath();
    std::filesystem::path asset_path;
    
    if (!base_path)
    {
        asset_path = "assets";
    }
    else
    {
        asset_path = base_path;
        asset_path /= "assets";
    }
    
    asset_path /= asset_name;
    asset_path += GetExtensionFromSignature(signature);

    return LoadStream(allocator, asset_path);
}

Object* LoadAsset(Allocator* allocator, const char* asset_name, asset_signature_t signature, AssetLoaderFunc loader)
{
    if (!asset_name || !loader)
        return nullptr;

    Stream* stream = LoadAssetStream(allocator, asset_name, signature);
    if (!stream)
        return nullptr;
    
    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature))
    {
        Destroy(stream);
        return nullptr;
    }

    auto asset = loader(allocator, stream, &header, asset_name);
    Destroy(stream);
    
    return asset;
}

