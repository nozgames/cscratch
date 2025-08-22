/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

typedef struct mesh_builder_impl
{
    vec3_t* positions;
    vec3_t* normals;
    vec2_t* uv0;
    uint8_t* bones;
    uint16_t* indices;
    size_t vertex_count;
    size_t vertex_max;
    size_t index_count;
    size_t index_max;
    bool is_full;
} mesh_builder_impl_t;

static object_type_t g_mesh_builder_type = NULL;

static inline mesh_builder_impl_t* to_impl(mesh_builder_t builder)
{
    assert(builder);
    return (mesh_builder_impl_t*)object_impl((object_t)builder, g_mesh_builder_type);
}

mesh_builder_t mesh_builder_create(int max_vertices, int max_indices)
{
    object_t object = object_create(g_mesh_builder_type, sizeof(mesh_builder_impl_t));
    if (!object)
        return NULL;
    
	mesh_builder_t builder = (mesh_builder_t)object;
    mesh_builder_impl_t* impl = to_impl(builder);
    
    impl->vertex_max = max_vertices;
    impl->index_max = max_indices;
    impl->vertex_count = 0;
    impl->index_count = 0;
    
    // Allocate arrays
    impl->positions = malloc(sizeof(vec3_t) * max_vertices);
    impl->normals = malloc(sizeof(vec3_t) * max_vertices);
    impl->uv0 = malloc(sizeof(vec2_t) * max_vertices);
    impl->bones = malloc(sizeof(uint8_t) * max_vertices);
    impl->indices = malloc(sizeof(uint16_t) * max_indices);
    
    if (!impl->positions || !impl->normals || !impl->uv0 || !impl->bones || !impl->indices) {
        free(impl->positions);
        free(impl->normals);
        free(impl->uv0);
        free(impl->bones);
        free(impl->indices);
        object_destroy(object);
        return NULL;
    }
    
    return builder;
}

void mesh_builder_destroy(mesh_builder_t builder)
{
    if (!builder) return;
    
    mesh_builder_impl_t* impl = to_impl(builder);    
    free(impl->positions);
    free(impl->normals);
    free(impl->uv0);
    free(impl->bones);
    free(impl->indices);
    
    object_destroy((object_t)builder);
}

void mesh_builder_clear(mesh_builder_t builder)
{
    if (!builder) return;
    
    mesh_builder_impl_t* impl = to_impl(builder);
    impl->vertex_count = 0;
    impl->index_count = 0;
}

const vec3_t* mesh_builder_positions(mesh_builder_t builder)
{
    return to_impl(builder)->positions;
}

const vec3_t* mesh_builder_normals(mesh_builder_t builder)
{
    return to_impl(builder)->normals;
}

const vec2_t* mesh_builder_uv0(mesh_builder_t builder)
{
    return to_impl(builder)->uv0;
}

const uint8_t* mesh_builder_bones(mesh_builder_t builder)
{
    return to_impl(builder)->bones;
}

const uint16_t* mesh_builder_indices(mesh_builder_t builder)
{
    return to_impl(builder)->indices;
}

size_t mesh_builder_vertex_count(mesh_builder_t builder)
{
    return to_impl(builder)->vertex_count;
}

size_t mesh_builder_index_count(mesh_builder_t builder)
{
    return to_impl(builder)->index_count;
}

void mesh_builder_init()
{
    g_mesh_builder_type = object_type_create("mesh_builder");
}

void mesh_builder_add_vertex(
	mesh_builder_t builder,
    vec3_t position,
    vec3_t normal,
    vec2_t uv,
    uint8_t bone_index)
{
    mesh_builder_impl_t* impl = to_impl(builder);
    impl->is_full = impl->is_full && impl->vertex_count + 1 >= impl->vertex_max;
    if (impl->is_full)
        return;

    size_t index = impl->vertex_count;
    impl->vertex_count++;
	impl->positions[index] = position;
	impl->normals[index] = normal;
	impl->uv0[index] = uv;
	impl->bones[index] = bone_index;
}

void mesh_builder_add_index(mesh_builder_t builder, uint16_t index)
{
    mesh_builder_impl_t* impl = to_impl(builder);
    impl->is_full = impl->is_full && impl->index_count + 1 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count] = index;
    impl->index_count++;    
}

void mesh_builder_add_triangle_indices(mesh_builder_t builder, uint16_t a, uint16_t b, uint16_t c)
{
    mesh_builder_impl_t* impl = to_impl(builder);
    impl->is_full = impl->is_full && impl->index_count + 3 >= impl->index_max;
    if (impl->is_full)
        return;

    impl->indices[impl->index_count + 0] = a;
    impl->indices[impl->index_count + 1] = b;
    impl->indices[impl->index_count + 2] = c;
    impl->index_count+=3;
}

void mesh_builder_add_triangle(
    mesh_builder_t builder,
    vec3_t a,
    vec3_t b,
    vec3_t c,
    uint8_t bone_index)
{
    // Calculate face normal
    vec3_t v1 = vec3_sub(b, a);
    vec3_t v2 = vec3_sub(c, a);
    vec3_t normal = vec3_normalize(vec3_cross(v2, v1));

    // Add vertices with computed normal
    auto vertex_index = to_impl(builder)->vertex_count;
	add_vertex(a, normal, (vec2_t) { 0.0f, 0.0f }, bone_index);
    add_vertex(a, normal, (vec2_t) { 1.0f, 0.0f }, bone_index);
    add_vertex(a, normal, (vec2_t) { 0.5f, 1.0f }, bone_index);
    mesh_builder_add_triangle_indices(builder, vertex_index, vertex_index + 1, vertex_index + 2);
}

void mesh_builder_add_pyramid(vec3_t start, vec3_t end, float size, uint8_t bone_index)
{
    // Calculate direction and create base
    vec3_t direction = vec3_normalize(vec3_sub(end, start));
    float length = vec3_distance(start, end);

    // Create rotation matrix to align with direction
    vec3_t up = { 0.0f, 1.0f, 0.0f };
    if (fabs(vec3_dot(direction, up)) > 0.9f)
        up = (vec3_t){ 1.0f, 0.0f, 0.0f };

    vec3_t right = vec3_normalize(vec3_cross(direction, up));
    up = vec3_normalize(vec3_cross(right, direction));

    auto hsize = size * 0.5f;
	right = vec3_muls(right, hsize);
	up = vec3_muls(up, hsize);

	vec3_t right_sub_up = vec3_sub(right, up);
	vec3_t right_add_up = vec3_add(right, up);

    add_triangle(
        vec3_add(start, right_add_up),
        vec3_add(start, right_sub_up),
        end,
        bone_index);

    add_triangle(
		vec3_sub(start, right_add_up),
        vec3_add(start, right_add_up),
        end,
        bone_index);

    add_triangle(
        vec3_sub(start, right_sub_up),
        vec3_sub(start, right_add_up),
        end,
        bone_index);

    add_triangle(
        vec3_add(start, right_sub_up),
        vec3_sub(start, right_sub_up),
        end,
        bone_index);
}

void mesh_builder_add_raw(
    mesh_builder_t builder,
    size_t vertex_count,
    vec3_t* positions,
    vec3_t* normals,
    vec2_t* uv0,
    uint8_t bone_index,
    size_t index_count,
    uint16_t* indices)
{
    mesh_builder_impl_t* impl = to_impl(builder);
	impl->is_full = impl->is_full && (impl->vertex_count + vertex_count >= impl->vertex_max || impl->index_count + index_count >= impl->index_max);
    if (impl->is_full)
        return;

	size_t vertex_start = impl->vertex_count;
	memcpy(impl->positions + impl->vertex_count, positions, sizeof(vec3_t) * vertex_count);
	memcpy(impl->normals + impl->vertex_count, normals, sizeof(vec3_t) * vertex_count);
	memcpy(impl->uv0 + impl->vertex_count, uv0, sizeof(vec2_t) * vertex_count);

    for (size_t i = 0; i < vertex_count; ++i)
		impl->bones[vertex_start + i] = bone_index;

    for (size_t i = 0; i < index_count; ++i)
    {
        impl->indices[impl->index_count] = indices[i] + (uint16_t)vertex_start;
        impl->index_count++;
	}
}

void mesh_builder_add_cube(mesh_builder_t builder, vec3_t center, vec3_t size, uint8_t bone_index)
{
	mesh_builder_impl_t* impl = to_impl(builder);

    vec3_t half_size = vec3_muls(size, 0.5f);

    vec3_t positions[8] = {
        vec3_add(center, (vec3_t) { -half_size.x, -half_size.y, -half_size.z }), // 0: bottom-left-back
        vec3_add(center, (vec3_t) { half_size.x, -half_size.y, -half_size.z }), // 1: bottom-right-back
        vec3_add(center, (vec3_t) { half_size.x,  half_size.y, -half_size.z }), // 2: top-right-back
        vec3_add(center, (vec3_t) { -half_size.x,  half_size.y, -half_size.z }), // 3: top-left-back
        vec3_add(center, (vec3_t) { -half_size.x, -half_size.y,  half_size.z }), // 4: bottom-left-front
        vec3_add(center, (vec3_t) { half_size.x, -half_size.y,  half_size.z }), // 5: bottom-right-front
        vec3_add(center, (vec3_t) { half_size.x,  half_size.y,  half_size.z }), // 6: top-right-front
        vec3_add(center, (vec3_t) { -half_size.x,  half_size.y,  half_size.z })  // 7: top-left-front
    };

    vec3_t normals[8] = {
        (vec3_t) { 0.0f,  0.0f, -1.0f }, // Back face
        (vec3_t) { 0.0f,  0.0f, -1.0f },
        (vec3_t) { 0.0f,  0.0f, -1.0f },
        (vec3_t) { 0.0f,  0.0f, -1.0f },
        (vec3_t) { 0.0f,  0.0f,  1.0f }, // Front face
        (vec3_t) { 0.0f,  0.0f,  1.0f },
        (vec3_t) { 0.0f,  0.0f,  1.0f },
		(vec3_t) { 0.0f,  0.0f,  1.0f }
    };

    vec3_t uvs[8] = {
        (vec3_t) { 0.0f, 0.0f, 0.0f },
        (vec3_t) { 1.0f, 0.0f, 0.0f },
        (vec3_t) { 1.0f, 1.0f, 0.0f },
        (vec3_t) { 0.0f, 1.0f, 0.0f },
        (vec3_t) { 0.0f, 0.0f, 0.0f },
        (vec3_t) { 1.0f, 0.0f, 0.0f },
        (vec3_t) { 1.0f, 1.0f, 0.0f },
		(vec3_t) { 0.0f, 1.0f, 0.0f }
	};

    uint16_t indices[36] = {
        0, 1, 2, 0, 2, 3, // Back face
        5, 4, 7, 5, 7, 6, // Front face
        4, 0, 3, 4, 3, 7, // Left face
        1, 5, 6, 1, 6, 2, // Right face
        3, 2, 6, 3, 6, 7, // Top face
		4, 5, 1, 4, 1, 0  // Bottom face
    };
}

#if 0

void mesh_builder::add_sphere(const glm::vec3& center, float radius, int segments, int rings, uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Generate sphere vertices
    for (int ring = 0; ring <= rings; ++ring)
    {
        float phi = (glm::pi<float>() * ring) / rings;
        float y = cos(phi);
        float ringRadius = sin(phi);

        for (int segment = 0; segment <= segments; ++segment)
        {
            float theta = (2.0f * glm::pi<float>() * segment) / segments;
            float x = cos(theta) * ringRadius;
            float z = sin(theta) * ringRadius;

            glm::vec3 position = center + glm::vec3(x, y, z) * radius;
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            glm::vec2 uv = glm::vec2(static_cast<float>(segment) / segments, static_cast<float>(ring) / rings);

            add_vertex(position, normal, uv, boneIndex);
        }
    }

    // Generate sphere indices
    for (int ring = 0; ring < rings; ++ring)
    {
        for (int segment = 0; segment < segments; ++segment)
        {
            int current = baseIndex + ring * (segments + 1) + segment;
            int next = current + 1;
            int below = current + (segments + 1);
            int belowNext = below + 1;

            // Two triangles per quad
            add_triangle(current, below, next);
            add_triangle(next, below, belowNext);
        }
    }
}

void mesh_builder::add_line(const glm::vec3& start, const glm::vec3& end, float thickness, uint32_t boneIndex)
{
    // Create a simple line as a thin cylinder
    glm::vec3 direction = glm::normalize(end - start);
    float length = glm::distance(start, end);

    // Create rotation matrix
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    // Create thin cylinder with 4 segments
    const int segments = 4;
    std::vector<uint32_t> startIndices, endIndices;

    // Add start vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::vec3 offset = right * cos(angle) + up * sin(angle);
        glm::vec3 position = start + offset * thickness;
        glm::vec3 normal = glm::normalize(offset);

        startIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(position, normal, glm::vec2(0.0f, static_cast<float>(i) / segments), boneIndex);
    }

    // Add end vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::vec3 offset = right * cos(angle) + up * sin(angle);
        glm::vec3 position = end + offset * thickness;
        glm::vec3 normal = glm::normalize(offset);

        endIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(position, normal, glm::vec2(1.0f, static_cast<float>(i) / segments), boneIndex);
    }

    // Create side faces
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Add quad for side face
        add_quad(
            glm::vec3(_positions[startIndices[i]]),
            glm::vec3(_positions[startIndices[next]]),
            glm::vec3(_positions[endIndices[next]]),
            glm::vec3(_positions[endIndices[i]]),
            glm::vec2(0, 0),
            glm::normalize(glm::cross(
                _positions[startIndices[next]] - _positions[startIndices[i]],
                _positions[endIndices[i]] - _positions[startIndices[i]]
            )),
            boneIndex
        );
    }
}

void mesh_builder::add_quad(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& d,
    const glm::vec2& color,
    const glm::vec3& normal,
    uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add vertices
    add_vertex(a, normal, color, boneIndex);
    add_vertex(b, normal, color, boneIndex);
    add_vertex(c, normal, color, boneIndex);
    add_vertex(d, normal, color, boneIndex);

    // Add triangles
    add_triangle(baseIndex, baseIndex + 1, baseIndex + 2);
    add_triangle(baseIndex, baseIndex + 2, baseIndex + 3);
}

void mesh_builder::add_quad(const vec3& forward, const vec3& right, float width, float height, const vec2& color_uv)
{
    auto hw = width * 0.5f;
    auto hh = height * 0.5f;
    auto normal = glm::cross(forward, right);
    auto a = right * -hw + forward * hh;
    auto b = right * hw + forward * hh;
    auto c = right * hw + forward * -hh;
    auto d = right * -hw + forward * -hh;
    add_quad(a, b, c, d, color_uv, normal, 0);
}

void mesh_builder::add_triangle(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec2& color,
    const glm::vec3& normal,
    uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add vertices
    add_vertex(a, normal, color, boneIndex);
    add_vertex(b, normal, color, boneIndex);
    add_vertex(c, normal, color, boneIndex);

    // Add triangle
    add_triangle(baseIndex, baseIndex + 1, baseIndex + 2);
}

void mesh_builder::add_cylinder(const glm::vec3& start, const glm::vec3& end, float radius, const glm::vec2& colorUV, int segments, uint32_t boneIndex)
{
    glm::vec3 direction = glm::normalize(end - start);
    float length = glm::distance(start, end);

    // Create rotation matrix to align with direction
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Generate vertices for start and end circles
    std::vector<uint32_t> startIndices, endIndices;

    // Add center vertices for caps
    uint32_t startCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(start, -direction, colorUV, boneIndex);

    uint32_t endCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(end, direction, colorUV, boneIndex);

    // Generate circle vertices
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::vec3 offset = right * cos(angle) + up * sin(angle);
        glm::vec3 normal = glm::normalize(offset);

        // Start circle vertex
        glm::vec3 startPos = start + offset * radius;
        startIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(startPos, -direction, colorUV, boneIndex);

        // End circle vertex
        glm::vec3 endPos = end + offset * radius;
        endIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(endPos, direction, colorUV, boneIndex);
    }

    // Create start cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        add_triangle(startIndices[i], startIndices[next], startCenterIndex);
    }

    // Create end cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        //add_triangle(endCenterIndex, endIndices[i], endIndices[next]);
    }

    // Create side faces
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Calculate normal for this face
        glm::vec3 v1 = glm::vec3(_positions[startIndices[next]]) - glm::vec3(_positions[startIndices[i]]);
        glm::vec3 v2 = glm::vec3(_positions[endIndices[i]]) - glm::vec3(_positions[startIndices[i]]);
        glm::vec3 faceNormal = -glm::normalize(glm::cross(v1, v2));

        // Add quad for side face
        add_quad(
            glm::vec3(_positions[endIndices[i]]),
            glm::vec3(_positions[endIndices[next]]),
            glm::vec3(_positions[startIndices[next]]),
            glm::vec3(_positions[startIndices[i]]),
            colorUV,
            faceNormal,
            boneIndex
        );
    }
}

void mesh_builder::add_cone(
    const glm::vec3& base,
    const glm::vec3& tip,
    float baseRadius,
    const glm::vec2& colorUV,
    int segments,
    uint32_t boneIndex)
{
    glm::vec3 direction = glm::normalize(tip - base);
    float length = glm::distance(base, tip);

    // Create rotation matrix to align with direction
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add base center vertex
    uint32_t baseCenterIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(base, -direction, colorUV, boneIndex);

    // Add tip vertex
    uint32_t tipIndex = static_cast<uint32_t>(_positions.size());
    add_vertex(tip, direction, colorUV, boneIndex);

    // Generate base circle vertices
    std::vector<uint32_t> baseIndices;
    for (int i = 0; i < segments; ++i)
    {
        float angle = (2.0f * glm::pi<float>() * i) / segments;
        glm::vec3 offset = right * cos(angle) + up * sin(angle);
        glm::vec3 basePos = base + offset * baseRadius;

        baseIndices.push_back(static_cast<uint32_t>(_positions.size()));
        add_vertex(basePos, -direction, colorUV, boneIndex);
    }

    // Create base cap triangles
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;
        add_triangle(baseIndices[i], baseIndices[next], baseCenterIndex);
    }

    // Create side triangles (from base to tip)
    for (int i = 0; i < segments; ++i)
    {
        int next = (i + 1) % segments;

        // Calculate normal for this face
        auto v1 = glm::vec3(_positions[baseIndices[next]]) - glm::vec3(_positions[baseIndices[i]]);
        auto v2 = glm::vec3(_positions[tipIndex]) - glm::vec3(_positions[baseIndices[i]]);
        auto n = -glm::normalize(glm::cross(v1, v2));

        // Add triangle
        add_triangle(
            glm::vec3(_positions[tipIndex]),
            glm::vec3(_positions[baseIndices[next]]),
            glm::vec3(_positions[baseIndices[i]]),
            colorUV,
            n,
            boneIndex
        );
    }
}
#endif

mesh_t mesh_builder_to_mesh(mesh_builder_t builder, const char* name)
{
    assert(builder);
	mesh_builder_impl_t* impl = to_impl(builder);
    return mesh_create_raw(
        builder,
        name,
		impl->vertex_count,
        impl->positions,
        impl->normals,
        impl->uv0,
        impl->bones,
        impl->index_count,
        impl->indices
	);
}

#if 0
void mesh_builder::add_mesh(mesh mesh, const vec3& offset)
{
    throw std::runtime_error("not implemented");

    assert(mesh);

    auto vertex_count = mesh->vertex_count();

    _positions.reserve(_positions.size() + mesh->positions().size());
    for (const auto& pos : mesh->positions())
        _positions.push_back(pos + offset);

    _normals.insert(_normals.end(), mesh->normals().begin(), mesh->normals().end());
    _uv0.insert(_uv0.end(), mesh->uv0().begin(), mesh->uv0().end());
    _bones.insert(_bones.end(), mesh->boneIndices().begin(), mesh->boneIndices().end());

    // Adjust indices to account for existing vertices
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size() - mesh->positions().size());
    for (const auto& index : mesh->indices())
        _indices.push_back(static_cast<uint16_t>(index + baseIndex));
}
#endif
