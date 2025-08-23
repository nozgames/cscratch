//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct camera_impl
{
    ENTITY_BASE;
    ivec2_t view_size;
	mat4_t projection;
} camera_impl_t;

#define to_impl(c) ((camera_impl_t*)to_object(c, type_camera))

camera_t* camera_create(allocator_t* allocator)
{
	camera_impl_t* camera = to_impl(entity_create(allocator, sizeof(camera_impl_t), type_camera));
	camera->view_size = (ivec2_t){ 800, 600 };
    return (camera_t*)camera;
}

mat4_t camera_projection(const camera_t* camera)
{
    return to_impl(camera)->projection;
}

void camera_init()
{
}

void camera_uninit()
{
}
