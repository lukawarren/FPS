#include "spotlight.h"

Spotlight::Spotlight(
    const glm::vec3 position,
    const glm::vec3 direction,
    const glm::vec3 colour,
    const float near,
    const float far,
    const float angle
) : position(position), direction(direction), colour(colour),
    near(near), far(far), angle(angle)
{}

glm::mat4 Spotlight::get_matrix() const
{
    const float fov = angle * 2.0f;
    const glm::mat4 projection = glm::perspective(fov, 1.0f, near, far);

    const glm::vec3 target = position + direction;

    // Fallback up-vector if pointing straight up/down
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(direction, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    const glm::mat4 view = glm::lookAt(position, target, up);
    return projection * view;
}

Spotlight::UniformBuffer Spotlight::get_uniform_buffer(const glm::mat4& matrix) const
{
    dbg(colour.x, colour.y, colour.z);
    return {
        .shadow = matrix,
        .position = glm::vec4(position, 0.0f),
        .colour = glm::vec4(colour, 0.0f),
        .direction = glm::vec4(direction, 0.0f),
        .params = { std::cos(angle * 0.85f), std::cos(angle), 0.0f, 0.0f }
    };
}