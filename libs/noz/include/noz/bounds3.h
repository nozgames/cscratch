/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

typedef struct bounds3
{
    vec3_t min;
    vec3_t max;
} bounds3_t;

GMATH_INLINE bounds3_t bounds3_from_points(const vec3_t* positions, size_t count)
{
    bounds3_t b;
    if (count == 0)
    {
        b.min = (vec3_t){0.0f, 0.0f, 0.0f};
        b.max = (vec3_t){0.0f, 0.0f, 0.0f};
        return b;
    }
    b.min = positions[0];
    b.max = positions[0];
    for (size_t i = 1; i < count; ++i)
    {
        b.min.x = fminf(b.min.x, positions[i].x);
        b.min.y = fminf(b.min.y, positions[i].y);
        b.min.z = fminf(b.min.z, positions[i].z);
        b.max.x = fmaxf(b.max.x, positions[i].x);
        b.max.y = fmaxf(b.max.y, positions[i].y);
        b.max.z = fmaxf(b.max.z, positions[i].z);
    }
    return b;
}

GMATH_INLINE vec3_t bounds3_center(bounds3_t bounds)
{
    return (vec3_t){
        (bounds.min.x + bounds.max.x) * 0.5f,
        (bounds.min.y + bounds.max.y) * 0.5f,
        (bounds.min.z + bounds.max.z) * 0.5f
	};
}

GMATH_INLINE vec3_t bounds3_size(bounds3_t bounds)
{
    return (vec3_t){
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z
    };
}

#if 0

    // Calculate bounds from vertex positions
    static bounds3_t from_vertices(const std::vector<glm::vec3>& positions);

    // Getters
    const glm::vec3& min() const
    {
        return _min;
    }
    const glm::vec3& max() const
    {
        return _max;
    }
    glm::vec3& min()
    {
        return _min;
    }
    glm::vec3& max()
    {
        return _max;
    }
    glm::vec3 center() const
    {
        return (_min + _max) * 0.5f;
    }
    glm::vec3 size() const
    {
        return _max - _min;
    }
    glm::vec3 extents() const
    {
        return (_max - _min) * 0.5f;
    }

    // Check if bounds are valid (min <= max for all components)
    bool valid() const;

    // Check if a point is inside the bounds
    bool contains(const glm::vec3& point) const;

    // Check if bounds intersect with another bounds
    bool intersects(const bounds3_t& other) const;

    // Expand bounds to include a point
    void expand(const glm::vec3& point);

    // Expand bounds to include another bounds
    void expand(const bounds3_t& other);

    // Transform bounds by a matrix
    bounds3_t transform(const mat4& matrix) const;

    // Get the volume of the bounds
    float volume() const;

    // Get the surface area of the bounds
    float surface_area() const;

  private:
    glm::vec3 _min = glm::vec3(std::numeric_limits<float>::infinity());
    glm::vec3 _max = glm::vec3(-std::numeric_limits<float>::infinity());
};

} // namespace noz

#endif