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
#include "map.h"
#include "array.h"
#include "string.h"
#include "hash.h"
#include "color.h"
#include "stream.h"
#include "object.h"
#include "tokenizer.h"
#include "props.h"
#include "asset.h"
#include "application.h"
#include "renderer.h"
#include "scene.h"

// Cross-platform file info structure
typedef struct file_stat
{
    uint64_t size;          // File size in bytes
    uint64_t mtime;         // Last modification time (timestamp)
    bool is_directory;      // True if this is a directory
    bool is_regular_file;   // True if this is a regular file
} file_stat_t;

// Platform utilities
void thread_sleep_ms(int milliseconds);

// Cross-platform file operations
bool platform_get_file_stat(const char* file_path, file_stat_t* stat);

// File enumeration callback
typedef void (*platform_file_callback_t)(const char* file_path, const file_stat_t* stat, void* user_data);

// Enumerate files in directory recursively, calling callback for each file/directory found
bool platform_enum_files(const char* dir_path, platform_file_callback_t callback, void* user_data);
