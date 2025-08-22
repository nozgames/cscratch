//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct camera_impl
{
    ivec2_t view_size;
	mat4_t projection;
} camera_impl_t;

static object_type_t g_camera_type = {0};

static inline camera_impl_t* to_impl(camera_t camera)
{
    assert(camera);
    return (camera_impl_t*)object_impl((object_t)camera, g_camera_type);
}

object_type_t camera_type()
{
    return g_camera_type;
}

camera_t camera_create()
{
    camera_t camera = (camera_t)entity_create(g_camera_type, sizeof(camera_impl_t));
    camera_impl_t* impl = to_impl(camera);
    return (camera_t)camera;
}

mat4_t camera_projection(camera_t camera)
{
    return to_impl(camera)->projection;
}

void camera_init()
{
    g_camera_type = object_type_create("camera");
}

void camera_uninit()
{
	g_camera_type = nullptr;
}
