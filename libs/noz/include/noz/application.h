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

void application_error(const char* message);    
void application_init(const application_traits* traits);
void application_uninit();
bool application_update();
