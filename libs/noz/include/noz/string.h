//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @name
typedef struct name
{
    char value[64];
    size_t length;
} name_t;

name_t* name_set(name_t* dst, const char* src);
name_t* SetName(name_t* dst, const name_t* src);
name_t* name_format(name_t* dst, const char* fmt, ...);
bool name_empty(const name_t* name);
bool name_eq(name_t* a, name_t* b);
bool name_eq_cstr(const name_t* name, const char* str);

// @path
typedef struct path
{
    char value[1024];
    size_t length;
} Path;

Path* path_set(Path* dst, const char* src);
Path* path_copy(Path* dst, Path* src);
bool path_eq(Path* a, Path* b);
Path* path_clear(Path* path);
Path* path_append(Path* dst, const char* component);
Path* path_join(Path* dst, const char* base, const char* component);
Path* path_dir(Path* src, Path* dst);
void path_filename(Path* src, name_t* dst);
void path_filename_without_extension(Path* src, name_t* dst);
const char* path_basename(Path* path);
const char* path_extension(Path* path);
bool path_has_extension(Path* path, const char* ext);
bool path_cstr_has_extension(char* path, const char* ext);
Path* path_set_extension(Path* path, const char* ext);
Path* path_normalize(Path* path);
bool path_is_absolute(Path* path);
bool path_is_empty(Path* path);
Path* path_make_relative(Path* dst, Path* path, Path* base);
Path* path_make_absolute(Path* dst, Path* path);
bool path_is_under(Path* path, Path* base);
bool path_find_relative_to_bases(Path* dst, Path* path, const char** bases, size_t base_count);

// @text
typedef struct text
{
    char value[128];
    size_t length;
} text_t;

text_t* text_init(text_t* text);
text_t* text_set(text_t* dst, const char* src);
text_t* text_copy(text_t* dst, text_t* src);
text_t* text_append(text_t* dst, const char* src);
text_t* text_format(text_t* dst, const char* fmt, ...);
size_t text_length(text_t* text);
text_t* text_clear(text_t* text);
text_t* text_trim(text_t* text);
bool text_equals(text_t* a, text_t* b);
bool text_equals_cstr(text_t* text, const char* str);

// Utility macros for compatibility
#define STRING_DATA(s) ((s)->value)
#define STRING_LENGTH(s) ((s)->length)