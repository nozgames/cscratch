//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Props : Object {};

enum PropType
{
    PROP_TYPE_VALUE,
    PROP_TYPE_LIST
};

Props* CreateProps(Allocator* allocator);
Props* LoadProps(Allocator* allocator, Path* file_path);
Props* LoadProps(Allocator* allocator, const char* content, size_t content_length);
void Clear(Props* props);
void SetString(Props* props, const char* key, const char* value);
void SetInt(Props* props, const char* key, int value);
void SetFloat(Props* props, const char* key, float value);
void SetVec3(Props* props, const char* key, vec3 value);
void AddToList(Props* props, const char* key, const char* value);
bool HasKey(Props* props, const char* key);
bool IsList(Props* props, const char* key);
PropType GetPropertyType(Props* props, const char* key);
const char* GetString(Props* props, const char* key, const char* default_value);
int GetInt(Props* props, const char* key, int default_value);
float GetFloat(Props* props, const char* key, float default_value);
bool GetBool(Props* props, const char* key, bool default_value);
vec3 GetVec3(Props* props, const char* key, vec3 default_value);
size_t GetListCount(Props* props, const char* key);
const char* GetListElement(Props* props, const char* key, size_t index, const char* default_value);
size_t GetKeyCount(Props* props);
const char* GetKeyAt(Props* props, size_t index);
void Print(Props* props);
