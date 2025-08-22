//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <stddef.h>
#include <stdbool.h>

// name_t - Immutable asset names/mini-paths (64 chars)
// Used for: shader names, texture names, material names, etc.
// Example: "shaders/foo/bar/boo"
typedef struct name
{
    char data[64];
    size_t length;
} name_t;

// Set a name from a C string
name_t* name_set(name_t* dst, const char* src);

// Format a name using printf-style formatting
name_t* name_format(name_t* dst, const char* fmt, ...);

// path_t - File system paths (1024 chars)
// Used for: temporary file operations, asset loading
typedef struct path
{
    char data[1024];
    size_t length;
} path_t;

// Path operations
path_t* path_set(path_t* dst, const char* src);
path_t* path_copy(path_t* dst, const path_t* src);
path_t* path_append(path_t* dst, const char* component);
path_t* path_join(path_t* dst, const char* base, const char* component);
const char* path_dirname(const path_t* path);
const char* path_basename(const path_t* path);
const char* path_extension(const path_t* path);
bool path_has_extension(const path_t* path, const char* ext);
bool path_cstr_has_extension(const char* path, const char* ext);
path_t* path_normalize(path_t* path);
bool path_is_absolute(const path_t* path);

// text_t - General text/UI labels (128 chars)
// Used for: UI text, labels, short messages
typedef struct text
{
    char data[128];
    size_t length;
} text_t;

// Text operations
text_t* text_init(text_t* text);
text_t* text_set(text_t* dst, const char* src);
text_t* text_copy(text_t* dst, const text_t* src);
text_t* text_append(text_t* dst, const char* src);
text_t* text_format(text_t* dst, const char* fmt, ...);
size_t text_length(const text_t* text);
text_t* text_clear(text_t* text);
bool text_equals(const text_t* a, const text_t* b);
bool text_equals_cstr(const text_t* text, const char* str);

// Utility macros for compatibility
#define STRING_DATA(s) ((s)->data)
#define STRING_LENGTH(s) ((s)->length)