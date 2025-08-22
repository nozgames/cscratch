//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

#include <SDL3/SDL.h>
#include "uthash.h"
#include <noz/noz.h>

// @renderer
void renderer_init(const renderer_traits* traits, SDL_Window* window);

// @mesh
typedef struct mesh_vertex
{
    vec3_t position;
    vec2_t uv0;
    vec3_t normal;
    float bone;
} mesh_vertex;