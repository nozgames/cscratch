#pragma once

#include "renderer.h"

struct ApplicationTraits
{
    const char* title;
    int width;
    int height;
    size_t scratch_size;
    RendererTraits renderer;
};

void InitDefaults(ApplicationTraits* traits);

void InitApplication(ApplicationTraits* traits);
void ShutdownApplication();
bool UpdateApplication();
void BeginRenderFrame();
void EndRenderFrame();

void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

ivec2 GetScreenSize();
float GetScreenAspectRatio();
