#pragma once

typedef unsigned int uint;

typedef struct string128
{
    char data[128];
    uint length;
} string128_t;

string128_t* string128_set(string128_t* dst, const char* src);
string128_t* string128_copy(string128_t* dst, const string128_t* src);