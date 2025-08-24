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
 

static bool style_deserialize_parameter(stream_t* stream, style_parameter_t* value)
{
    value->keyword = (style_keyword_t)stream_read_uint8(stream);
    return value->keyword == style_keyword_overwrite;
}

#if 0 // not used yet
static void style_deserialize_bool(stream_t* stream, style_bool_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;

    value->value = stream_read_bool(stream);
}
static void style_deserialize_float(stream_t* stream, style_float_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = stream_read_float(stream);
}
#endif

static void style_deserialize_int(stream_t* stream, style_int_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = stream_read_int32(stream);
}

static void style_deserialize_color(stream_t* stream, style_color_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = stream_read_color(stream);
}

static void style_deserialize_flex_direction(stream_t* stream, style_flex_direction_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;
    value->value = (flex_direction_t)stream_read_uint8(stream);
}

static void style_deserialize_length(stream_t* stream, style_length_t* value)
{
    if (!style_deserialize_parameter(stream, (style_parameter_t*)value))
        return;

    value->unit = (style_length_unit_t)stream_read_uint8(stream);
    value->value = stream_read_float(stream);
}

void style_deserialize_into(stream_t* stream, style_t* style)
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

style_t style_deserialize(stream_t* stream)
{
    style_t style = {};
    style_deserialize_into(stream, &style);
    return style;
}

static bool style_serialize_parameter(stream_t* stream, style_parameter_t* value)
{
    stream_write_uint8(stream, value->keyword);
    return value->keyword == style_keyword_overwrite;
}

static void style_serialize_int(stream_t* stream, const style_int_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    stream_write_int32(stream, value->value);
}

#if 0

static void style_serialize_bool(stream_t* stream, const style_bool_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    stream_write_bool(stream, value->value);
}

static void style_serialize_float(stream_t* stream, const style_float_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    stream_write_float(stream, value->value);
}

#endif

static void style_serialize_color(stream_t* stream, const style_color_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    stream_write_color(stream, value->value);
}

static void style_serialize_flex_direction(stream_t* stream, const style_flex_direction_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;
    stream_write_uint8(stream, (uint8_t)value->value);
}

static void style_serialize_length(stream_t* stream, const style_length_t* value)
{
    if (!style_serialize_parameter(stream, (style_parameter_t*)value))
        return;

    stream_write_uint8(stream, (uint8_t)value->unit);
    stream_write_float(stream, value->value);
}

void style_serialize(const style_t* style, stream_t* stream)
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
#define STYLE_MERGE(n) if ((int)src->##n.parameter.keyword >= (int)dst->n.parameter.keyword) dst->n = src->##n
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