#include "renderer.h"
#include "object.h"

typedef struct texture_impl
{
    int width;
    int height;
} texture_impl;

static object_type_t g_texture_type = {0};

object_type_t texture_type() 
{
    return g_texture_type;
}

void texture_register_type()
{ 
    g_texture_type = object_type_register("texture");
}

texture_t texture_create(int width, int height)
{
//    object_t object = object_create(object_type_texture, sizeof(texture_impl));

    //texture_impl* t = (texture_impl*)malloc(sizeof(texture_impl));
    //assert(t);
    //t->width = width;
    //t->height = height;
    //return t;
return NULL;
}

int texture_width(texture_t texture)
{ return 0; }