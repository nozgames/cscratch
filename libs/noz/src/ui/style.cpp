//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/ui.h>

static Style g_default_style = {
    .flex_direction = { STYLE_KEYWORD_INHERIT, FLEX_DIRECTION_ROW },
    .width = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_AUTO, 0.0f },
    .height = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_AUTO, 0.0f },
    .background_color = { STYLE_KEYWORD_INHERIT, {0,0,0,0} },
    .color = { STYLE_KEYWORD_INHERIT, {255,255,255,255} },
    .font_size = { STYLE_KEYWORD_INHERIT, 16 },
    .margin_top = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_left = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_bottom = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_right = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_top = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_left = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_bottom = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_right = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f }
};

const Style& GetDefaultStyle()
{
    return g_default_style;
}

static bool DeserializeStyleParameter(Stream* stream, StyleParameter* value)
{
    value->keyword = (StyleKeyword)ReadU8(stream);
    return value->keyword == STYLE_KEYWORD_OVERWRITE;
}

#if 0 // not used yet
static void style_deserialize_bool(Stream* stream, StyleBool* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;

    value->value = ReadBool(stream);
}
static void style_deserialize_float(Stream* stream, StyleFloat* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;
    value->value = ReadFloat(stream);
}
#endif

static void DeserializeParameter(Stream* stream, StyleInt* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;
    value->value = ReadI32(stream);
}

static void DeserializeParameter(Stream* stream, StyleColor* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;
    value->value = ReadColor(stream);
}

static void DeserializeParameter(Stream* stream, StyleFlexDirection* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;
    value->value = (FlexDirection)ReadU8(stream);
}

static void DeserializeParameter(Stream* stream, StyleLength* value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter*)value))
        return;

    value->unit = (StyleLengthUnit)ReadU8(stream);
    value->value = ReadFloat(stream);
}

void DeserializeStyle(Stream* stream, Style* style)
{
    DeserializeParameter(stream, &style->flex_direction);
    DeserializeParameter(stream, &style->width);
    DeserializeParameter(stream, &style->height);
    DeserializeParameter(stream, &style->background_color);
    DeserializeParameter(stream, &style->color);
    DeserializeParameter(stream, &style->font_size);
    DeserializeParameter(stream, &style->margin_top);
    DeserializeParameter(stream, &style->margin_left);
    DeserializeParameter(stream, &style->margin_bottom);
    DeserializeParameter(stream, &style->margin_right);
    DeserializeParameter(stream, &style->padding_top);
    DeserializeParameter(stream, &style->padding_left);
    DeserializeParameter(stream, &style->padding_bottom);
    DeserializeParameter(stream, &style->padding_right);
}

Style DeserializeStyle(Stream* stream)
{
    Style style = {};
    DeserializeStyle(stream, &style);
    return style;
}

static bool SerializeParameter(Stream* stream, StyleParameter* value)
{
    WriteU8(stream, value->keyword);
    return value->keyword == STYLE_KEYWORD_OVERWRITE;
}

static void SerializeParameter(Stream* stream, const StyleInt* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;
    WriteI32(stream, value->value);
}

#if 0

static void style_serialize_bool(Stream* stream, const StyleBool* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;
    WriteBool(stream, value->value);
}

static void style_serialize_float(Stream* stream, const StyleFloat* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;
    WriteFloat(stream, value->value);
}

#endif

static void SerializeParameter(Stream* stream, const StyleColor* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;
    WriteColor(stream, value->value);
}

static void SerializeParameter(Stream* stream, const StyleFlexDirection* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;
    WriteU8(stream, (uint8_t)value->value);
}

static void SerializeParameter(Stream* stream, const StyleLength* value)
{
    if (!SerializeParameter(stream, (StyleParameter*)value))
        return;

    WriteU8(stream, (uint8_t)value->unit);
    WriteFloat(stream, value->value);
}

void SerializeStyle(const Style* style, Stream* stream)
{
    SerializeParameter(stream, &style->flex_direction);
    SerializeParameter(stream, &style->width);
    SerializeParameter(stream, &style->height);
    SerializeParameter(stream, &style->background_color);
    SerializeParameter(stream, &style->color);
    SerializeParameter(stream, &style->font_size);
    SerializeParameter(stream, &style->margin_top);
    SerializeParameter(stream, &style->margin_left);
    SerializeParameter(stream, &style->margin_bottom);
    SerializeParameter(stream, &style->margin_right);
    SerializeParameter(stream, &style->padding_top);
    SerializeParameter(stream, &style->padding_left);
    SerializeParameter(stream, &style->padding_bottom);
    SerializeParameter(stream, &style->padding_right);
}

void MergeStyles(Style* dst, const Style* src)
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