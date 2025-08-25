//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace glm;

#include "noz_math.h"

#define VEC3_FORWARD vec3(0, 0, 1)
#define VEC3_BACKWARD vec3(0, 0, -1)
#define VEC3_UP vec3(0, 1, 0)
#define VEC3_DOWN vec3(0, -1, 0)
#define VEC3_RIGHT vec3(1, 0, 0)
#define VEC3_LEFT vec3(-1, 0, 0)
#define VEC3_ZERO vec3(0,0,0)
#define VEC3_ONE vec3(1,1,1)
#define VEC4_ZERO vec4(0,0,0,0)
#define VEC4_ONE vec4(1,1,1,1)
#define VEC2_ZERO vec2(0,0)
#define VEC2_ONE vec2(1,1)

inline int i32_max(i32 a, i32 b) { return (a > b) ? a : b; }
inline int i32_min(i32 a, i32 b) { return (a < b) ? a : b; }

#include "allocator.h"
#include "bounds3.h"
#include "color.h"
#include "string.h"
#include "hash.h"
#include "map.h"
#include "object.h"
#include "list.h"
#include "stream.h"
#include "asset.h"
#include "platform.h"
#include "application.h"
#include "scene.h"
#include "types.h"
#include "ui.h"
