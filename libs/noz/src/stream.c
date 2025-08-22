/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include "noz/stream.h"
#include "noz/object.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// Initial capacity for binary stream buffer
#define INITIAL_CAPACITY 1024

// Stream implementation
typedef struct stream_impl 
{
    uint8_t* data;
    size_t size;
    size_t capacity;
    size_t position;
} stream_impl_t;

static object_type_t stream_type = NULL;

// Internal helper functions
static void ensure_capacity(stream_impl_t* impl, size_t required_size);

stream_t stream_create(void) 
{
    init_stream_type();
    
    object_t obj = object_create(stream_type, sizeof(stream_impl_t));
    if (!obj) return NULL;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl(obj, stream_type);
    impl->data = malloc(INITIAL_CAPACITY);
    if (!impl->data) 
    {
        object_destroy(obj);
        return NULL;
    }
    
    impl->size = 0;
    impl->capacity = INITIAL_CAPACITY;
    impl->position = 0;
    
    return (stream_t)obj;
}

stream_t stream_create_from_data(const uint8_t* data, size_t size) 
{
    stream_t stream = stream_create();
    if (!stream) return NULL;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    ensure_capacity(impl, size);
    memcpy(impl->data, data, size);
    impl->size = size;
    impl->position = 0;
    
    return stream;
}

stream_t stream_create_from_file(const char* path) 
{
    if (!path) return NULL;
    
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) 
    {
        fclose(file);
        return NULL;
    }
    
    // Create stream and read file
    stream_t stream = stream_create();
    if (!stream) 
    {
        fclose(file);
        return NULL;
    }
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    ensure_capacity(impl, (size_t)file_size);
    size_t bytes_read = fread(impl->data, 1, (size_t)file_size, file);
    fclose(file);
    
    impl->size = bytes_read;
    impl->position = 0;
    
    return stream;
}

void stream_destroy(stream_t stream) 
{
    if (!stream) return;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    free(impl->data);
    object_destroy((object_t)stream);
}

bool stream_save(stream_t stream, const char* path) 
{
    if (!stream || !path) return false;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    FILE* file = fopen(path, "wb");
    if (!file) return false;
    
    size_t bytes_written = fwrite(impl->data, 1, impl->size, file);
    fclose(file);
    
    return bytes_written == impl->size;
}

const uint8_t* stream_data(stream_t stream) 
{
    if (!stream) return NULL;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    return impl->data;
}

size_t stream_size(stream_t stream) 
{
    if (!stream) return 0;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    return impl->size;
}

void stream_clear(stream_t stream) 
{
    if (!stream) return;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    impl->size = 0;
    impl->position = 0;
}

size_t stream_position(stream_t stream) 
{
    if (!stream) return 0;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    return impl->position;
}

void stream_set_position(stream_t stream, size_t position) 
{
    if (!stream) return;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    impl->position = position;
}

size_t stream_seek_begin(stream_t stream, size_t offset) 
{
    if (!stream) return 0;
    
    stream_set_position(stream, offset);
    return stream_position(stream);
}

bool stream_is_eos(stream_t stream) 
{
    if (!stream) return true;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    return impl->position >= impl->size;
}

// Reading operations
bool stream_read_signature(stream_t stream, const char* expected_signature, size_t signature_length) 
{
    if (!stream || !expected_signature) return false;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    if (impl->position + signature_length > impl->size) return false;
    
    bool match = memcmp(impl->data + impl->position, expected_signature, signature_length) == 0;
    if (match) 
    {
        impl->position += signature_length;
    }
    return match;
}

uint8_t stream_read_uint8(stream_t stream) 
{
    if (!stream) return 0;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    if (impl->position + sizeof(uint8_t) > impl->size) return 0;
    
    uint8_t value = impl->data[impl->position];
    impl->position += sizeof(uint8_t);
    return value;
}

uint16_t stream_read_uint16(stream_t stream) 
{
    uint16_t value;
    stream_read(stream, &value, sizeof(uint16_t));
    return value;
}

uint32_t stream_read_uint32(stream_t stream) 
{
    uint32_t value;
    stream_read(stream, &value, sizeof(uint32_t));
    return value;
}

uint64_t stream_read_uint64(stream_t stream) 
{
    uint64_t value;
    stream_read(stream, &value, sizeof(uint64_t));
    return value;
}

int8_t stream_read_int8(stream_t stream) 
{
    return (int8_t)stream_read_uint8(stream);
}

int16_t stream_read_int16(stream_t stream) 
{
    return (int16_t)stream_read_uint16(stream);
}

int32_t stream_read_int32(stream_t stream) 
{
    return (int32_t)stream_read_uint32(stream);
}

int64_t stream_read_int64(stream_t stream) 
{
    return (int64_t)stream_read_uint64(stream);
}

float stream_read_float(stream_t stream) 
{
    float value;
    stream_read(stream, &value, sizeof(float));
    return value;
}

double stream_read_double(stream_t stream) 
{
    double value;
    stream_read(stream, &value, sizeof(double));
    return value;
}

bool stream_read_bool(stream_t stream) 
{
    return stream_read_uint8(stream) != 0;
}

char* stream_read_string(stream_t stream) 
{
    if (!stream) return NULL;
    
    uint32_t length = stream_read_uint32(stream);
    if (length == 0) 
    {
        char* empty_str = malloc(1);
        if (empty_str) empty_str[0] = '\0';
        return empty_str;
    }
    
    char* str = malloc(length + 1);
    if (!str) return NULL;
    
    stream_read_bytes(stream, (uint8_t*)str, length);
    str[length] = '\0';
    
    return str;
}

void stream_read_bytes(stream_t stream, uint8_t* dest, size_t count) 
{
    stream_read(stream, dest, count);
}

void stream_read(stream_t stream, void* dest, size_t size) 
{
    if (!stream || !dest || size == 0) return;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    if (impl->position + size > impl->size) 
    {
        // Read what we can and zero the rest
        size_t available = impl->size - impl->position;
        if (available > 0) 
        {
            memcpy(dest, impl->data + impl->position, available);
            impl->position += available;
        }
        // Zero remaining bytes
        if (size > available) 
        {
            memset((uint8_t*)dest + available, 0, size - available);
        }
        return;
    }
    
    memcpy(dest, impl->data + impl->position, size);
    impl->position += size;
}

// Writing operations
void stream_write_signature(stream_t stream, const char* signature, size_t signature_length) 
{
    stream_write_bytes(stream, (const uint8_t*)signature, signature_length);
}

void stream_write_uint8(stream_t stream, uint8_t value) 
{
    stream_write(stream, &value, sizeof(uint8_t));
}

void stream_write_uint16(stream_t stream, uint16_t value) 
{
    stream_write(stream, &value, sizeof(uint16_t));
}

void stream_write_uint32(stream_t stream, uint32_t value) 
{
    stream_write(stream, &value, sizeof(uint32_t));
}

void stream_write_uint64(stream_t stream, uint64_t value) 
{
    stream_write(stream, &value, sizeof(uint64_t));
}

void stream_write_int8(stream_t stream, int8_t value) 
{
    stream_write_uint8(stream, (uint8_t)value);
}

void stream_write_int16(stream_t stream, int16_t value) 
{
    stream_write_uint16(stream, (uint16_t)value);
}

void stream_write_int32(stream_t stream, int32_t value) 
{
    stream_write_uint32(stream, (uint32_t)value);
}

void stream_write_int64(stream_t stream, int64_t value) 
{
    stream_write_uint64(stream, (uint64_t)value);
}

void stream_write_float(stream_t stream, float value) 
{
    stream_write(stream, &value, sizeof(float));
}

void stream_write_double(stream_t stream, double value) 
{
    stream_write(stream, &value, sizeof(double));
}

void stream_write_bool(stream_t stream, bool value) 
{
    stream_write_uint8(stream, value ? 1 : 0);
}

void stream_write_string(stream_t stream, const char* value) 
{
    if (!stream) return;
    
    if (!value) 
    {
        stream_write_uint32(stream, 0);
        return;
    }
    
    size_t length = strlen(value);
    stream_write_uint32(stream, (uint32_t)length);
    stream_write_bytes(stream, (const uint8_t*)value, length);
}

void stream_write_bytes(stream_t stream, const uint8_t* data, size_t size) 
{
    stream_write(stream, data, size);
}

void stream_write(stream_t stream, const void* src, size_t size) 
{
    if (!stream || !src || size == 0) return;
    
    stream_impl_t* impl = (stream_impl_t*)object_impl((object_t)stream, stream_type);
    
    ensure_capacity(impl, impl->position + size);
    
    memcpy(impl->data + impl->position, src, size);
    impl->position += size;
    
    // Update size if we've written past the current end
    if (impl->position > impl->size) 
    {
        impl->size = impl->position;
    }
}

// Color operations
color_t stream_read_color(stream_t stream) 
{
    color_t color = {0.0f, 0.0f, 0.0f, 1.0f};
    stream_read(stream, &color, sizeof(color_t));
    return color;
}

void stream_write_color(stream_t stream, color_t value) 
{
    stream_write(stream, &value, sizeof(color_t));
}

// Internal helper functions
static void ensure_capacity(stream_impl_t* impl, size_t required_size) 
{
    if (required_size <= impl->capacity) return;
    
    size_t new_capacity = impl->capacity;
    while (new_capacity < required_size) 
    {
        new_capacity *= 2;
    }
    
    uint8_t* new_data = realloc(impl->data, new_capacity);
    if (new_data) 
    {
        impl->data = new_data;
        impl->capacity = new_capacity;
    }
}

static void stream_init(void) 
{
    stream_type = object_type_create("stream", sizeof(stream_impl_t));
}

static void stream_uninit(void)
{
}
