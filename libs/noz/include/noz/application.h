#pragma once

#include "renderer.h"

struct ApplicationTraits
{
    const char* title;
    int width;
    int height;
    RendererTraits renderer;
};

void InitDefaults(ApplicationTraits* traits);

void InitApplication(ApplicationTraits* traits);
void ShutdownApplication(void);
bool UpdateApplication(void);
void BeginRenderFrame(void);
void EndRenderFrame(void);

void Exit(const char* format, ...);
inline void ExitOutOfMemory() { Exit("out_of_memory"); }

ivec2 GetScreenSize(void);
float GetScreenAspectRatio(void);
