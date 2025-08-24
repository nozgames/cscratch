//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Props : Object {};

typedef enum prop_type
{
    prop_type_value,
    prop_type_list
} prop_type_t;

Props* props_alloc(Allocator* allocator);
Props* props_load_from_file(Allocator* allocator, path_t* file_path);
Props* props_load_from_memory(Allocator* allocator, const char* content, size_t content_length);
void props_clear(Props* props);
void props_set_string(Props* props, const char* key, const char* value);
void props_set_int(Props* props, const char* key, int value);
void props_set_float(Props* props, const char* key, float value);
void props_set_vec3(Props* props, const char* key, vec3 value);
void props_add_to_list(Props* props, const char* key, const char* value);
bool props_has_key(Props* props, const char* key);
bool props_is_list(Props* props, const char* key);
prop_type_t props_get_type(Props* props, const char* key);
const char* props_get_string(Props* props, const char* key, const char* default_value);
int props_get_int(Props* props, const char* key, int default_value);
float props_get_float(Props* props, const char* key, float default_value);
bool props_get_bool(Props* props, const char* key, bool default_value);
vec3 props_get_vec3(Props* props, const char* key, vec3 default_value);
size_t props_get_list_count(Props* props, const char* key);
const char* props_get_list_item(Props* props, const char* key, size_t index, const char* default_value);
size_t props_get_key_count(Props* props);
const char* props_get_key_at(Props* props, size_t index);
void props_print(Props* props);
