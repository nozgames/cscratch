//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/ui.h>

static style_t g_default_style = {
    .flex_direction = { style_keyword_inherit, flex_direction_row },
    .width = { style_keyword_inherit, style_length_unit_auto, 0.0f },
    .height = { style_keyword_inherit, style_length_unit_auto, 0.0f },
    .background_color = { style_keyword_inherit, {0,0,0,0} },
    .color = { style_keyword_inherit, {255,255,255,255} },
    .font_size = { style_keyword_inherit, 16 },
    .margin_top = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .margin_left = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .margin_bottom = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .margin_right = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .padding_top = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .padding_left = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .padding_bottom = { style_keyword_inherit, style_length_unit_fixed, 0.0f },
    .padding_right = { style_keyword_inherit, style_length_unit_fixed, 0.0f }
};
 

static bool style_deserialize_parameter(Stream* stream, style_parameter_t* value)
{
    value->keyword = (style_keyword_t)ReadU8(stream);
    return value->keyword == style_keyword_overwrite;
}

#if 0 // not used yet
static void style_deserialize_bool(Stream* stream, style_bool_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;

    value->value = ReadBool(stream);
}
static void style_deserialize_float(Stream* stream, style_float_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = ReadFloat(stream);
}
#endif

static void style_deserialize_int(Stream* stream, style_int_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = ReadI32(stream);
}

static void style_deserialize_color(Stream* stream, style_color_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = ReadColor(stream);
}

static void style_deserialize_flex_direction(Stream* stream, style_flex_direction_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = (flex_direction_t)ReadU8(stream);
}

static void style_deserialize_length(Stream* stream, style_length_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;

    value->unit = (style_length_unit_t)ReadU8(stream);
    value->value = ReadFloat(stream);
}

void style_deserialize_into(Stream* stream, style_t* style)
{
    style_deserialize_flex_direction(stream, &style->flex_direction);
    style_deserialize_length(stream, &style->width);
    style_deserialize_length(stream, &style->height);
    style_deserialize_color(stream, &style->background_color);
    style_deserialize_color(stream, &style->color);
    style_deserialize_int(stream, &style->font_size);
    style_deserialize_length(stream, &style->margin_top);
    style_deserialize_length(stream, &style->margin_left);
    style_deserialize_length(stream, &style->margin_bottom);
    style_deserialize_length(stream, &style->margin_right);
    style_deserialize_length(stream, &style->padding_top);
    style_deserialize_length(stream, &style->padding_left);
    style_deserialize_length(stream, &style->padding_bottom);
    style_deserialize_length(stream, &style->padding_right);
}

style_t style_deserialize(Stream* stream)
{
    style_t style = {};
    style_deserialize_into(stream, &style);
    return style;
}

static bool style_serialize_parameter(Stream* stream, style_parameter_t* value)
{
    WriteU8(stream, value->keyword);
    return value->keyword == style_keyword_overwrite;
}

static void style_serialize_int(Stream* stream, const style_int_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    WriteI32(stream, value->value);
}

#if 0

static void style_serialize_bool(Stream* stream, const style_bool_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    WriteBool(stream, value->value);
}

static void style_serialize_float(Stream* stream, const style_float_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    WriteFloat(stream, value->value);
}

#endif

static void style_serialize_color(Stream* stream, const style_color_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    WriteColor(stream, value->value);
}

static void style_serialize_flex_direction(Stream* stream, const style_flex_direction_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    WriteU8(stream, (uint8_t)value->value);
}

static void style_serialize_length(Stream* stream, const style_length_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;

    WriteU8(stream, (uint8_t)value->unit);
    WriteFloat(stream, value->value);
}

void style_serialize(const style_t* style, Stream* stream)
{
    style_serialize_flex_direction(stream, &style->flex_direction);
    style_serialize_length(stream, &style->width);
    style_serialize_length(stream, &style->height);
    style_serialize_color(stream, &style->background_color);
    style_serialize_color(stream, &style->color);
    style_serialize_int(stream, &style->font_size);
    style_serialize_length(stream, &style->margin_top);
    style_serialize_length(stream, &style->margin_left);
    style_serialize_length(stream, &style->margin_bottom);
    style_serialize_length(stream, &style->margin_right);
    style_serialize_length(stream, &style->padding_top);
    style_serialize_length(stream, &style->padding_left);
    style_serialize_length(stream, &style->padding_bottom);
    style_serialize_length(stream, &style->padding_right);
}

void style_merge(style_t* dst, const style_t* src)
{
#define STYLE_MERGE(n) if (src->n.parameter.keyword >= dst->n.parameter.keyword) dst->n = src->n
    STYLE_MERGE(flex_direction);
    STYLE_MERGE(color);
    STYLE_MERGE(background_color);
    STYLE_MERGE(width);
    STYLE_MERGE(height);
    STYLE_MERGE(font_size);
    STYLE_MERGE(margin_top);
    STYLE_MERGE(margin_left);
    STYLE_MERGE(margin_bottom);
    STYLE_MERGE(margin_right);
    STYLE_MERGE(padding_top);
    STYLE_MERGE(padding_left);
    STYLE_MERGE(padding_bottom);
    STYLE_MERGE(padding_right);
#undef STYLE_MERGE
}