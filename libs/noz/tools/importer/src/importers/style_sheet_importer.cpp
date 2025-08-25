//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <noz/asset.h>
#include <noz/noz.h>
#include <noz/ui.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace fs = std::filesystem;

using namespace noz;

static PseudoState ParsePseudoState(const std::string& str)
{
    if (str == "selected:hover") return PSEUDO_STATE_HOVER | PSEUDO_STATE_SELECTED;
    if (str == "hover")    return PSEUDO_STATE_HOVER;
    if (str == "active")   return PSEUDO_STATE_ACTIVE;
    if (str == "selected") return PSEUDO_STATE_SELECTED;
    if (str == "disabled") return PSEUDO_STATE_DISABLED;
    if (str == "focused")  return PSEUDO_STATE_FOCUSED;
    if (str == "pressed")  return PSEUDO_STATE_PRESSED;
    if (str == "checked")  return PSEUDO_STATE_CHECKED;
    return PSEUDO_STATE_NONE;
}

static StyleColor ParseStyleColor(const std::string& value)
{
    Tokenizer tk = {};
    Token token = {};
    color color = color_transparent;
    InitTokenizer(tk, value.c_str());
    ExpectColor(tk, &token, &color);
    return StyleColor{ {STYLE_KEYWORD_OVERWRITE}, color };
}

static StyleLength ParseStyleLength(const std::string& value)
{
    if (value == "auto")
        return StyleLength { .parameter = { .keyword = STYLE_KEYWORD_OVERWRITE }, .unit = STYLE_LENGTH_UNIT_AUTO, .value = 0.0f };

    if (!value.empty() && value.back() == '%')
        return StyleLength{.parameter = {.keyword = STYLE_KEYWORD_OVERWRITE},
                           .unit = STYLE_LENGTH_UNIT_PERCENT,
                           .value = std::stof(value.substr(0, value.length() - 1)) / 100.0f};

    return StyleLength { .parameter = {.keyword = STYLE_KEYWORD_OVERWRITE}, .unit = STYLE_LENGTH_UNIT_FIXED, .value = std::stof(value) };
}

static StyleInt ParseStyleInt (const std::string& value)
{
    return StyleInt { .parameter = {.keyword = STYLE_KEYWORD_OVERWRITE}, .value = std::stoi(value) };
}

static StyleFlexDirection ParseStyleFlexDirection(const std::string& value)
{
    if (value == "row") return StyleFlexDirection{ STYLE_KEYWORD_OVERWRITE, FLEX_DIRECTION_ROW };
    if (value == "column") return StyleFlexDirection{ STYLE_KEYWORD_OVERWRITE, FLEX_DIRECTION_COL };
    if (value == "row-reverse") return StyleFlexDirection{ STYLE_KEYWORD_OVERWRITE, FLEX_DIRECTION_ROW_REVERSE };
    if (value == "column-reverse") return StyleFlexDirection{ STYLE_KEYWORD_OVERWRITE, FLEX_DIRECTION_COL_REVERSE };
    return StyleFlexDirection{ STYLE_KEYWORD_INHERIT, FLEX_DIRECTION_ROW };
}

static void WriteStyleSheetData(
    Stream* stream,
    const std::unordered_map<std::string, Style>& styles)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_STYLESHEET;
    header.runtime_size = styles.size() * 256 + 512; // Rough estimate
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);

    // Write number of styles
    WriteU32(stream, static_cast<uint32_t>(styles.size()));

    for (const auto& [class_name, style_data] : styles)
    {
        WriteString(stream, class_name.c_str());
        SerializeStyle(&style_data, stream);
    }
}

static bool ParseParameter(const std::string& group, const std::string& key, Props* meta, Style& style)
{
    auto value = meta->GetString(group.c_str(),key.c_str(), nullptr);
    if (value.empty())
        return false;

    if (key == "width")
        style.width = ParseStyleLength(value);
    else if (key == "height")
        style.height = ParseStyleLength(value);
    else if (key == "background-color")
        style.background_color = ParseStyleColor(value);
    else if (key == "color")
        style.color = ParseStyleColor(value);
    else if (key == "font-size")
        style.font_size = ParseStyleInt(value);
    else if (key == "margin")
        style.margin_top = style.margin_left = style.margin_right = style.margin_bottom = ParseStyleLength(value);
    else if (key == "margin-top")
        style.margin_top = ParseStyleLength(value);
    else if (key == "margin-left")
        style.margin_left = ParseStyleLength(value);
    else if (key == "margin-bottom")
        style.margin_bottom = ParseStyleLength(value);
    else if (key == "margin-right")
        style.margin_right = ParseStyleLength(value);
    else if (key == "padding")
        style.padding_top = style.padding_left = style.padding_right = style.padding_bottom = ParseStyleLength(value);
    else if (key == "padding-top")
        style.padding_top = ParseStyleLength(value);
    else if (key == "padding-left")
        style.padding_left = ParseStyleLength(value);
    else if (key == "padding-bottom")
        style.padding_bottom = ParseStyleLength(value);
    else if (key == "padding-right")
        style.padding_right = ParseStyleLength(value);
    else if (key == "flex-direction")
        style.flex_direction = ParseStyleFlexDirection(value);

    return true;
}

static void ParseStyles(const std::string& path, Props* meta, std::unordered_map<std::string, Style>& styles)
{
    // Inherit styles first
    auto inheritedFiles = meta->GetKeys("inherit");
    for (const auto& inheritedPath : inheritedFiles)
    {
        // TODO: Need to load Props for inherited files
        // This would require loading Props from the inherited file path
        (void)inheritedPath;
    }

    auto groups = meta->GetGroups();

    // Parse each group/section
    for (auto& group_name : groups)
    {
        if (group_name == "inherit")
            continue;

        Style style {};

        auto keys = meta->GetKeys(group_name.c_str());
        for (const auto& key_name : keys)
            ParseParameter(group_name, key_name, meta, style);

        // Merge or add the style
        auto it = styles.find(group_name);
        if (it != styles.end())
            MergeStyles(&it->second, &style);
        else
            styles[group_name] = style;
    }

    // Handle pseudo states (like "Button:hover")
    for (const auto& [full_class_name, style_obj] : styles)
    {
        auto colon_pos = full_class_name.find(':');
        if (colon_pos == std::string::npos)
            continue;

        auto class_name = full_class_name.substr(0, colon_pos);
        auto state_name = full_class_name.substr(colon_pos + 1);
        auto state = ParsePseudoState(state_name);

        if (state == PSEUDO_STATE_NONE)
            throw std::runtime_error("Invalid pseudo state in style '" + full_class_name + "'");

        auto base_it = styles.find(class_name);
        if (base_it == styles.end())
            continue;

        Style resolved_style = base_it->second; // Start with base
        MergeStyles(&resolved_style, const_cast<Style*>(&style_obj));
        styles[full_class_name] = resolved_style;
    }
}

static std::unordered_map<std::string, Style> ParseStyles(const std::string& path, Props* meta_props)
{
    std::unordered_map<std::string, Style> styles;
    ParseStyles(path, meta_props, styles);
    return styles;
}
void ImportStyleSheet(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta_props)
{
    fs::path src_path = source_path;
    
    // Parse styles from source file  
    auto styles = ParseStyles(src_path.string(), meta_props);
    
    // Write stylesheet data using Stream API
    WriteStyleSheetData(output_stream, styles);
}

bool DoesStyleSheetDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    // Check if dependency is the meta file for this stylesheet
    fs::path meta_path = fs::path(source_path.string() + ".meta");
    
    if (meta_path == dependency_path)
        return true;
    
    // TODO: Add support for stylesheet inheritance dependency checking
    // Need to load meta_props to check inherited files
    
    return false;
}

static const char* g_stylesheet_extensions[] = {
    ".styles",
    nullptr
};

static AssetImporterTraits g_stylesheet_importer_traits = {
    .type_name = "StyleSheet",
    .type = TYPE_STYLE_SHEET,
    .signature = ASSET_SIGNATURE_STYLESHEET,
    .file_extensions = g_stylesheet_extensions,
    .import_func = ImportStyleSheet,
    .does_depend_on = DoesStyleSheetDependOn
};

AssetImporterTraits* GetStyleSheetImporterTraits()
{
    return &g_stylesheet_importer_traits;
}
