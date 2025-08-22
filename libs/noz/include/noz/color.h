//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct color32 color32_t;
typedef struct color24 color24_t;
typedef struct color color_t;

// 32-bit RGBA color (0-255 per component)
typedef struct color32
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} color32_t;

// 24-bit RGB color (0-255 per component)
typedef struct color24
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color24_t;

// Floating point RGBA color (0.0-1.0 per component)
typedef struct color
{
    float r;
    float g;
    float b;
    float a;
} color_t;

// Color32 functions
color32_t color32_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color32_t color32_from_color(const color_t* color);
color32_t color32_from_color24(const color24_t* color, uint8_t alpha);
bool color32_is_transparent(const color32_t* color);
bool color32_is_opaque(const color32_t* color);
bool color32_equals(const color32_t* a, const color32_t* b);

// Color24 functions
color24_t color24_create(uint8_t r, uint8_t g, uint8_t b);
color24_t color24_from_color(const color_t* color);
color24_t color24_from_color32(const color32_t* color);
bool color24_equals(const color24_t* a, const color24_t* b);

// Color functions
color_t color_create(float r, float g, float b, float a);
color_t color_from_color32(const color32_t* color);
color_t color_from_color24(const color24_t* color, float alpha);
bool color_is_transparent(const color_t* color);
bool color_is_opaque(const color_t* color);
color_t color_clamped(const color_t* color);
bool color_equals(const color_t* a, const color_t* b);
color_t color_add(const color_t* a, const color_t* b);
color_t color_subtract(const color_t* a, const color_t* b);
color_t color_multiply_scalar(const color_t* color, float scalar);
color_t color_multiply(const color_t* a, const color_t* b);
color_t color_lerp(const color_t* a, const color_t* b, float t);

// Predefined colors
extern const color32_t color32_black;
extern const color32_t color32_white;
extern const color32_t color32_red;
extern const color32_t color32_green;
extern const color32_t color32_blue;
extern const color32_t color32_transparent;

extern const color24_t color24_black;
extern const color24_t color24_white;
extern const color24_t color24_red;
extern const color24_t color24_green;
extern const color24_t color24_blue;

extern const color_t color_black;
extern const color_t color_white;
extern const color_t color_red;
extern const color_t color_green;
extern const color_t color_blue;
extern const color_t color_transparent;