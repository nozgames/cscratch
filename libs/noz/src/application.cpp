//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @traits
static ApplicationTraits g_default_traits = 
{
    .title = "noz",
    .width = 800,
    .height = 600,
    .asset_memory_size = 32 * noz::MB,
    .scratch_memory_size = 8 * noz::MB,
    .renderer = 
    {
        .max_textures = 32,
        .max_shaders = 32,
        .max_samplers = 16,
        .max_pipelines = 64,
        .max_meshes = 256,
        .max_fonts = 8,
        .max_frame_commands = 2048,
        .max_frame_objects = 128,
        .max_frame_transforms = 1024,
        .shadow_map_size = 2048,
    }
};

// @impl
struct Application
{
    SDL_Window* window;
    bool has_focus;
    bool vsync;
    ivec2 screen_size;
    float screen_aspect_ratio;
    const char* title;
    ApplicationTraits traits;
};

static Application g_application = {0};

void Init(ApplicationTraits& traits)
{
    memcpy(&traits, &g_default_traits, sizeof(ApplicationTraits));
}

// @error
void Exit(const char* format, ...)
{
    char buffer[1024];
    
    if (format)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        fprintf(stderr, "error: %s\n", buffer);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_application.title, buffer, NULL);
    }
    else
    {
        fprintf(stderr, "error: unknown error\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_application.title, "unknown error", NULL);
    }
    
    exit(1);
}

void ExitOutOfMemory(const char* message)
{
    if (message)
        Exit("out_of_memory: %s", message);
    else
        Exit("out_of_memory");
}

static void UpdateScreenSize()
{
    int w;
    int h;
    SDL_GetWindowSize(g_application.window, &w, &h);
    g_application.screen_size = { w, h };
    g_application.screen_aspect_ratio = (float)w / (float)h;
}

// @init
void InitApplication(ApplicationTraits* traits)
{
    traits = traits ? traits : &g_default_traits;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) != 1)
        return;

    Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    memset(&g_application, 0, sizeof(Application));
    g_application.title = traits->title;
    g_application.window = SDL_CreateWindow(traits->title, traits->width, traits->height, windowFlags);
    if (!g_application.window)
    {
        SDL_Quit();
        return;
    }

    g_application.traits = *traits;

    UpdateScreenSize();

    InitRenderer(&traits->renderer, g_application.window);
    InitScene();

    if (traits->load_assets)
        traits->load_assets(traits->asset_memory_size);
}

// @shutdown
void ShutdownApplication()
{
    if (g_application.traits.unload_assets)
        g_application.traits.unload_assets();

    ShutdownScene();
    ShutdownRenderer();
}

// @update
bool UpdateApplication()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            return false;

        if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            g_application.has_focus = true;
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            g_application.has_focus = false;
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            UpdateScreenSize();
    }

    return true;
}

// @screen
ivec2 GetScreenSize()
{
    return g_application.screen_size;
}

float GetScreenAspectRatio()
{
    return g_application.screen_aspect_ratio;
}
