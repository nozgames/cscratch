//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <stdio.h>
#include <stdarg.h>

#define DEFAULT_INITIAL_CAPACITY 256

typedef struct stream_impl 
{
    OBJECT_BASE;
    uint8_t* data;
    size_t size;
    size_t capacity;
    size_t position;
} stream_impl_t;

static void stream_ensure_capacity(stream_impl_t* impl, size_t required_size);
static inline stream_impl_t* Impl(void* stream) { return (stream_impl_t*)to_object((Object*)stream, type_stream); }

stream_t* stream_alloc(Allocator* allocator, size_t capacity)
{
    stream_impl_t* impl = Impl(Alloc(allocator, sizeof(stream_impl_t), type_stream));
    if (!impl)
        return NULL;

    if (capacity == 0)
        capacity = DEFAULT_INITIAL_CAPACITY;

    impl->data = (u8*)malloc(capacity);
    if (!impl->data) 
    {
        Free((Object*)impl);
        return NULL;
    }
    
    impl->size = 0;
    impl->capacity = capacity;
    impl->position = 0;
    
    return (stream_t*)impl;
}

stream_t* stream_load_from_memory(Allocator* allocator, uint8_t* data, size_t size)
{
    stream_impl_t* impl = Impl(stream_alloc(allocator, size));
    if (!impl)
        return NULL;
        
    stream_ensure_capacity(impl, size);
    memcpy(impl->data, data, size);
    impl->size = size;
    impl->position = 0;
    
    return (stream_t*)impl;
}

stream_t* LoadStream(Allocator* allocator, path_t* path)
{
    assert(path);
    
    FILE* file = fopen(path->value, "rb");
    if (!file)
        return NULL;
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size == 0) 
    {
        fclose(file);
        return NULL;
    }
    
    // Create stream and read file
    void* t = stream_alloc(allocator, file_size + 1);
    stream_impl_t* impl = Impl(t); //  stream_alloc(allocator, file_size));
    if (!impl) 
    {
        fclose(file);
        return NULL;
    }
    
    stream_ensure_capacity(impl, file_size);
    size_t bytes_read = fread(impl->data, 1, file_size, file);
    fclose(file);
    
    impl->size = bytes_read;
    impl->position = 0;
    
    return (stream_t*)impl;
}

// todo: destructor
#if 0
void stream_destroy(stream_t* stream) 
{
	stream_impl_t* impl = Impl(stream);    
    Free((Object*)stream);
}
#endif

bool stream_save_to_file(stream_t* stream, path_t* path) 
{
    if (!stream || !path)
        return false;
    
    stream_impl_t* impl = Impl(stream);
    
    FILE* file = fopen(path->value, "wb");
    if (!file)
        return false;
    
    size_t bytes_written = fwrite(impl->data, 1, impl->size, file);
    fclose(file);
    
    return bytes_written == impl->size;
}

uint8_t* stream_data(stream_t* stream)
{
    return Impl(stream)->data;
}

size_t stream_size(stream_t* stream) 
{
	return Impl(stream)->size;
}

void stream_clear(stream_t* stream) 
{
	stream_impl_t* impl = Impl(stream);
    impl->size = 0;
    impl->position = 0;
}

size_t stream_position(stream_t* stream) 
{
    return Impl(stream)->position;
}

void stream_set_position(stream_t* stream, size_t position) 
{
    stream_impl_t* impl = Impl(stream);
    impl->position = position;
}

size_t stream_seek_begin(stream_t* stream, size_t offset) 
{
    stream_set_position(stream, offset);
    return stream_position(stream);
}

size_t stream_seek_end(stream_t* stream, size_t offset)
{
    stream_impl_t* impl = Impl(stream);
    stream_set_position(stream, i32_max(i32(impl->size - offset), 0));
    return impl->position;
}

bool stream_is_eos(stream_t* stream) 
{
	stream_impl_t* impl = Impl(stream);
    return impl->position >= impl->size;
}

bool stream_read_signature(stream_t* stream, const char* expected_signature, size_t signature_length) 
{
    if (!stream || !expected_signature) return false;
    
    stream_impl_t* impl = Impl(stream);
    
    if (impl->position + signature_length > impl->size) return false;
    
    bool match = memcmp(impl->data + impl->position, expected_signature, signature_length) == 0;
    if (match) 
    {
        impl->position += signature_length;
    }
    return match;
}

uint8_t stream_read_uint8(stream_t* stream) 
{
    if (!stream) return 0;
    
    stream_impl_t* impl = Impl(stream);
    
    if (impl->position + sizeof(uint8_t) > impl->size) return 0;
    
    uint8_t value = impl->data[impl->position];
    impl->position += sizeof(uint8_t);
    return value;
}

uint16_t stream_read_uint16(stream_t* stream) 
{
    uint16_t value;
    stream_read(stream, &value, sizeof(uint16_t));
    return value;
}

uint32_t stream_read_uint32(stream_t* stream) 
{
    uint32_t value;
    stream_read(stream, &value, sizeof(uint32_t));
    return value;
}

uint64_t stream_read_uint64(stream_t* stream) 
{
    uint64_t value;
    stream_read(stream, &value, sizeof(uint64_t));
    return value;
}

int8_t stream_read_int8(stream_t* stream) 
{
    return (int8_t)stream_read_uint8(stream);
}

int16_t stream_read_int16(stream_t* stream) 
{
    return (int16_t)stream_read_uint16(stream);
}

int32_t stream_read_int32(stream_t* stream) 
{
    return (int32_t)stream_read_uint32(stream);
}

int64_t stream_read_int64(stream_t* stream) 
{
    return (int64_t)stream_read_uint64(stream);
}

float stream_read_float(stream_t* stream) 
{
    float value;
    stream_read(stream, &value, sizeof(float));
    return value;
}

double stream_read_double(stream_t* stream) 
{
    double value;
    stream_read(stream, &value, sizeof(double));
    return value;
}

bool stream_read_bool(stream_t* stream) 
{
    return stream_read_uint8(stream) != 0;
}

void stream_read_bytes(stream_t* stream, uint8_t* dest, size_t count) 
{
    stream_read(stream, dest, count);
}

void stream_read(stream_t* stream, void* dest, size_t size) 
{
    if (!stream || !dest || size == 0) return;
    
    stream_impl_t* impl = Impl(stream);
    
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
void stream_write_signature(stream_t* stream, const char* signature, size_t signature_length) 
{
    stream_write_bytes(stream, (uint8_t*)signature, signature_length);
}

void stream_write_uint8(stream_t* stream, uint8_t value) 
{
    stream_write(stream, &value, sizeof(uint8_t));
}

void stream_write_uint16(stream_t* stream, uint16_t value) 
{
    stream_write(stream, &value, sizeof(uint16_t));
}

void stream_write_uint32(stream_t* stream, uint32_t value) 
{
    stream_write(stream, &value, sizeof(uint32_t));
}

void stream_write_uint64(stream_t* stream, uint64_t value) 
{
    stream_write(stream, &value, sizeof(uint64_t));
}

void stream_write_int8(stream_t* stream, int8_t value) 
{
    stream_write_uint8(stream, (uint8_t)value);
}

void stream_write_int16(stream_t* stream, int16_t value) 
{
    stream_write_uint16(stream, (uint16_t)value);
}

void stream_write_int32(stream_t* stream, int32_t value) 
{
    stream_write_uint32(stream, (uint32_t)value);
}

void stream_write_int64(stream_t* stream, int64_t value) 
{
    stream_write_uint64(stream, (uint64_t)value);
}

void stream_write_float(stream_t* stream, float value) 
{
    stream_write(stream, &value, sizeof(float));
}

void stream_write_double(stream_t* stream, double value) 
{
    stream_write(stream, &value, sizeof(double));
}

void stream_write_bool(stream_t* stream, bool value) 
{
    stream_write_uint8(stream, value ? 1 : 0);
}

void stream_write_string(stream_t* stream, const char* value) 
{
    if (!stream) return;
    
    if (!value) 
    {
        stream_write_uint32(stream, 0);
        return;
    }
    
    size_t length = strlen(value);
    stream_write_uint32(stream, (uint32_t)length);
    stream_write_bytes(stream, (uint8_t*)value, length);
}

void stream_write_raw_cstr(stream_t* stream, const char* format, ...)
{
    if (!stream || !format) return;
    
    char buffer[4096];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (written > 0 && (size_t)written < sizeof(buffer))
    {
        stream_write_bytes(stream, (uint8_t*)buffer, written);
    }
}

void stream_write_bytes(stream_t* stream, uint8_t* data, size_t size) 
{
    stream_write(stream, data, size);
}

void stream_write(stream_t* stream, void* src, size_t size) 
{
    if (!stream || !src || size == 0) return;
    
    stream_impl_t* impl = Impl(stream);
    
    stream_ensure_capacity(impl, impl->position + size);
    
    memcpy(impl->data + impl->position, src, size);
    impl->position += size;
    
    // Update size if we've written past the current end
    if (impl->position > impl->size) 
    {
        impl->size = impl->position;
    }
}

// Color operations
color_t stream_read_color(stream_t* stream) 
{
    color_t color = {0.0f, 0.0f, 0.0f, 1.0f};
    stream_read(stream, &color, sizeof(color_t));
    return color;
}

void stream_write_color(stream_t* stream, color_t value) 
{
    stream_write(stream, &value, sizeof(color_t));
}

// Internal helper functions
static void stream_ensure_capacity(stream_impl_t* impl, size_t required_size) 
{
    if (required_size <= impl->capacity) return;
    
    size_t new_capacity = impl->capacity;
    while (new_capacity < required_size) 
    {
        new_capacity *= 2;
    }
    
    u8* new_data = (u8*)realloc(impl->data, new_capacity);
    if (new_data) 
    {
        impl->data = new_data;
        impl->capacity = new_capacity;
    }
}
