//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

int main(int argc, char* argv[])
{
    application_init(NULL);
    while (application_update())
    {
        application_begin_render_frame();
        render_buffer_begin_pass(true, color_blue, false, NULL);
        render_buffer_end_pass();
        application_end_render_frame();
    }
    application_uninit();
}
