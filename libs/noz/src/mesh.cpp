//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct mesh_impl 
{
    OBJECT_BASE;
	name_t name;
    size_t vertex_count;
    size_t index_count;
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    SDL_GPUTransferBuffer* vertex_transfer;
    SDL_GPUTransferBuffer* index_transfer;
    mesh_vertex* vertices;
    uint16_t* indices;
    bounds3 bounds;
};

static map_t* g_mesh_cache = NULL;
static SDL_GPUDevice* g_device = NULL;

static void mesh_upload(mesh_impl* impl);
static void mesh_destroy_impl(mesh_impl* impl);
static inline mesh_impl* to_impl(void* s) { return (mesh_impl*)to_object((object_t*)s, type_mesh); }

inline size_t mesh_impl_size(size_t vertex_count, size_t index_count)
{
    return
        sizeof(mesh_impl) +
        sizeof(mesh_vertex) * vertex_count +
        sizeof(uint16_t) * index_count;
}

static mesh_t* mesh_alloc_internal(allocator_t* allocator, name_t* name, size_t vertex_count, size_t index_count)
{
    mesh_impl* impl = to_impl(object_alloc(allocator, mesh_impl_size(vertex_count, index_count), type_mesh));
    if (!impl)
	    return NULL; 

    impl->vertex_count = vertex_count;
    impl->index_count = index_count;
    impl->vertices = (mesh_vertex*)(impl + sizeof(mesh_impl));
    impl->indices = (uint16_t*)(impl->vertices + sizeof(mesh_vertex) * vertex_count);
	name_copy(&impl->name, name);
    return (mesh_t*)impl;
}

mesh_t* mesh_alloc_raw(
    allocator_t* allocator,
    size_t vertex_count,
    vec3* positions,
    vec3* normals,
    vec2* uvs,
    uint8_t* bone_indices,
    size_t index_count,
    uint16_t* indices,
    name_t* name)
{
    assert(positions);
    assert(normals);
    assert(uvs);
    assert(indices);
    
	mesh_t* mesh = mesh_alloc_internal(allocator, name, vertex_count, index_count);
    mesh_impl* impl = to_impl(mesh);
    impl->bounds = to_bounds(positions, vertex_count);

    if (bone_indices)
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            impl->vertices[i].position = positions[i];
            impl->vertices[i].normal = normals[i];
            impl->vertices[i].uv0 = uvs[i];
            impl->vertices[i].bone = (float)bone_indices[i];
        }
    }
    else
    {
        for (size_t i = 0; i < vertex_count; i++)
        {
            impl->vertices[i].position = positions[i];
            impl->vertices[i].normal = normals[i];
            impl->vertices[i].uv0 = uvs[i];
            impl->vertices[i].bone = 0;
        }
    }

    memcpy(impl->indices, indices, sizeof(uint16_t) * index_count);
    mesh_upload(impl);
    return mesh;
}

static mesh_t* mesh_load_stream(allocator_t* allocator, name_t* name, stream_t* stream)
{
    if (!stream_read_signature(stream, "MESH", 4))
        return NULL;

    // Read bounds
    bounds3 bounds;
    stream_read(stream, &bounds, sizeof(bounds3));

    // counts
    uint32_t vertex_count = stream_read_uint32(stream);
    uint32_t index_count = stream_read_uint32(stream);

	mesh_t* mesh = mesh_alloc_internal(allocator, name, vertex_count, index_count);
    if (!mesh)
		return NULL;

    mesh_impl* impl = to_impl(mesh);
    stream_read(stream, impl->vertices, sizeof(mesh_vertex) * impl->vertex_count);
    stream_read(stream, impl->indices, sizeof(uint16_t) * impl->index_count);
    mesh_upload(impl);

    return mesh;
}

mesh_t* mesh_load(allocator_t* allocator, name_t* name) 
{
    assert(name);
    
    path_t mesh_path;
    asset_path(&mesh_path, name, "mesh");
    stream_t* stream = stream_load_from_file(allocator, &mesh_path);
    if (!stream)
        return NULL;

	mesh_t* mesh = mesh_load_stream(allocator, name, stream);
    object_free(stream);

    if (!mesh)
        return NULL;

	name_copy(&to_impl(mesh)->name, name);

    return mesh;
}

#if 0
static void mesh_destroy_impl(mesh_impl* impl)
{
    if (impl->index_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->index_transfer);

    if (impl->index_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->index_buffer);

    if (impl->vertex_transfer)
        SDL_ReleaseGPUTransferBuffer(g_device, impl->vertex_transfer);

    if (impl->vertex_buffer)
        SDL_ReleaseGPUBuffer(g_device, impl->vertex_buffer);    
}
#endif

void mesh_render(mesh_t* mesh, SDL_GPURenderPass* pass)
{
    assert(pass);

    mesh_impl* impl = to_impl(mesh);
    if (!impl->vertex_buffer)
        return;

    SDL_GPUBufferBinding vertex_binding = {0};
    vertex_binding.buffer = impl->vertex_buffer;
    vertex_binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

    SDL_GPUBufferBinding index_binding = {0};
    index_binding.buffer = impl->index_buffer;
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(pass, (uint32_t)impl->index_count, 1, 0, 0, 0);
}

static void mesh_upload(mesh_impl* impl)
{
    assert(impl);
    assert(!impl->vertex_buffer);
    assert(g_device);

    size_t vertex_count = impl->vertex_count;
    size_t index_count = impl->index_count;

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertex_info = {0};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = (uint32_t)(sizeof(mesh_vertex) * vertex_count);
    vertex_info.props = 0;
    impl->vertex_buffer = SDL_CreateGPUBuffer(g_device, &vertex_info);

    SDL_GPUTransferBufferCreateInfo vertex_transfer_info = {0};
    vertex_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertex_transfer_info.size = vertex_info.size;
    vertex_transfer_info.props = 0;
    impl->vertex_transfer = SDL_CreateGPUTransferBuffer(g_device, &vertex_transfer_info);

    SDL_GPUTransferBufferLocation vertex_source = {impl->vertex_transfer, 0};
    SDL_GPUBufferRegion vertex_dest = {impl->vertex_buffer, 0, vertex_info.size};
    void* vertex_mapped = SDL_MapGPUTransferBuffer(g_device, impl->vertex_transfer, false);

    SDL_memcpy(vertex_mapped, impl->vertices, vertex_info.size);
    SDL_UnmapGPUTransferBuffer(g_device, impl->vertex_transfer);

    SDL_GPUCommandBuffer* vertex_upload_cmd = SDL_AcquireGPUCommandBuffer(g_device);
    SDL_GPUCopyPass* vertex_copy_pass = SDL_BeginGPUCopyPass(vertex_upload_cmd);
    SDL_UploadToGPUBuffer(vertex_copy_pass, &vertex_source, &vertex_dest, false);
    SDL_EndGPUCopyPass(vertex_copy_pass);
    SDL_SubmitGPUCommandBuffer(vertex_upload_cmd);

    // Create index buffer
    SDL_GPUBufferCreateInfo index_info = {0};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = (uint32_t)(sizeof(uint16_t) * index_count);
    index_info.props = 0;
    impl->index_buffer = SDL_CreateGPUBuffer(g_device, &index_info);

    SDL_GPUTransferBufferCreateInfo index_transfer_info = {0};
    index_transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    index_transfer_info.size = index_info.size;
    index_transfer_info.props = 0;
    impl->index_transfer = SDL_CreateGPUTransferBuffer(g_device, &index_transfer_info);

    SDL_GPUTransferBufferLocation index_source = {impl->index_transfer, 0};
    SDL_GPUBufferRegion index_dest = {impl->index_buffer, 0, index_info.size};
    void* index_mapped = SDL_MapGPUTransferBuffer(g_device, impl->index_transfer, false);

    SDL_memcpy(index_mapped, impl->indices, index_info.size);
    SDL_UnmapGPUTransferBuffer(g_device, impl->index_transfer);

    SDL_GPUCommandBuffer* index_upload_cmd = SDL_AcquireGPUCommandBuffer(g_device);
    SDL_GPUCopyPass* index_copy_pass = SDL_BeginGPUCopyPass(index_upload_cmd);
    SDL_UploadToGPUBuffer(index_copy_pass, &index_source, &index_dest, false);
    SDL_EndGPUCopyPass(index_copy_pass);
    SDL_SubmitGPUCommandBuffer(index_upload_cmd);
}

const name_t* mesh_name(mesh_t* mesh)
{
    return &to_impl(mesh)->name;
}

size_t mesh_vertex_count(mesh_t* mesh)
{
	return to_impl(mesh)->vertex_count;
}

size_t mesh_index_count(mesh_t* mesh)
{
    return to_impl(mesh)->index_count;
}

bounds3 mesh_bounds(mesh_t* mesh)
{
    return to_impl(mesh)->bounds;
}

void mesh_init(renderer_traits* traits, SDL_GPUDevice* device)
{
    assert(!g_mesh_cache);
    g_device = device;
    g_mesh_cache = map_alloc(NULL, traits->max_meshes);
}

void mesh_uninit()
{
	assert(g_mesh_cache);
    object_free(g_mesh_cache);
    g_mesh_cache = NULL;
    g_device = NULL;
}

