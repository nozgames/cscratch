//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct array
{
    void* data;
    size_t length;
    size_t capacity;
    size_t element_size;
} array_t;

array_t* array_create(size_t element_size, size_t initial_capacity);
void array_destroy(array_t* array);
array_t* array_alloc(array_t* array, size_t new_capacity);
void* array_data(const array_t* array);
size_t array_length(const array_t* array);
size_t array_capacity(const array_t* array);
void* array_push(array_t* array);
void* array_get(const array_t* array, size_t index);
void array_clear(array_t* array);
bool array_is_empty(const array_t* array);
