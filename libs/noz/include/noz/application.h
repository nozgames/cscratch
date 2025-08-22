#pragma once

#include "renderer.h"

typedef struct application_traits
{
    const char* title;
    int width;
    int height;
    renderer_traits renderer;

} application_traits;

void application_traits_init_defaults(application_traits* traits);

void application_init(const application_traits* traits);
void application_uninit();
bool application_update();
void application_begin_render_frame();
void application_end_render_frame();

void application_error(const char* message);
inline void application_error_out_of_memory() { application_error("out_of_memory"); }

ivec2_t screen_size();
float screen_aspect_ratio();
