#pragma once
#include "common.h"

class Camera
{
public:
    Camera() {}

    Camera(glm::vec3 position, float pitch, float yaw, float roll, float fov) :
        position(position), pitch(pitch), yaw(yaw), roll(roll), fov(fov) {}

    glm::mat4 view_matrix() const
    {
        glm::mat4 view = glm::mat4(1.0f);

        // Rotation
        view = glm::rotate(view, glm::radians(pitch), glm::vec3(1, 0, 0));
        view = glm::rotate(view, glm::radians(yaw),   glm::vec3(0, 1, 0));
        view = glm::rotate(view, glm::radians(roll),  glm::vec3(0, 0, 1));

        // Translation
        view = glm::translate(view, -position);
        return view;
    }

    glm::mat4 projection_matrix(
        const float width,
        const float height,
        const float near = Z_NEAR,
        const float far = Z_FAR
    ) const
    {
        return glm::perspective(fov, width / height, near, far);
    }

    glm::vec3 direction_vector() const
    {
        glm::vec3 direction;

        const float p = glm::radians(pitch);
        const float y = glm::radians(yaw);

        direction.x = std::cos(p) * std::sin(y);
        direction.y = -std::sin(p);
        direction.z = -std::cos(p) * std::cos(y);

        return glm::normalize(direction);
    }

    glm::vec3 right_vector() const
    {
        glm::vec3 direction;

        const float p = glm::radians(pitch);
        const float y = glm::radians(yaw + 90.0f);

        direction.x = std::cos(p) * std::sin(y);
        direction.y = -std::sin(p);
        direction.z = -std::cos(p) * std::cos(y);

        return glm::normalize(direction);
    }

    glm::vec3 up_vector() const
    {
        glm::vec3 direction;

        const float p = glm::radians(pitch - 90.0f);
        const float y = glm::radians(yaw);

        direction.x = std::cos(p) * std::sin(y);
        direction.y = -std::sin(p);
        direction.z = -std::cos(p) * std::cos(y);

        return glm::normalize(direction);
    }

    glm::vec3 position = {};
    glm::vec3 velocity = {};
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    float fov = glm::radians(100.0f);

    static inline float Z_NEAR = 0.01f;
    static inline float Z_FAR = 1000.0f;
};