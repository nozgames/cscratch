/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

typedef struct mesh_builder_impl
{
    mesh_vertex* vertices;
    uint16_t* indices;
    uint32_t vertex_count;
    uint32_t max_vertices;
    uint32_t index_count;
    uint32_t max_indices;
} mesh_builder_impl;

static object_type_t g_mesh_builder_type = nullptr;

void mesh_builder_init()
{
    g_mesh_builder_type = object_type_create("mesh_builder");
}

#if 0
mesh_builder_t mesh_builder_create(int max_vertices, int max_indices)
{
    size_t object_size = sizeof(mesh_vertex) * max_vertices + sizeof(uint16_t) * max_indices;
    object_t object = object_create(mesh_builder_type(), object_size);

    _positions.reserve(initial_size);
    _normals.reserve(initial_size);
    _uv0.reserve(initial_size);
    _bones.reserve(initial_size);
    _indices.reserve(initial_size * 3);
}

void mesh_builder::clear()
{
    _positions.clear();
    _normals.clear();
    _uv0.clear();
    _bones.clear();
    _indices.clear();
}

void mesh_builder::add_vertex(const vec3& position, const vec3& normal, const vec2& uv, uint32_t boneIndex)
{
    _positions.push_back(position);
    _normals.push_back(normal);
    _uv0.push_back(uv);
    _bones.push_back(boneIndex);
}

void mesh_builder::add_index(uint16_t index)
{
    _indices.push_back(index);
}

void mesh_builder::add_triangle(uint16_t a, uint16_t b, uint16_t c)
{
    _indices.push_back(a);
    _indices.push_back(b);
    _indices.push_back(c);
}

void mesh_builder::add_triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, uint32_t boneIndex)
{
    // Calculate face normal
    glm::vec3 v1 = b - a;
    glm::vec3 v2 = c - a;
    glm::vec3 normal = glm::normalize(glm::cross(v2, v1));

    // Add vertices with computed normal
    uint16_t indexA = static_cast<uint16_t>(_positions.size());
    add_vertex(a, normal, glm::vec2(0.0f, 0.0f), boneIndex);

    uint16_t indexB = static_cast<uint16_t>(_positions.size());
    add_vertex(b, normal, glm::vec2(1.0f, 0.0f), boneIndex);

    uint16_t indexC = static_cast<uint16_t>(_positions.size());
    add_vertex(c, normal, glm::vec2(0.5f, 1.0f), boneIndex);

    // Add triangle indices
    add_triangle(indexA, indexB, indexC);
}

void mesh_builder::add_pyramid(const glm::vec3& start, const glm::vec3& end, float size, uint32_t boneIndex)
{
    // Calculate direction and create base
    glm::vec3 direction = glm::normalize(end - start);
    float length = glm::distance(start, end);

    // Create rotation matrix to align with direction
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.9f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));

    auto hsize = size * 0.5f;
    add_triangle(start + right * hsize + up * hsize, start + right * hsize - up * hsize, end, boneIndex);

    add_triangle(start - right * hsize + up * hsize, start + right * hsize + up * hsize, end, boneIndex);

    add_triangle(start - right * hsize - up * hsize, start - right * hsize + up * hsize, end, boneIndex);

    add_triangle(start + right * hsize - up * hsize, start - right * hsize - up * hsize, end, boneIndex);
}

void mesh_builder::add_cube(const glm::vec3& center, const glm::vec3& size, uint32_t boneIndex)
{
    glm::vec3 halfSize = size * 0.5f;

    // Define cube vertices
    std::vector<glm::vec3> vertices = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0: bottom-left-back
        center + glm::vec3(halfSize.x, -halfSize.y, -halfSize.z),  // 1: bottom-right-back
        center + glm::vec3(halfSize.x, halfSize.y, -halfSize.z),   // 2: top-right-back
        center + glm::vec3(-halfSize.x, halfSize.y, -halfSize.z),  // 3: top-left-back
        center + glm::vec3(-halfSize.x, -halfSize.y, halfSize.z),  // 4: bottom-left-front
        center + glm::vec3(halfSize.x, -halfSize.y, halfSize.z),   // 5: bottom-right-front
        center + glm::vec3(halfSize.x, halfSize.y, halfSize.z),    // 6: top-right-front
        center + glm::vec3(-halfSize.x, halfSize.y, halfSize.z)    // 7: top-left-front
    };

    // Add vertices
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());
    for (const auto& vertex : vertices)
    {
        add_vertex(vertex, glm::vec3(0.0f), glm::vec2(0.0f), boneIndex);
    }

    // Define faces (6 faces, each with 2 triangles)
    std::vector<std::vector<uint32_t>> faces = {
        {0, 1, 2, 3}, // Back face
        {5, 4, 7, 6}, // Front face
        {4, 0, 3, 7}, // Left face
        {1, 5, 6, 2}, // Right face
        {3, 2, 6, 7}, // Top face
        {4, 5, 1, 0}  // Bottom face
    };

    // Face normals
    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0.0f, 0.0f, -1.0f), // Back
        glm::vec3(0.0f, 0.0f, 1.0f),  // Front
        glm::vec3(-1.0f, 0.0f, 0.0f), // Left
        glm::vec3(1.0f, 0.0f, 0.0f),  // Right
        glm::vec3(0.0f, 1.0f, 0.0f),  // Top
        glm::vec3(0.0f, -1.0f, 0.0f)  // Bottom
    };

    // Create faces
    for (size_t i = 0; i < faces.size(); ++i)
    {
        const auto& face = faces[i];
        const auto& normal = faceNormals[i];

        // Update normals for this face
        for (uint32_t vertexIndex : face)
        {
            _normals[baseIndex + vertexIndex] = normal;
        }

        // Add triangles (each face is a quad, so 2 triangles)
        add_triangle(baseIndex + face[0], baseIndex + face[1], baseIndex + face[2]);
        add_triangle(baseIndex + face[0], baseIndex + face[2], baseIndex + face[3]);
    }
}

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
        add_quad(glm::vec3(_positions[startIndices[i]]), glm::vec3(_positions[startIndices[next]]),
                 glm::vec3(_positions[endIndices[next]]), glm::vec3(_positions[endIndices[i]]), glm::vec2(0, 0),
                 glm::normalize(glm::cross(_positions[startIndices[next]] - _positions[startIndices[i]],
                                           _positions[endIndices[i]] - _positions[startIndices[i]])),
                 boneIndex);
    }
}

void mesh_builder::add_quad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
                            const glm::vec2& color, const glm::vec3& normal, uint32_t boneIndex)
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

void mesh_builder::add_triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec2& color,
                                const glm::vec3& normal, uint32_t boneIndex)
{
    uint32_t baseIndex = static_cast<uint32_t>(_positions.size());

    // Add vertices
    add_vertex(a, normal, color, boneIndex);
    add_vertex(b, normal, color, boneIndex);
    add_vertex(c, normal, color, boneIndex);

    // Add triangle
    add_triangle(baseIndex, baseIndex + 1, baseIndex + 2);
}

void mesh_builder::add_cylinder(const glm::vec3& start, const glm::vec3& end, float radius, const glm::vec2& colorUV,
                                int segments, uint32_t boneIndex)
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
        // add_triangle(endCenterIndex, endIndices[i], endIndices[next]);
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
        add_quad(glm::vec3(_positions[endIndices[i]]), glm::vec3(_positions[endIndices[next]]),
                 glm::vec3(_positions[startIndices[next]]), glm::vec3(_positions[startIndices[i]]), colorUV, faceNormal,
                 boneIndex);
    }
}

void mesh_builder::add_cone(const glm::vec3& base, const glm::vec3& tip, float baseRadius, const glm::vec2& colorUV,
                            int segments, uint32_t boneIndex)
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
        add_triangle(glm::vec3(_positions[tipIndex]), glm::vec3(_positions[baseIndices[next]]),
                     glm::vec3(_positions[baseIndices[i]]), colorUV, n, boneIndex);
    }
}

mesh mesh_builder::to_mesh(const std::string& name, bool gpu_only)
{
    if (!is_valid())
        return mesh();

    // calculate bounds
    auto min_pos = _positions[0];
    auto max_pos = _positions[0];

    for (const auto& pos : _positions)
    {
        min_pos = glm::min(min_pos, pos);
        max_pos = glm::max(max_pos, pos);
    }

    auto center = (min_pos + max_pos) * 0.5f;
    auto extents = (max_pos - min_pos) * 0.5f;
    auto bounds = bounds3(center, extents);

    return create_mesh(*this, bounds, gpu_only);
}

void mesh_builder::add_mesh(mesh mesh, const vec3& offset)
{
    throw std::runtime_error("not implemented");

#if 0
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
#endif
}
} // namespace noz
#endif