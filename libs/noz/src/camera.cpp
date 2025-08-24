//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl
{
    ENTITY_BASE;
    ivec2 view_size;
    mat4 projection;
};

static CameraImpl* Impl(Camera* c) { return (CameraImpl*)Cast(c, type_camera); }

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* camera = Impl((Camera*)CreateEntity(allocator, sizeof(CameraImpl), type_camera));
    camera->view_size = { 800, 600 };
    return (Camera*)camera;
}

mat4 camera_projection(Camera* camera)
{
    return Impl(camera)->projection;
}
