//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: we can build a single bone buffer to upload and just add bone_offset to the model buffer for each mesh

typedef enum command_type
{
    command_type_bind_material,
    command_type_bind_light,
    command_type_bind_transform,
    command_type_bind_camera,
    command_type_bind_bones,
    command_type_bind_default_texture,
    command_type_bind_color,
    command_type_set_viewport,
    command_type_set_scissor,
    command_type_draw_mesh,
    command_type_begin_pass,
    command_type_begin_shadow_pass,
    command_type_begin_gamma_pass,
    command_type_end_pass,
} command_type_t;

typedef struct bind_material
{
    material_t material;
} bind_material_t;

typedef struct bind_transform
{
    mat4_t transform;
} bind_transform_t;

typedef struct bind_camera
{
    mat4_t view;
    mat4_t projection;
    mat4_t view_projection;
    mat4_t light_view_projection;
} bind_camera_t;

typedef struct bind_bones
{
    size_t count;
    size_t offset;
} bind_bones_t;

typedef struct bind_light
{
    vec3_t ambient_color;
    float ambient_intensity;
    vec3_t diffuse_color;
    float diffuse_intensity;
    vec3_t direction;
    float shadow_bias;
} bind_light_t;

typedef struct set_viewport
{
    SDL_GPUViewport gpu_viewport;
} set_viewport_t;

typedef struct set_scissor
{
    SDL_Rect rect;
} set_scissor_t;

typedef struct bind_color
{
    color_t color;
} bind_color_t;

typedef struct set_text_options
{
    vec4_t color;
    vec4_t outline_color;
    float outline_width;
    float smoothing;
    float padding1;
    float padding2;
} set_text_options_t;

typedef struct draw_mesh
{
    mesh_t mesh;
} draw_mesh_t;

typedef struct begin_pass
{
    bool clear;
    color_t color;
    bool msaa;
    texture_t target;
} begin_pass_t;

typedef struct bind_default_texture
{
    int index;
} bind_default_texture_t;

typedef struct command
{
    command_type_t type;
    union 
    {
        bind_material_t bind_material;
        bind_transform_t bind_transform;
        bind_camera_t bind_camera;
        bind_light_t bind_light;
        bind_bones_t bind_bones;
        bind_color_t bind_color;
        bind_default_texture_t bind_default_texture;
        set_viewport_t set_viewport;
        set_scissor_t set_scissor;
        begin_pass_t begin_pass;
        draw_mesh_t draw_mesh;
    } data;
} command_t;

typedef struct render_buffer_impl
{
    command_t* commands;
    size_t command_count;
    mat4_t* transforms;
    size_t transform_count;
    size_t command_count_max;
    size_t transform_count_max;
    bool is_shadow_pass;
    bool is_full;
} render_buffer_impl_t;

static object_type_t g_render_buffer_type = nullptr;
static render_buffer_impl_t* g_render_buffer = nullptr;

static inline void render_buffer_add_command(const command_t* cmd)
{
    // don't add the command if we are full
    if (g_render_buffer->is_full)
        return;

    g_render_buffer->commands[g_render_buffer->command_count++] = *cmd;
    g_render_buffer->is_full = g_render_buffer->command_count == g_render_buffer->command_count_max;
}

void render_buffer_clear()
{
    g_render_buffer->command_count = 0;
    g_render_buffer->transform_count = 0;
    g_render_buffer->is_shadow_pass = false;
	g_render_buffer->is_full = false;
    
    // add identity transform by default for all meshes with no bones
    static mat4_t identity = {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
	mat4_identity(&identity);
    g_render_buffer->transforms[0] = identity;
    g_render_buffer->transform_count = 1;
}

void render_buffer_begin_pass(bool clear, color_t clear_color, bool msaa, texture_t target)
{
    command_t cmd = {
        .type = command_type_begin_pass,
        .data = {
            .begin_pass = {
                .clear = clear,
                .color = clear_color,
                .msaa = msaa, target = target}}};
	render_buffer_add_command(&cmd);
}

void render_buffer_begin_shadow_pass(mat4_t light_view, mat4_t light_projection)
{
    command_t cmd = {.type = command_type_begin_shadow_pass};
    render_buffer_add_command(&cmd);
}

void render_buffer_end_pass()
{
    command_t cmd = { .type = command_type_end_pass };
    render_buffer_add_command(&cmd);
}

void render_buffer_begin_gamma_pass()
{
    command_t cmd = { .type = command_type_begin_gamma_pass };
    render_buffer_add_command(&cmd);
}

void render_buffer_bind_default_texture(int texture_index)
{
    command_t cmd = {
        .type = command_type_bind_default_texture,
        .data = {
            .bind_default_texture = {
				.index = texture_index}} };
    render_buffer_add_command(&cmd);
}

void render_buffer_bind_camera(camera_t camera)
{
    assert(camera);
    render_buffer_bind_camera_matrices(entity_world_to_local((entity_t)camera), camera_projection(camera));
}

void render_buffer_bind_camera_matrices(mat4_t view, mat4_t projection)
{
    mat4_t view_projection = mat4_mul(projection, view);
    command_t cmd = {
        .type = command_type_bind_camera,
        .data = {
            .bind_camera = {
                .view=view,
                .projection=projection,
                .view_projection=view_projection,
                .light_view_projection=view_projection}}};
    render_buffer_add_command(&cmd);
}

void render_buffer_bind_material(const material_t material)
{
    assert(material);

    command_t cmd = {
        .type = command_type_bind_material,
        .data = {
            .bind_material = {
                .material = material}} };

    render_buffer_add_command(&cmd);
}

void render_buffer_bind_transform(mat4_t transform)
{
    command_t cmd = {
        .type = command_type_bind_transform,
        .data = {
            .bind_transform = {
				.transform = transform}} };
	render_buffer_add_command(&cmd);
}

//void render_buffer_bind_transform(render_buffer_t rb, const entity& entity)
//{
//    rb.impl()->commands.emplace_back(command_type_bind_transform, command::bind_transform{get_local_to_world(entity)});
//}

void render_buffer_bind_bones(const mat4_t* bones, size_t bone_count)
{
    if (bone_count == 0)
        return;

    if (g_render_buffer->command_count + bone_count > g_render_buffer->transform_count_max)
    {
        g_render_buffer->is_full = true;
        return;
    }   

    command_t cmd = {
        .type = command_type_bind_bones,
        .data = {
            .bind_bones = {
                .count = bone_count,
                .offset = g_render_buffer->transform_count}} };

    memcpy(
        g_render_buffer->transforms + g_render_buffer->transform_count,
		bones,
        bone_count * sizeof(mat4_t));
    g_render_buffer->transform_count += bone_count;

    render_buffer_add_command(&cmd);
}

void render_buffer_bind_color(color_t color)
{
    command_t cmd = {
        .type = command_type_bind_color,
        .data = {
            .bind_color = {
				.color = {color.r, color.g, color.b, color.a}}} };
    render_buffer_add_command(&cmd);
}

void render_buffer_render_mesh(mesh_t mesh)
{
    assert(mesh);
    command_t cmd = {
        .type = command_type_draw_mesh,
        .data = {
            .draw_mesh = {
				.mesh = mesh}} };
    render_buffer_add_command(&cmd);
}

void render_buffer_execute(SDL_GPUCommandBuffer* cb)
{
    SDL_GPURenderPass* pass = nullptr;

    const command_t* commands = g_render_buffer->commands;
	size_t command_count = g_render_buffer->command_count;
    for (size_t command_index=0; command_index < command_count; ++command_index)
    {
		const command_t* command = commands + command_index;
        switch (command->type)
        {
        case command_type_bind_material:
            renderer_bind_material(command->data.bind_material.material);
            break;

        case command_type_bind_transform:
            SDL_PushGPUVertexUniformData(cb, vertex_register_object, &command->data, sizeof(bind_transform_t));
            break;

        case command_type_bind_camera:
        {
            SDL_PushGPUVertexUniformData(cb, vertex_register_camera, &command->data, sizeof(bind_camera_t));

            // Store for legacy compatibility (still needed by bindTransform for ObjectBuffer)
            //_view = data.view;
            //_view_projection = data.view_projection;

            // if (_shadowPassActive)
            //     _lightViewProjectionMatrix = _view_projection;

            break;
        }

        case command_type_bind_bones:
            SDL_PushGPUVertexUniformData(
                cb,
                vertex_register_bone,
                g_render_buffer->transforms + command->data.bind_bones.offset,
                command->data.bind_bones.count);
            break;

        case command_type_bind_light:
            SDL_PushGPUFragmentUniformData(cb, fragment_register_light, &command->data, sizeof(bind_light_t));
            break;

        case command_type_bind_color:
            SDL_PushGPUFragmentUniformData(cb, fragment_register_color, &command->data, sizeof(bind_color_t));
            break;

        case command_type_draw_mesh:
            mesh_render(command->data.draw_mesh.mesh, pass);
            break;

        case command_type_begin_pass:
            pass = renderer_begin_pass(
                command->data.begin_pass.clear,
                command->data.begin_pass.color,
                command->data.begin_pass.msaa,
                command->data.begin_pass.target);
            break;

        case command_type_bind_default_texture:
            renderer_bind_default_texture(command->data.bind_default_texture.index);
            break;

        case command_type_begin_gamma_pass:
            pass = renderer_begin_gamma_pass();
            break;

        case command_type_end_pass:
            end_renderer_pass();
            pass = nullptr;
            break;

        case command_type_begin_shadow_pass:
            pass = renderer_begin_shadow_pass();
            break;

        case command_type_set_viewport:
        {
            SDL_SetGPUViewport(pass, &command->data.set_viewport.gpu_viewport);
            break;
        }

        case command_type_set_scissor:
            SDL_SetGPUScissor(pass, &command->data.set_scissor.rect);
            break;
        }
    }
}

#if 0




    void render_buffer::bind_light(
        const vec3& direction,
        float ambient_intensity,
        const vec3& ambient_color,
        float diffuse_intensity,
        const vec3& diffuse_color,
        float shadow_bias)
    {
		impl()->commands.emplace_back
        (
            command_type_bind_light,
            command::bind_light
            {
                ambient_color,
                ambient_intensity,
                diffuse_color,
                diffuse_intensity,
                direction,
                shadow_bias
			});
    }





    void render_buffer::set_viewport(int x, int y, int width, int height)
    {
		impl()->commands.emplace_back(command_type_set_viewport, command::set_viewport { x, y, width, height });
    }

    void render_buffer::set_scissor(int x, int y, int width, int height)
    {
        impl()->commands.emplace_back(command_type_set_scissor, command::set_scissor { x, y, width, height });
    }

#endif

void render_buffer_init(const renderer_traits* traits)
{
    g_render_buffer_type = object_type_create("render_buffer");

    size_t commands_size = traits->max_frame_commands * sizeof(command_t);
	size_t transforms_size = traits->max_frame_transforms * sizeof(mat4_t);
    size_t buffer_size = sizeof(render_buffer_impl_t) + commands_size + transforms_size;
    
    g_render_buffer = (render_buffer_impl_t*)malloc(buffer_size);
    if (!g_render_buffer)
    {
        application_error_out_of_memory();
        return;
    }        

	memset(g_render_buffer, 0, buffer_size);
	g_render_buffer->commands = (command_t*)((char*)g_render_buffer + sizeof(render_buffer_impl_t));
    g_render_buffer->transforms = (mat4_t*)((char*)g_render_buffer->commands + commands_size);
    g_render_buffer->command_count_max = traits->max_frame_commands;
    g_render_buffer->transform_count_max = traits->max_frame_transforms;
}

void render_buffer_uninit()
{
    assert(g_render_buffer);
    free(g_render_buffer);
    g_render_buffer = nullptr;
}
