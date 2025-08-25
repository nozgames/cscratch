//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct StyleSheetImpl
{
    OBJECT_BASE;
    Map styles;
};

static StyleSheetImpl* Impl(StyleSheet* s) { return (StyleSheetImpl*)Cast(s, TYPE_STYLE_SHEET); }

Object* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const char* name)
{
    auto style_count = ReadU32(stream);
    auto keys_size = style_count * sizeof(u64);
    auto data_size = style_count * sizeof(Style);
    auto style_data_size = keys_size + data_size;

    auto* sheet = (StyleSheet*)CreateObject(allocator, sizeof(StyleSheetImpl) + style_data_size, TYPE_STYLE_SHEET);
    if (!sheet)
        return nullptr;

    auto impl = Impl(sheet);
    ReadBytes(stream, impl + 1, style_data_size);

    auto keys = (u64*)(impl + 1);
    auto styles = (Style*)(keys + style_count);

    impl->styles = CreateMap((u64*)(impl + 1), style_count, styles, sizeof(Style), style_count);

    return sheet;
}

const Style& GetStyle(StyleSheet* sheet, const name_t* name)
{
    auto impl = Impl(sheet);
    auto style = (Style*)GetValue(impl->styles, Hash(name));
    if (!style)
        return GetDefaultStyle();

    return *style;
}

bool HasStyle(StyleSheet* sheet, const name_t* name)
{
    auto impl = Impl(sheet);
    return HasKey(impl->styles, Hash(name));
}
