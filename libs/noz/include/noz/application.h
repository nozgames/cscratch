#pragma once

#include "renderer.h"

typedef struct application_traits
{
    const char* title;
    int width;
    int height;
    renderer_traits renderer;

} application_traits;

void application_init(application_traits* traits);
void application_uninit(void);
bool application_update(void);
void application_begin_render_frame(void);
void application_end_render_frame(void);

void application_error(const char* format, ...);
inline void application_error_out_of_memory() { application_error("out_of_memory"); }

ivec2 screen_size(void);
float screen_aspect_ratio(void);
