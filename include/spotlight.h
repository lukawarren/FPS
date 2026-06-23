#pragma once
#include "common.h"
#include "camera.h"

class Spotlight
{
public:
    Spotlight(
        const glm::vec3 position,
        const glm::vec3 direction,
        const glm::vec3 colour,
        const float near,
        const float far,
        const float angle
    );

    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 colour;
    float near;
    float far;
    float angle;

    // TODO: pack near and far into vec4's with position, etc.
    struct alignas(16) UniformBuffer
    {
        glm::mat4 shadow;
        glm::vec4 position;
        glm::vec4 colour;
        glm::vec4 direction;
        glm::vec4 params; // inner_cutoff, outer_cutoff
    };

    glm::mat4 get_matrix() const;
    UniformBuffer get_uniform_buffer(const glm::mat4& matrix) const;
};