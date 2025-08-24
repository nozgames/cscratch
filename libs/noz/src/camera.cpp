//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct CameraImpl
{
    ENTITY_BASE;
    ivec2 view_size;
    mat4 projection;
};

static CameraImpl* Impl(Camera* c) { return (CameraImpl*)Cast(c, TYPE_CAMERA); }

Camera* CreateCamera(Allocator* allocator)
{
    CameraImpl* camera = Impl((Camera*)CreateEntity(allocator, sizeof(CameraImpl), TYPE_CAMERA));
    camera->view_size = { 800, 600 };
    return (Camera*)camera;
}

const mat4& GetProjection(Camera* camera)
{
    return Impl(camera)->projection;
}
