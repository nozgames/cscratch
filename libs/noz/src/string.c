//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/string.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

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

// ============================================================================
// name_t implementation
// ============================================================================

name_t* name_set(name_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->data[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->data)) {
        src_len = sizeof(dst->data) - 1;
    }
    
    memcpy(dst->data, src, src_len);
    dst->data[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

name_t* name_format(name_t* dst, const char* fmt, ...)
{
    assert(dst);
    assert(fmt);
    
    va_list args;
    va_start(args, fmt);
    
    int written = vsnprintf(dst->data, sizeof(dst->data), fmt, args);
    
    va_end(args);
    
    if (written < 0) {
        dst->data[0] = '\0';
        dst->length = 0;
    } else if ((size_t)written >= sizeof(dst->data)) {
        dst->length = sizeof(dst->data) - 1;
        dst->data[dst->length] = '\0';
    } else {
        dst->length = written;
    }
    
    return dst;
}

// ============================================================================
// path_t implementation  
// ============================================================================

path_t* path_set(path_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->data[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->data)) {
        src_len = sizeof(dst->data) - 1;
    }
    
    memcpy(dst->data, src, src_len);
    dst->data[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

path_t* path_copy(path_t* dst, const path_t* src)
{
    assert(dst);
    assert(src);
    
    if (dst != src) {
        memcpy(dst->data, src->data, src->length + 1);
        dst->length = src->length;
    }
    
    return dst;
}

path_t* path_append(path_t* dst, const char* component)
{
    assert(dst);
    
    if (!component || !*component) {
        return dst;
    }
    
    // Add separator if needed
    if (dst->length > 0 && dst->data[dst->length - 1] != '/' && dst->data[dst->length - 1] != '\\') {
        if (dst->length + 1 < sizeof(dst->data)) {
            dst->data[dst->length++] = '/';
            dst->data[dst->length] = '\0';
        }
    }
    
    // Append component
    size_t comp_len = strlen(component);
    size_t available = sizeof(dst->data) - dst->length - 1;
    
    if (comp_len > available) {
        comp_len = available;
    }
    
    if (comp_len > 0) {
        memcpy(dst->data + dst->length, component, comp_len);
        dst->length += comp_len;
        dst->data[dst->length] = '\0';
    }
    
    return dst;
}

path_t* path_join(path_t* dst, const char* base, const char* component)
{
    assert(dst);
    
    path_set(dst, base);
    return path_append(dst, component);
}

const char* path_dirname(const path_t* path)
{
    assert(path);
    
    static char dirname_buf[1024];
    
    if (path->length == 0) {
        dirname_buf[0] = '.';
        dirname_buf[1] = '\0';
        return dirname_buf;
    }
    
    // Find last separator
    const char* last_sep = nullptr;
    for (size_t i = 0; i < path->length; i++) {
        if (path->data[i] == '/' || path->data[i] == '\\') {
            last_sep = &path->data[i];
        }
    }
    
    if (!last_sep) {
        dirname_buf[0] = '.';
        dirname_buf[1] = '\0';
        return dirname_buf;
    }
    
    size_t dir_len = last_sep - path->data;
    if (dir_len == 0) {
        dirname_buf[0] = '/';
        dirname_buf[1] = '\0';
    } else {
        memcpy(dirname_buf, path->data, dir_len);
        dirname_buf[dir_len] = '\0';
    }
    
    return dirname_buf;
}

const char* path_basename(const path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return "";
    }
    
    // Find last separator
    const char* last_sep = nullptr;
    for (size_t i = 0; i < path->length; i++) {
        if (path->data[i] == '/' || path->data[i] == '\\') {
            last_sep = &path->data[i];
        }
    }
    
    if (!last_sep) {
        return path->data;
    }
    
    return last_sep + 1;
}

const char* path_extension(const path_t* path)
{
    assert(path);
    
    if (path->length == 0) {
        return "";
    }
    
    // Find last dot after last separator
    const char* last_dot = nullptr;
    const char* last_sep = nullptr;
    
    for (size_t i = 0; i < path->length; i++) {
        if (path->data[i] == '/' || path->data[i] == '\\') {
            last_sep = &path->data[i];
            last_dot = nullptr; // Reset dot search after separator
        } else if (path->data[i] == '.') {
            last_dot = &path->data[i];
        }
    }
    
    if (!last_dot || last_dot == path->data) {
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
    
    // Case-insensitive comparison
    while (*path_ext && *ext) {
        char c1 = *path_ext;
        char c2 = *ext;
        
        // Convert to lowercase for comparison
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return false;
        }
        
        path_ext++;
        ext++;
    }
    
    return *path_ext == '\0' && *ext == '\0';
}

bool path_cstr_has_extension(const char* path_str, const char* ext)
{
    if (!path_str || !ext) {
        return false;
    }
    
    // Find the extension in the C string
    const char* last_dot = nullptr;
    const char* last_sep = nullptr;
    const char* p = path_str;
    
    while (*p) {
        if (*p == '/' || *p == '\\') {
            last_sep = p;
            last_dot = nullptr;
        } else if (*p == '.') {
            last_dot = p;
        }
        p++;
    }
    
    if (!last_dot || last_dot == path_str) {
        return false;
    }
    
    // Compare extension (case-insensitive)
    const char* path_ext = last_dot + 1;
    while (*path_ext && *ext) {
        char c1 = *path_ext;
        char c2 = *ext;
        
        // Convert to lowercase for comparison
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return false;
        }
        
        path_ext++;
        ext++;
    }
    
    return *path_ext == '\0' && *ext == '\0';
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
        while (i < path->length && (path->data[i] == '/' || path->data[i] == '\\')) {
            if (temp_len == 0 || (temp_len > 0 && temp[temp_len - 1] != '/')) {
                temp[temp_len++] = '/';
            }
            i++;
        }
        
        // Get next component
        size_t comp_start = i;
        while (i < path->length && path->data[i] != '/' && path->data[i] != '\\') {
            i++;
        }
        
        if (comp_start < i) {
            size_t comp_len = i - comp_start;
            
            // Handle . and ..
            if (comp_len == 1 && path->data[comp_start] == '.') {
                // Skip current directory marker
                continue;
            } else if (comp_len == 2 && path->data[comp_start] == '.' && path->data[comp_start + 1] == '.') {
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
                memcpy(temp + temp_len, path->data + comp_start, comp_len);
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
        path->data[0] = '.';
        path->data[1] = '\0';
        path->length = 1;
    } else {
        memcpy(path->data, temp, temp_len);
        path->data[temp_len] = '\0';
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
    if (path->data[0] == '/') {
        return true;
    }
    
    // Windows absolute path (C:\ or C:/)
    if (path->length >= 3 && 
        ((path->data[0] >= 'A' && path->data[0] <= 'Z') || 
         (path->data[0] >= 'a' && path->data[0] <= 'z')) &&
        path->data[1] == ':' &&
        (path->data[2] == '/' || path->data[2] == '\\')) {
        return true;
    }
    
    return false;
}

// ============================================================================
// text_t implementation
// ============================================================================

text_t* text_init(text_t* text)
{
    assert(text);
    text->data[0] = '\0';
    text->length = 0;
    return text;
}

text_t* text_set(text_t* dst, const char* src)
{
    assert(dst);
    
    if (!src) {
        dst->data[0] = '\0';
        dst->length = 0;
        return dst;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= sizeof(dst->data)) {
        src_len = sizeof(dst->data) - 1;
    }
    
    memcpy(dst->data, src, src_len);
    dst->data[src_len] = '\0';
    dst->length = src_len;
    
    return dst;
}

text_t* text_copy(text_t* dst, const text_t* src)
{
    assert(dst);
    assert(src);
    
    if (dst != src) {
        memcpy(dst->data, src->data, src->length + 1);
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
    size_t available = sizeof(dst->data) - dst->length - 1;
    
    if (src_len > available) {
        src_len = available;
    }
    
    if (src_len > 0) {
        memcpy(dst->data + dst->length, src, src_len);
        dst->length += src_len;
        dst->data[dst->length] = '\0';
    }
    
    return dst;
}

text_t* text_format(text_t* dst, const char* fmt, ...)
{
    assert(dst);
    assert(fmt);
    
    va_list args;
    va_start(args, fmt);
    
    int written = vsnprintf(dst->data, sizeof(dst->data), fmt, args);
    
    va_end(args);
    
    if (written < 0) {
        dst->data[0] = '\0';
        dst->length = 0;
    } else if ((size_t)written >= sizeof(dst->data)) {
        dst->length = sizeof(dst->data) - 1;
        dst->data[dst->length] = '\0';
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
    text->data[0] = '\0';
    text->length = 0;
    return text;
}

bool text_equals(const text_t* a, const text_t* b)
{
    assert(a);
    assert(b);
    
    if (a->length != b->length) {
        return false;
    }
    
    return memcmp(a->data, b->data, a->length) == 0;
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
    
    return memcmp(text->data, str, str_len) == 0;
}