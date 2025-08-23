//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/string.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifndef nullptr
#define nullptr NULL
#endif

// Helper macro to safely copy strings with bounds checking
#define SAFE_STRING_COPY(dst, dst_size, src) do { \
    size_t src_len = strlen(src); \
    if (src_len >= dst_size) src_len = dst_size - 1; \
    memcpy(dst, src, src_len); \
    dst[src_len] = '\0'; \
    return src_len; \
} while(0)

static bool string_eq(const char* a, size_t a_len, const char* b, size_t b_len)
{
    if (a_len != b_len)
        return false;

    bool eq = true;
    for (size_t i = 0; eq && i < a_len; i++, a++, b++)
        eq = *a == *b;

    return eq;
}

// ============================================================================
// name_t implementation
// ============================================================================

name_t* name_set(name_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->value[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->value)) {
        src_len = sizeof(dst->value) - 1;
    }
    
    memcpy(dst->value, src, src_len);
    dst->value[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

bool name_empty(const name_t* name)
{
    assert(name);
    return name->length == 0;
}

name_t* name_copy(name_t* dst, const name_t* src)
{
    assert(dst);
    assert(src);

    memcpy(dst, src, sizeof(name_t));
    return dst;
}

name_t* name_format(name_t* dst, const char* fmt, ...)
{
    assert(dst);
    assert(fmt);
    
    va_list args;
    va_start(args, fmt);
    
    int written = vsnprintf(dst->value, sizeof(dst->value), fmt, args);
    
    va_end(args);
    
    if (written < 0) {
        dst->value[0] = '\0';
        dst->length = 0;
    } else if ((size_t)written >= sizeof(dst->value)) {
        dst->length = sizeof(dst->value) - 1;
        dst->value[dst->length] = '\0';
    } else {
        dst->length = written;
    }
    
    return dst;
}

bool name_eq(const name_t* a, const name_t* b)
{
    assert(a);
    assert(b);
	return string_eq(a->value, a->length, b->value, b->length);
}

bool name_eq_cstr(const name_t* a, const char* b)
{
    assert(a);
    assert(b);
    return string_eq(a->value, a->length, b, strlen(b));
}

// ============================================================================
// path_t implementation  
// ============================================================================

// Helper function to find the last separator in a path
static const char* path_find_last_separator(const char* value, size_t length)
{
    const char* last_sep = nullptr;
    for (size_t i = 0; i < length; i++) {
        if (value[i] == '/' || value[i] == '\\') {
            last_sep = &value[i];
        }
    }
    return last_sep;
}

// Helper function to find the last dot after the last separator
static const char* path_find_extension_dot(const char* value, size_t length)
{
    const char* last_dot = nullptr;
    const char* last_sep = nullptr;
    
    for (size_t i = 0; i < length; i++) {
        if (value[i] == '/' || value[i] == '\\') {
            last_sep = &value[i];
            last_dot = nullptr; // Reset dot search after separator
        } else if (value[i] == '.') {
            last_dot = &value[i];
        }
    }
    
    // Only return the dot if it's after the last separator and not at the start
    if (last_dot && last_dot != value && (!last_sep || last_dot > last_sep)) {
        return last_dot;
    }
    return nullptr;
}

// Helper for case-insensitive string comparison
static bool path_compare_extension(const char* ext1, const char* ext2)
{
    while (*ext1 && *ext2) {
        char c1 = *ext1;
        char c2 = *ext2;
        
        // Convert to lowercase for comparison
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return false;
        }
        
        ext1++;
        ext2++;
    }
    
    return *ext1 == '\0' && *ext2 == '\0';
}

path_t* path_set(path_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->value[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->value)) {
        src_len = sizeof(dst->value) - 1;
    }
    
    memcpy(dst->value, src, src_len);
    dst->value[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

path_t* path_copy(path_t* dst, const path_t* src)
{
    assert(dst);
    assert(src);
    
    if (dst != src) {
        memcpy(dst->value, src->value, src->length + 1);
        dst->length = src->length;
    }
    
    return dst;
}

bool path_eq(const path_t* a, const path_t* b)
{
    assert(a);
    assert(b);
    return string_eq(a->value, a->length, b->value, b->length);
}

path_t* path_clear(path_t* path)
{
    assert(path);
    path->value[0] = '\0';
    path->length = 0;
    return path;
}

path_t* path_append(path_t* dst, const char* component)
{
    assert(dst);
    
    if (!component || !*component) {
        return dst;
    }
    
    // Add separator if needed
    if (dst->length > 0 && dst->value[dst->length - 1] != '/' && dst->value[dst->length - 1] != '\\') {
        if (dst->length + 1 < sizeof(dst->value)) {
            dst->value[dst->length++] = '/';
            dst->value[dst->length] = '\0';
        }
    }
    
    // Append component
    size_t comp_len = strlen(component);
    size_t available = sizeof(dst->value) - dst->length - 1;
    
    if (comp_len > available) {
        comp_len = available;
    }
    
    if (comp_len > 0) {
        memcpy(dst->value + dst->length, component, comp_len);
        dst->length += comp_len;
        dst->value[dst->length] = '\0';
    }
    
    return dst;
}

path_t* path_join(path_t* dst, const char* base, const char* component)
{
    assert(dst);
    
    path_set(dst, base);
    return path_append(dst, component);
}

path_t* path_dir(const path_t* src, path_t* dst)
{
    assert(src);
    assert(dst);
    
    if (src->length == 0) {
        dst->value[0] = '.';
        dst->value[1] = '\0';
        dst->length = 1;
        return dst;
    }
    
    const char* last_sep = path_find_last_separator(src->value, src->length);
    
    if (!last_sep) {
        dst->value[0] = '.';
        dst->value[1] = '\0';
        dst->length = 1;
        return dst;
    }
    
    size_t dir_len = last_sep - src->value;
    if (dir_len == 0) {
        dst->value[0] = '/';
        dst->value[1] = '\0';
        dst->length = 1;
    } else {
        if (dir_len >= sizeof(dst->value)) {
            dir_len = sizeof(dst->value) - 1;
        }
        memcpy(dst->value, src->value, dir_len);
        dst->value[dir_len] = '\0';
        dst->length = dir_len;
    }
    
    return dst;
}


const char* path_basename(const path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return "";
    }
    
    const char* last_sep = path_find_last_separator(path->value, path->length);
    
    if (!last_sep) {
        return path->value;
    }
    
    return last_sep + 1;
}

void path_filename(const path_t* src, name_t* dst)
{
    // todo: implement
}

void path_filename_without_extension(const path_t* src, name_t* dst)
{
    // todo: implement
}

const char* path_extension(const path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return "";
    }
    
    const char* last_dot = path_find_extension_dot(path->value, path->length);
    
    if (!last_dot) {
        return "";
    }
    
    return last_dot + 1;
}

bool path_has_extension(const path_t* path, const char* ext)
{
    assert(path);
    
    if (!ext || !*ext) {
        return false;
    }
    
    const char* path_ext = path_extension(path);
    if (!path_ext || !*path_ext) {
        return false;
    }
    
    return path_compare_extension(path_ext, ext);
}

bool path_cstr_has_extension(const char* path_str, const char* ext)
{
    if (!path_str || !ext) {
        return false;
    }
    
    size_t len = strlen(path_str);
    const char* last_dot = path_find_extension_dot(path_str, len);
    
    if (!last_dot) {
        return false;
    }
    
    return path_compare_extension(last_dot + 1, ext);
}

path_t* path_set_extension(path_t* path, const char* ext)
{
    assert(path);
    
    if (!ext) {
        return path;
    }
    
    const char* last_dot = path_find_extension_dot(path->value, path->length);
    
    // Determine where to start the new extension
    size_t ext_start = last_dot ? (last_dot - path->value) : path->length;
    
    // Calculate new length
    size_t ext_len = strlen(ext);
    size_t dot_len = (*ext == '.') ? 0 : 1;  // Add dot if ext doesn't start with one
    size_t new_len = ext_start + dot_len + ext_len;
    
    if (new_len >= sizeof(path->value)) {
        // Too long, truncate
        new_len = sizeof(path->value) - 1;
    }
    
    // Set the extension
    if (dot_len > 0) {
        path->value[ext_start] = '.';
    }
    
    size_t copy_len = new_len - ext_start - dot_len;
    if (copy_len > ext_len) {
        copy_len = ext_len;
    }
    
    memcpy(path->value + ext_start + dot_len, (*ext == '.') ? ext + 1 : ext, copy_len);
    path->value[new_len] = '\0';
    path->length = new_len;
    
    return path;
}

path_t* path_normalize(path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return path;
    }
    
    char temp[1024];
    size_t temp_len = 0;
    size_t i = 0;
    
    // Process each component
    while (i < path->length) {
        // Skip consecutive separators
        while (i < path->length && (path->value[i] == '/' || path->value[i] == '\\')) {
            if (temp_len == 0 || (temp_len > 0 && temp[temp_len - 1] != '/')) {
                temp[temp_len++] = '/';
            }
            i++;
        }
        
        // Get next component
        size_t comp_start = i;
        while (i < path->length && path->value[i] != '/' && path->value[i] != '\\') {
            i++;
        }
        
        if (comp_start < i) {
            size_t comp_len = i - comp_start;
            
            // Handle . and ..
            if (comp_len == 1 && path->value[comp_start] == '.') {
                // Skip current directory marker
                continue;
            } else if (comp_len == 2 && path->value[comp_start] == '.' && path->value[comp_start + 1] == '.') {
                // Go up one directory
                if (temp_len > 0) {
                    // Remove trailing slash
                    if (temp[temp_len - 1] == '/') {
                        temp_len--;
                    }
                    // Find previous slash
                    while (temp_len > 0 && temp[temp_len - 1] != '/') {
                        temp_len--;
                    }
                }
            } else {
                // Normal component - copy it
                memcpy(temp + temp_len, path->value + comp_start, comp_len);
                temp_len += comp_len;
            }
        }
    }
    
    // Remove trailing slash unless it's root
    if (temp_len > 1 && temp[temp_len - 1] == '/') {
        temp_len--;
    }
    
    // Copy back to path
    if (temp_len == 0) {
        path->value[0] = '.';
        path->value[1] = '\0';
        path->length = 1;
    } else {
        memcpy(path->value, temp, temp_len);
        path->value[temp_len] = '\0';
        path->length = temp_len;
    }
    
    return path;
}

bool path_is_absolute(const path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return false;
    }
    
    // Unix absolute path
    if (path->value[0] == '/') {
        return true;
    }
    
    // Windows absolute path (C:\ or C:/)
    if (path->length >= 3 && 
        ((path->value[0] >= 'A' && path->value[0] <= 'Z') || 
         (path->value[0] >= 'a' && path->value[0] <= 'z')) &&
        path->value[1] == ':' &&
        (path->value[2] == '/' || path->value[2] == '\\')) {
        return true;
    }
    
    return false;
}

bool path_is_empty(const path_t* path)
{
    assert(path);
    return path->length == 0 || path->value[0] == '\0';
}

path_t* path_make_relative(path_t* dst, const path_t* path, const path_t* base)
{
    assert(dst);
    assert(path);
    assert(base);
    
    // If base is empty, just copy the path
    if (base->length == 0) {
        return path_copy(dst, path);
    }
    
    // Check if path starts with base
    size_t base_len = base->length;
    if (strncmp(path->value, base->value, base_len) == 0) {
        // Skip the base path and any trailing separators
        const char* relative = path->value + base_len;
        while (*relative == '/' || *relative == '\\') {
            relative++;
        }
        return path_set(dst, relative);
    }
    
    // Path doesn't start with base, return as-is
    return path_copy(dst, path);
}

// ============================================================================
// text_t implementation
// ============================================================================

text_t* text_init(text_t* text)
{
    assert(text);
    text->value[0] = '\0';
    text->length = 0;
    return text;
}

text_t* text_set(text_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->value[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->value)) {
        src_len = sizeof(dst->value) - 1;
    }
    
    memcpy(dst->value, src, src_len);
    dst->value[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

text_t* text_copy(text_t* dst, const text_t* src)
{
    assert(dst);
    assert(src);
    
    if (dst != src) {
        memcpy(dst->value, src->value, src->length + 1);
        dst->length = src->length;
    }
    
    return dst;
}

text_t* text_append(text_t* dst, const char* src)
{
    assert(dst);
    
    if (!src || !*src) {
        return dst;
    }
    
    size_t src_len = strlen(src);
    size_t available = sizeof(dst->value) - dst->length - 1;
    
    if (src_len > available) {
        src_len = available;
    }
    
    if (src_len > 0) {
        memcpy(dst->value + dst->length, src, src_len);
        dst->length += src_len;
        dst->value[dst->length] = '\0';
    }
    
    return dst;
}

text_t* text_format(text_t* dst, const char* fmt, ...)
{
    assert(dst);
    assert(fmt);
    
    va_list args;
    va_start(args, fmt);
    
    int written = vsnprintf(dst->value, sizeof(dst->value), fmt, args);
    
    va_end(args);
    
    if (written < 0) {
        dst->value[0] = '\0';
        dst->length = 0;
    } else if ((size_t)written >= sizeof(dst->value)) {
        dst->length = sizeof(dst->value) - 1;
        dst->value[dst->length] = '\0';
    } else {
        dst->length = written;
    }
    
    return dst;
}

size_t text_length(const text_t* text)
{
    assert(text);
    return text->length;
}

text_t* text_clear(text_t* text)
{
    assert(text);
    text->value[0] = '\0';
    text->length = 0;
    return text;
}

text_t* text_trim(text_t* text)
{
    assert(text);
    
    if (text->length == 0) {
        return text;
    }
    
    // Trim leading whitespace
    size_t start = 0;
    while (start < text->length && isspace((unsigned char)text->value[start])) {
        start++;
    }
    
    // All whitespace?
    if (start == text->length) {
        text->value[0] = '\0';
        text->length = 0;
        return text;
    }
    
    // Trim trailing whitespace
    size_t end = text->length;
    while (end > start && isspace((unsigned char)text->value[end - 1])) {
        end--;
    }
    
    // Move content to beginning if needed
    if (start > 0) {
        memmove(text->value, text->value + start, end - start);
    }
    
    text->length = end - start;
    text->value[text->length] = '\0';
    
    return text;
}

bool text_equals(const text_t* a, const text_t* b)
{
    assert(a);
    assert(b);
    
    if (a->length != b->length) {
        return false;
    }
    
    return memcmp(a->value, b->value, a->length) == 0;
}

bool text_equals_cstr(const text_t* text, const char* str)
{
    assert(text);
    
    if (!str) {
        return text->length == 0;
    }
    
    size_t str_len = strlen(str);
    if (text->length != str_len) {
        return false;
    }
    
    return memcmp(text->value, str, str_len) == 0;
}