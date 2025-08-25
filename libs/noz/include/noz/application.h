#pragma once

#include "renderer.h"

struct ApplicationTraits
{
    const char* title;
    int width;
    int height;
    size_t asset_memory_size;
    size_t scratch_memory_size;
    RendererTraits renderer;
    bool (*load_assets)(size_t size);
    void (*unload_assets)();
};

void Init(ApplicationTraits& traits);

void InitApplication(ApplicationTraits* traits);
void ShutdownApplication();
bool UpdateApplication();
void BeginRenderFrame();
void EndRenderFrame();

void Exit(const char* format, ...);
void ExitOutOfMemory(const char* message=nullptr);

ivec2 GetScreenSize();
float GetScreenAspectRatio();
