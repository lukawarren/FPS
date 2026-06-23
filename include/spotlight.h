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

    glm::mat4 get_matrix() const;
};