#pragma once

#include "object.h"

// @renderer_traits
typedef struct renderer_traits
{
    uint32_t max_textures;
    uint32_t shadow_map_size;

} renderer_traits;

// @renderer

// @texture
typedef struct texture* texture_t;
object_type_t texture_type();
texture_t texture_create(int width, int height);
int texture_width(texture_t texture);
