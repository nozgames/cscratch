//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/array.h>
#include <stdlib.h>
#include <string.h>

array_t* array_create(size_t element_size, size_t initial_capacity)
{
    if (element_size == 0) return NULL;
    
    array_t* array = (array_t*)malloc(sizeof(array_t));
    if (!array) return NULL;
    
    array->element_size = element_size;
    array->length = 0;
    array->capacity = initial_capacity;
    
    if (initial_capacity > 0)
    {
        array->data = malloc(element_size * initial_capacity);
        if (!array->data)
        {
            free(array);
            return NULL;
        }
    }
    else
    {
        array->data = NULL;
    }
    
    return array;
}

void array_destroy(array_t* array)
{
    if (!array) return;
    
    if (array->data)
        free(array->data);
    free(array);
}

array_t* array_alloc(array_t* array, size_t new_capacity)
{
    if (!array) return NULL;
    
    if (new_capacity == 0)
    {
        if (array->data)
        {
            free(array->data);
            array->data = NULL;
        }
        array->capacity = 0;
        array->length = 0;
        return array;
    }
    
    void* new_data = realloc(array->data, array->element_size * new_capacity);
    if (!new_data) return NULL;
    
    array->data = new_data;
    array->capacity = new_capacity;
    
    if (array->length > new_capacity)
        array->length = new_capacity;
    
    return array;
}

void* array_data(const array_t* array)
{
    return array ? array->data : NULL;
}

size_t array_length(const array_t* array)
{
    return array ? array->length : 0;
}

size_t array_capacity(const array_t* array)
{
    return array ? array->capacity : 0;
}

void* array_push(array_t* array)
{
    if (!array) return NULL;
    
    if (array->length >= array->capacity)
    {
        size_t new_capacity = array->capacity == 0 ? 8 : array->capacity * 2;
        if (!array_alloc(array, new_capacity))
            return NULL;
    }
    
    void* element = (char*)array->data + (array->length * array->element_size);
    array->length++;
    
    memset(element, 0, array->element_size);
    return element;
}

void* array_get(const array_t* array, size_t index)
{
    if (!array || index >= array->length) return NULL;
    
    return (char*)array->data + (index * array->element_size);
}

void array_clear(array_t* array)
{
    if (array)
        array->length = 0;
}

bool array_is_empty(const array_t* array)
{
    return !array || array->length == 0;
}