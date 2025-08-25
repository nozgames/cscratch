//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "assets.h"
int main(int argc, char* argv[])
{
    InitApplication(nullptr);
    LoadAssets();

    auto material = CreateMaterial(ALLOCATOR_DEFAULT, Assets.shaders._default);
    SetTexture(material, Assets.textures.palette);

    while (UpdateApplication())
    {
        BeginRenderFrame();
        BeginRenderPass(true, color_blue, false, nullptr);

        vec3 camera_position = vec3(0.0f,1000.0f,0.0f);
        mat4 transform = glm::translate(identity<mat4>(), camera_position)
            * mat4_cast(quat(vec3(glm::radians(-90.0f), 0, 0)));
        mat4 ortho = glm::ortho(-2.0f,2.0f,-2.0f,2.0f, 0.001f, 100.0f);
        BindCamera(glm::inverse(transform), ortho);

        BindColor(color_white);
        BindTransform(identity<mat4>());
        BindMaterial(material);
        DrawMesh(Assets.meshes.cursors.pickaxecursor);

        EndRenderPass();

        EndRenderFrame();
    }

    UnloadAssets();
    ShutdownApplication();
}
