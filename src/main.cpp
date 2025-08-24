//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

int main(int argc, char* argv[])
{
    InitApplication(nullptr);

    while (UpdateApplication())
    {
        BeginRenderFrame();

        BeginRenderPass(true, color_blue, false, nullptr);

        EndRenderPass();

        EndRenderFrame();
    }

    ShutdownApplication();
}
