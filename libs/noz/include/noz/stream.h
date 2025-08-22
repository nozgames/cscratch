//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "noz/color.h"

// Forward declarations
typedef struct stream* stream_t;

// Stream object using your object system
stream_t stream_create(void);
stream_t stream_create_from_data(const uint8_t* data, size_t size);
stream_t stream_create_from_file(const char* path);
void stream_destroy(stream_t stream);

// File operations
bool stream_save(stream_t stream, const char* path);

// Data access
const uint8_t* stream_data(stream_t stream);
size_t stream_size(stream_t stream);
void stream_clear(stream_t stream);

// Position management
size_t stream_position(stream_t stream);
void stream_set_position(stream_t stream, size_t position);
size_t stream_seek_begin(stream_t stream, size_t offset);
bool stream_is_eos(stream_t stream);

// Reading operations
bool stream_read_signature(stream_t stream, const char* expected_signature, size_t signature_length);

uint8_t stream_read_uint8(stream_t stream);
uint16_t stream_read_uint16(stream_t stream);
uint32_t stream_read_uint32(stream_t stream);
uint64_t stream_read_uint64(stream_t stream);
int8_t stream_read_int8(stream_t stream);
int16_t stream_read_int16(stream_t stream);
int32_t stream_read_int32(stream_t stream);
int64_t stream_read_int64(stream_t stream);
float stream_read_float(stream_t stream);
double stream_read_double(stream_t stream);
bool stream_read_bool(stream_t stream);

// String reading (returns allocated string - caller must free)
char* stream_read_string(stream_t stream);
void stream_read_bytes(stream_t stream, uint8_t* dest, size_t count);
void stream_read(stream_t stream, void* dest, size_t size);

// Color reading (if color_t is defined)
color_t stream_read_color(stream_t stream);

// Writing operations
void stream_write_signature(stream_t stream, const char* signature, size_t signature_length);

void stream_write_uint8(stream_t stream, uint8_t value);
void stream_write_uint16(stream_t stream, uint16_t value);
void stream_write_uint32(stream_t stream, uint32_t value);
void stream_write_uint64(stream_t stream, uint64_t value);
void stream_write_int8(stream_t stream, int8_t value);
void stream_write_int16(stream_t stream, int16_t value);
void stream_write_int32(stream_t stream, int32_t value);
void stream_write_int64(stream_t stream, int64_t value);
void stream_write_float(stream_t stream, float value);
void stream_write_double(stream_t stream, double value);
void stream_write_bool(stream_t stream, bool value);

// String writing
void stream_write_string(stream_t stream, const char* value);
void stream_write_bytes(stream_t stream, const uint8_t* data, size_t size);
void stream_write(stream_t stream, const void* src, size_t size);

// Color writing (if color_t is defined)
void stream_write_color(stream_t stream, color_t value);

// Convenience macros for reading/writing structs
#define stream_read_struct(stream, type) \
    ({ type result; stream_read(stream, &result, sizeof(type)); result; })

#define stream_write_struct(stream, value) \
    stream_write(stream, &(value), sizeof(value))