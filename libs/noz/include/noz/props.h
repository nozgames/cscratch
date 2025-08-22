//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct props* props_t;

typedef enum prop_type
{
    prop_type_value,
    prop_type_list
} prop_type_t;

props_t props_create(void);
void props_destroy(props_t props);
bool props_load_from_file(props_t props, const char* file_path);
bool props_load_from_string(props_t props, const char* content);
void props_clear(props_t props);
void props_set_string(props_t props, const char* key, const char* value);
void props_set_int(props_t props, const char* key, int value);
void props_set_float(props_t props, const char* key, float value);
void props_set_vec3(props_t props, const char* key, vec3_t value);
void props_add_to_list(props_t props, const char* key, const char* value);
bool props_has_key(props_t props, const char* key);
bool props_is_list(props_t props, const char* key);
prop_type_t props_get_type(props_t props, const char* key);
const char* props_get_string(props_t props, const char* key, const char* default_value);
int props_get_int(props_t props, const char* key, int default_value);
float props_get_float(props_t props, const char* key, float default_value);
vec3_t props_get_vec3(props_t props, const char* key, vec3_t default_value);
size_t props_get_list_count(props_t props, const char* key);
const char* props_get_list_item(props_t props, const char* key, size_t index, const char* default_value);
size_t props_get_key_count(props_t props);
const char* props_get_key_at(props_t props, size_t index);
void props_print(props_t props);
