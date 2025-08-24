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
name_t* name_copy(name_t* dst, name_t* src);
name_t* name_format(name_t* dst, const char* fmt, ...);
bool name_empty(name_t* name);
bool name_eq(name_t* a, name_t* b);
bool name_eq_cstr(name_t* name, const char* str);

// @path
typedef struct path
{
    char value[1024];
    size_t length;
} path_t;

path_t* path_set(path_t* dst, const char* src);
path_t* path_copy(path_t* dst, path_t* src);
bool path_eq(path_t* a, path_t* b);
path_t* path_clear(path_t* path);
path_t* path_append(path_t* dst, const char* component);
path_t* path_join(path_t* dst, const char* base, const char* component);
path_t* path_dir(path_t* src, path_t* dst);
void path_filename(path_t* src, name_t* dst);
void path_filename_without_extension(path_t* src, name_t* dst);
const char* path_basename(path_t* path);
const char* path_extension(path_t* path);
bool path_has_extension(path_t* path, const char* ext);
bool path_cstr_has_extension(char* path, const char* ext);
path_t* path_set_extension(path_t* path, const char* ext);
path_t* path_normalize(path_t* path);
bool path_is_absolute(path_t* path);
bool path_is_empty(path_t* path);
path_t* path_make_relative(path_t* dst, path_t* path, path_t* base);
path_t* path_make_absolute(path_t* dst, path_t* path);
bool path_is_under(path_t* path, path_t* base);
bool path_find_relative_to_bases(path_t* dst, path_t* path, const char** bases, size_t base_count);

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