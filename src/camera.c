#include "scene.h"

typedef struct camera_impl
{
    ivec2 view_size;
} camera_impl;

static object_type_t g_camera_type = {0};

void camera_init()
{
    g_camera_type = object_type_register("camera");
}

object_type_t camera_type()
{
    return g_camera_type;
}

camera_t camera_create()
{
    entity_t e = entity_create(g_camera_type, sizeof(camera_impl));
    camera_impl* impl = (camera_impl*)object_impl((object_t)e, g_camera_type);
    impl->view_size[0] = 0;
    return (camera_t)e;
}
