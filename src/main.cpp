//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "assets.h"
int main(int argc, char* argv[])
{
    InitApplication(nullptr);
    LoadAssets();

    while (UpdateApplication())
    {
        BeginRenderFrame();

        BeginRenderPass(true, color_blue, false, nullptr);

        EndRenderPass();

        EndRenderFrame();
    }

    UnloadAssets();
    ShutdownApplication();
}
