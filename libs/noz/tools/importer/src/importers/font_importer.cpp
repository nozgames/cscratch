
//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <noz/asset.h>
#include <noz/noz.h>
#include <noz/noz_math.h>
#include <rect_packer.h>
#include <ttf/TrueTypeFont.h>
#include <msdf/msdf.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace noz;

struct FontGlyph
{
    const ttf::TrueTypeFont::Glyph* ttf;
    ivec2 size;
    dvec2 scale;
    ivec2 packedSize;
    ivec2 advance;
    rect_packer::BinRect packedRect;
    ivec2 bearing;
    char ascii;
};

struct FontKerning
{
    uint32_t first;
    uint32_t second;
    float amount;
};

struct FontMetrics
{
    float ascent;
    float descent;
    float lineHeight;
    float baseline;
};

static void WriteFontData(
    Stream* stream,
    const ttf::TrueTypeFont* ttf,
    const std::vector<unsigned char>& atlasData,
    const glm::ivec2& atlasSize,
    const std::vector<FontGlyph>& glyphs,
    int fontSize)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_FONT;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);

    // Write font size (this is important for runtime scaling)
    WriteU32(stream, static_cast<uint32_t>(fontSize));

    // Write atlas dimensions
    WriteU32(stream, static_cast<uint32_t>(atlasSize.x));
    WriteU32(stream, static_cast<uint32_t>(atlasSize.y));

    // Write font metrics
    WriteFloat(stream, float(ttf->ascent()) / fontSize);
    WriteFloat(stream, float(ttf->descent()) / fontSize);
    WriteFloat(stream, float(ttf->height()) / fontSize);
    WriteFloat(stream, float(ttf->ascent()) / fontSize);

    // Write glyph count and glyph data
    WriteU16(stream, static_cast<uint16_t>(glyphs.size()));
    for (const auto& glyph : glyphs)
    {
        WriteU32(stream, glyph.ascii);
        WriteFloat(stream, glyph.packedRect.x / float(atlasSize.x));
        WriteFloat(stream, glyph.packedRect.y / float(atlasSize.y));
        WriteFloat(stream, (glyph.packedRect.x + glyph.packedRect.w) / float(atlasSize.x));
        WriteFloat(stream, (glyph.packedRect.y + glyph.packedRect.h) / float(atlasSize.y));
        WriteFloat(stream, float(glyph.size.x) / fontSize);
        WriteFloat(stream, float(glyph.size.y) / fontSize);
        WriteFloat(stream, float(glyph.advance.x) / fontSize);
        WriteFloat(stream, 0.0f);
        WriteFloat(stream, float(glyph.bearing.x) / fontSize);
        WriteFloat(stream, float(-glyph.bearing.y) / fontSize);
        WriteFloat(stream, 0.0f);
    }

    // Write kerning count and kerning data
    WriteU16(stream, static_cast<uint16_t>(ttf->kerning().size()));
    for (const auto& k : ttf->kerning())
    {
        WriteU32(stream, k.left);
        WriteU32(stream, k.right);
        WriteFloat(stream, k.value);
    }

    WriteBytes(stream, (void*)atlasData.data(), atlasData.size());
}
void ImportFont(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    fs::path src_path = source_path;

    // Parse font properties from meta props (with defaults)
    int fontSize = meta->GetInt("font", "size", 48);
    std::string characters = meta->GetString("font", "characters", " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    int sdfPadding = meta->GetInt("font", "sdfPadding", 8);
    int padding = meta->GetInt("font", "padding", 1);

    // Load font file
    std::ifstream file(src_path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open font file");
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read font data
    std::vector<unsigned char> fontData(fileSize);
    file.read(reinterpret_cast<char*>(fontData.data()), fileSize);
    file.close();

    // Create stream for font loading
    Stream* stream = LoadStream(nullptr, fontData.data(), fontData.size());
    auto ttf = std::shared_ptr<ttf::TrueTypeFont>(ttf::TrueTypeFont::load(stream, fontSize, characters));

    // Build the imported glyph list
    std::vector<FontGlyph> glyphs;
    for (size_t i = 0; i < characters.size(); i++)
    {
        auto ttfGlyph = ttf->glyph(characters[i]);
        if (ttfGlyph == nullptr)
            continue;

        FontGlyph iglyph{};
        iglyph.ascii = characters[i];
        iglyph.ttf = ttfGlyph;

        iglyph.size = noz::RoundToNearest(ttfGlyph->size + glm::dvec2(sdfPadding * 2));
        iglyph.scale = glm::dvec2(iglyph.size.x, iglyph.size.y) / ttfGlyph->size;
        iglyph.packedSize = iglyph.size + (padding + sdfPadding) * 2;
        iglyph.bearing = noz::RoundToNearest(ttfGlyph->bearing);
        iglyph.advance.x = noz::RoundToNearest((float)ttfGlyph->advance);

        glyphs.push_back(iglyph);
    }

    // Pack the glyphs
    int minHeight = (int)noz::NextPowerOf2((uint32_t)(fontSize + 2 + sdfPadding * 2 + padding * 2));
    rect_packer packer(minHeight, minHeight);

    while (packer.empty())
    {
        for(auto& glyph : glyphs)
        {
            if (glyph.ttf->contours.size() == 0)
                continue;

            if (-1 == packer.Insert(glyph.packedSize, rect_packer::method::BestLongSideFit, glyph.packedRect))
            {
                rect_packer::BinSize size = packer.size();
                if (size.w <= size.h)
                    size.w <<= 1;
                else
                    size.h <<= 1;

                packer.Resize(size.w, size.h);
                break;
            }
        }
    }

    if (!packer.validate())
    {
        throw std::runtime_error("RectPacker validation failed");
    }

    auto imageSize = glm::ivec2(packer.size().w, packer.size().h);
    std::vector<uint8_t> image;
    image.resize(imageSize.x * imageSize.y, 0);

    for (size_t i = 0; i < glyphs.size(); i++)
    {
        auto glyph = glyphs[i];
        if (glyph.ttf->contours.size() == 0)
            continue;

        msdf::renderGlyph(
            glyph.ttf,
            image,
            imageSize.x,
            glm::ivec2(
                glyph.packedRect.x + padding,
                glyph.packedRect.y + padding
            ),
            glm::ivec2(
                glyph.packedRect.w - padding * 2,
                glyph.packedRect.h - padding * 2
            ),
            sdfPadding,
            glyph.scale,
            glm::dvec2(
                -glyph.ttf->bearing.x + sdfPadding,
                (glyph.ttf->size.y - glyph.ttf->bearing.y) + sdfPadding
            )
        );
    }

    WriteFontData(output_stream, ttf.get(), image, imageSize, glyphs, fontSize);
}

bool DoesFontDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // Check if dependency is the meta file for this font
    fs::path meta_path = fs::path(source_path.string() + ".meta");

    return meta_path == dependency_path;
}

static const char* g_font_extensions[] = {
    ".ttf",
    ".otf",
    nullptr
};

static AssetImporterTraits g_font_importer_traits = {
    .type_name = "Font",
    .type = TYPE_FONT,
    .signature = ASSET_SIGNATURE_FONT,
    .file_extensions = g_font_extensions,
    .import_func = ImportFont,
    .does_depend_on = DoesFontDependOn
};

AssetImporterTraits* GetFontImporterTraits()
{
    return &g_font_importer_traits;
}
