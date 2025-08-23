//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

// Macros for disabling external library warnings
#ifdef _MSC_VER
    #define NOZ_WARNINGS_DISABLE() \
        __pragma(warning(push)) \
        __pragma(warning(disable: 4820)) \
        __pragma(warning(disable: 4255)) \
        __pragma(warning(disable: 4668)) \
        __pragma(warning(disable: 4710)) \
        __pragma(warning(disable: 4711)) \
        __pragma(warning(disable: 4514)) \
        __pragma(warning(disable: 4061)) \
        __pragma(warning(disable: 4062)) \
        __pragma(warning(disable: 4371)) \
        __pragma(warning(disable: 4191)) \
        __pragma(warning(disable: 4100))
    
    #define NOZ_WARNINGS_ENABLE() __pragma(warning(pop))
#else
    #define NOZ_WARNINGS_ENABLE()
    #define NOZ_WARNINGS_DISABLE()
#endif

#include "types.h"
#include "allocator.h"
#include "gmath.h"
#include "object.h"
#include "map.h"
#include "list.h"
#include "string.h"
#include "hash.h"
#include "color.h"
#include "stream.h"
#include "tokenizer.h"
#include "props.h"
#include "asset.h"
#include "application.h"
#include "renderer.h"
#include "scene.h"
#include "platform.h"