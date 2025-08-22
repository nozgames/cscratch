#pragma once

typedef unsigned int uint;

typedef struct
{
    char data[128];
    uint length;
} string128;

string128* string128_set(string128* dst, const char* src);
string128* string128_copy(string128* dst, const string128* src);