//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @forward
void update_screen_size();

// @traits
static application_traits g_default_traits = 
{
	.title = "noz",
	.width = 800,
	.height = 600,
	.renderer = 
	{
		.max_textures = 32,
        .max_shaders = 32,
        .max_meshes = 256,
        .max_fonts = 8,
        .shadow_map_size = 2048,
        .max_frame_commands = 2048,
        .max_frame_objects = 128,
        .max_frame_transforms = 1024,
        .max_samplers = 16
	}
};

// @impl
typedef struct application_impl
{
    SDL_Window* window;
    bool has_focus;
    bool vsync;
    ivec2_t screen_size;
    float screen_aspect_ratio;
    const char* title;

} application_impl;

static application_impl g_application = {0};

void application_traits_init_defaults(application_traits* traits)
{
    assert(traits);
    memcpy(traits, &g_default_traits, sizeof(application_traits));
}

// @error
void application_error(const char* error)
{
    if (error)
    {
        fprintf(stderr, "error: %s\n", error);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_application.title, error, NULL);
    }
    else
    {
        fprintf(stderr, "error: unknown error\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, g_application.title, "unknown error", NULL);
    }
    
    exit(1);
}

// @init
void application_init(const application_traits* traits)
{
    if (!traits)
		traits = &g_default_traits;

    g_application.title = traits->title;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) != 1)
        return;

	Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	memset(&g_application, 0, sizeof(application_impl));
    g_application.window = SDL_CreateWindow(traits->title, traits->width, traits->height, windowFlags);
    if (!g_application.window)
    {
        SDL_Quit();
        return;
    }

	object_init();
    object_pool_init();
    map_init();
    stream_init();
	renderer_init(&traits->renderer, g_application.window);
}

// @uninit
void application_uninit()
{
	renderer_uninit();
    stream_uninit();
    map_uninit();
    object_pool_uninit();
    object_uninit();
}

// @update
bool application_update()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            return false;

        if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
        {
            bool previousFocus = g_application.has_focus;
            g_application.has_focus = true;
        }
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            bool previousFocus = g_application.has_focus;
            g_application.has_focus = false;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            update_screen_size();
    }

    return true;
}

void update_screen_size()
{
    int w;
    int h;
    SDL_GetWindowSize(g_application.window, &w, &h);
    g_application.screen_size = (ivec2_t){w, h};
    g_application.screen_aspect_ratio = (float)w / (float)h;
}
