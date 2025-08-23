//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct object_impl stream_t;

// @alloc
stream_t* stream_alloc(allocator_t* allocator, size_t capacity);
stream_t* stream_load_from_memory(allocator_t* allocator, const uint8_t* data, size_t size);
stream_t* stream_load_from_file(allocator_t* allocator, const path_t* path);

// @file
bool stream_save_to_file(const stream_t* stream, const path_t* path);

// @data
const uint8_t* stream_data(const stream_t* stream);
size_t stream_size(const stream_t* stream);
void stream_clear(stream_t* stream);

// @position
size_t stream_position(const stream_t* stream);
void stream_set_position(stream_t* stream, size_t position);
size_t stream_seek_begin(stream_t* stream, size_t offset);
size_t stream_seek_end(stream_t* stream, size_t offset);
bool stream_is_eos(const stream_t* stream);

// @read
bool stream_read_signature(stream_t* stream, const char* expected_signature, size_t signature_length);
uint8_t stream_read_uint8(stream_t* stream);
uint16_t stream_read_uint16(stream_t* stream);
uint32_t stream_read_uint32(stream_t* stream);
uint64_t stream_read_uint64(stream_t* stream);
int8_t stream_read_int8(stream_t* stream);
int16_t stream_read_int16(stream_t* stream);
int32_t stream_read_int32(stream_t* stream);
int64_t stream_read_int64(stream_t* stream);
float stream_read_float(stream_t* stream);
double stream_read_double(stream_t* stream);
bool stream_read_bool(stream_t* stream);
char* stream_read_string(stream_t* stream);
void stream_read_bytes(stream_t* stream, uint8_t* dest, size_t count);
void stream_read(stream_t* stream, void* dest, size_t size);
color_t stream_read_color(stream_t* stream);

// @write
void stream_write_signature(stream_t* stream, const char* signature, size_t signature_length);
void stream_write_uint8(stream_t* stream, uint8_t value);
void stream_write_uint16(stream_t* stream, uint16_t value);
void stream_write_uint32(stream_t* stream, uint32_t value);
void stream_write_uint64(stream_t* stream, uint64_t value);
void stream_write_int8(stream_t* stream, int8_t value);
void stream_write_int16(stream_t* stream, int16_t value);
void stream_write_int32(stream_t* stream, int32_t value);
void stream_write_int64(stream_t* stream, int64_t value);
void stream_write_float(stream_t* stream, float value);
void stream_write_double(stream_t* stream, double value);
void stream_write_bool(stream_t* stream, bool value);
void stream_write_string(stream_t* stream, const char* value);
void stream_write_raw_cstr(stream_t* stream, const char* format, ...); // Write formatted C string without length prefix
void stream_write_bytes(stream_t* stream, const uint8_t* data, size_t size);
void stream_write(stream_t* stream, const void* src, size_t size);
void stream_write_color(stream_t* stream, color_t value);


#define stream_read_struct(stream, type) \
    ({ type result; stream_read(stream, &result, sizeof(type)); result; })

#define stream_write_struct(stream, value) \
    stream_write(stream, &(value), sizeof(value))