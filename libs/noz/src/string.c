//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "string.h"

string128_t* string128_set(string128_t* dst, const char* src)
{
    assert(dst);
    
    if (src == nullptr)
    {
        dst->data[0] = 0;
        dst->length = 0;
        return dst;
    }

    size_t src_length = strlen(src);
    if (src_length >= sizeof(dst->data)) 
        src_length = sizeof(dst->data) - 1;
    memcpy(dst->data, src, src_length + 1);
    return dst;
}

string128_t* string128_copy(string128_t* dst, const string128_t* src)
{
    assert(dst);
    assert(src);
    dst->length = src->length;
    if (dst->length > 0)
        memcpy(dst->data, src->data, dst->length + 1);
    return dst;
}
