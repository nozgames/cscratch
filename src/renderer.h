#pragma once

#include "object.h"

typedef struct texture* texture_t;

extern object_type_t object_type_texture;

void renderer_init();
void renderer_uninit();

// @texture
object_type_t texture_type();
texture_t texture_create(int width, int height);
int texture_width(texture_t texture);
