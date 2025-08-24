//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

int main(int argc, char* argv[])
{
    InitApplication(nullptr);

    while (UpdateApplication())
    {
        BeginRenderFrame();

        render_buffer_begin_pass(true, color_blue, false, nullptr);
        render_buffer_end_pass();

        EndRenderFrame();
    }

    ShutdownApplication();
}
