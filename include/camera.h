#pragma once
#include "common.h"

constexpr inline float z_near = 0.01f;
constexpr inline float z_far = 1000.0f;
constexpr inline float fov = glm::radians(90.0f);

class Camera
{
public:
    Camera() {}

    Camera(glm::vec3 _position, float _pitch, float _yaw, float _roll) :
        position(_position), pitch(_pitch), yaw(_yaw), roll(_roll) {}

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
        const float field_of_view = fov,
        const float near = z_near,
        const float far = z_far
    ) const
    {
        return glm::perspective(field_of_view, width / height, near, far);
    }

    glm::vec3 direction_vector() const
    {
        // Eye coordinates at centre of screen
        glm::vec4 eye = { 0.0f, 0.0f, -1.0f, 0.0f };

        // World coordinates
        glm::vec4 world_ray_xyzw = glm::inverse(view_matrix()) * eye;
        glm::vec3 world_ray_xyz = { world_ray_xyzw.x, world_ray_xyzw.y, world_ray_xyzw.z };
        return glm::normalize(world_ray_xyz);
    }

    glm::vec3 position = {};
    glm::vec3 velocity = {};
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};