//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @style

typedef enum pseudo_state
{
    pseudo_state_none = 0,
    pseudo_state_hover = 1 << 0,
    pseudo_state_active = 1 << 1,
    pseudo_state_selected = 1 << 2,
    pseudo_state_disabled = 1 << 3,
    pseudo_state_focused = 1 << 4,
    pseudo_state_pressed = 1 << 5,
    pseudo_state_checked = 1 << 6
} pseudo_state_t;

typedef enum style_keyword
{
    style_keyword_inherit,
    style_keyword_overwrite,
    style_keyword_inline
} style_keyword_t;

typedef enum flex_direction
{
    flex_direction_row,
    flex_direction_col,
    flex_direction_row_reverse,
    flex_direction_col_reverse
} flex_direction_t;

typedef enum style_length_unit
{
    style_length_unit_fixed,
    style_length_unit_percent,
    style_length_unit_auto
} style_length_unit_t;

typedef struct style_parameter
{
    style_keyword_t keyword;
} style_parameter_t;

typedef struct style_length
{
	style_parameter_t parameter;
    style_length_unit_t unit;
    float value;
} style_length_t;

typedef struct style_color
{
    style_parameter_t parameter;
    color_t value;
} style_color_t;

typedef struct style_float
{
    style_parameter_t parameter;
    float value;
} style_float_t;

typedef struct style_int
{
    style_parameter_t parameter;
    int value;
} style_int_t;

typedef struct style_bool
{
    style_parameter_t parameter;
    bool value;
} style_bool_t;

typedef struct style_flex_direction
{
    style_parameter_t parameter;
    flex_direction_t value;
} style_flex_direction_t;

struct style_t
{
    style_flex_direction_t flex_direction;
    style_length_t width;
    style_length_t height;
    style_color_t background_color;
    style_color_t color;
    style_int_t font_size;
    style_length_t margin_top;
    style_length_t margin_left;
    style_length_t margin_bottom;
    style_length_t margin_right;
    style_length_t padding_top;
    style_length_t padding_left;
    style_length_t padding_bottom;
    style_length_t padding_right;
};


void style_deserialize_into(Stream* stream, style_t* style);
style_t style_deserialize(Stream* stream);
void style_serialize(style_t* style, Stream* stream);
void style_merge(style_t* dst, style_t* src);
