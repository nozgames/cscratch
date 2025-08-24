//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct camera_impl_t
{
    ENTITY_BASE;
    ivec2 view_size;
	mat4 projection;
};

static inline camera_impl_t* to_impl(camera_t* c) { return (camera_impl_t*)to_object(c, type_camera); }

camera_t* camera_create(allocator_t* allocator)
{
	camera_impl_t* camera = to_impl(entity_create(allocator, sizeof(camera_impl_t), type_camera));
	camera->view_size = { 800, 600 };
    return (camera_t*)camera;
}

mat4 camera_projection(camera_t* camera)
{
    return to_impl(camera)->projection;
}

void camera_init()
{
}

void camera_uninit()
{
}
