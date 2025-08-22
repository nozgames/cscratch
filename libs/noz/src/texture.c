//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct texture_impl
{
    int width;
    int height;
} texture_impl;

static object_type_t g_texture_type = {0};
static object_pool_t g_texture_pool = NULL;

void texture_init()
{
    g_texture_type = object_type_create("texture");
    g_texture_pool = object_pool_create(g_texture_type, sizeof(texture_impl), 32);
}

void texture_uninit()
{
    object_destroy((object_t)g_texture_pool);
}

object_type_t texture_type() 
{
    return g_texture_type;
}


texture_t texture_create(int width, int height)
{
//    object_t o = 

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