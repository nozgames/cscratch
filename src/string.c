#include "string.h"

string128* string128_set(string128* dst, const char* src)
{
    assert(dst);
    assert(src);

    size_t src_length = strlen(src);
    if (src_length >= sizeof(dst->data)) 
        src_length = sizeof(dst->data) - 1;
    memcpy(dst->data, src, src_length + 1);
    return dst;
}

string128* string128_copy(string128* dst, const string128* src)
{
    assert(dst);
    assert(src);
    dst->length = src->length;
    if (dst->length > 0)
        memcpy(dst->data, src->data, dst->length + 1);
    return dst;
}
