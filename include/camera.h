#pragma once
#include "pch.h"
#include "window.h"

constexpr float z_near = 0.01f;
constexpr float z_far = 1000.0f;
constexpr float fov = glm::radians(90.0f);

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

    void update_freecam(const float delta)
    {
        Window& window = *Window::window;

        // WASD
        const float speed = 10.0f * delta;
        glm::vec3 movement = {};
        if (window.get_key(GLFW_KEY_W)) movement.z += 1.0f;
        if (window.get_key(GLFW_KEY_S)) movement.z -= 1.0f;
        if (window.get_key(GLFW_KEY_A)) movement.x -= 1.0f;
        if (window.get_key(GLFW_KEY_D)) movement.x += 1.0f;

        // Apply relative to rotation
        position += glm::vec3 { sin(glm::radians(yaw)), 0, -cos(glm::radians(yaw)) } * movement.z * speed;
        position += glm::vec3 { cos(glm::radians(yaw)), 0,  sin(glm::radians(yaw)) } * movement.x * speed;

        // Vertical movement
        if (window.get_key(GLFW_KEY_SPACE)) position.y += 1.0f * speed;
        if (window.get_key(GLFW_KEY_LEFT_SHIFT)) position.y -= 1.0f * speed;

        // Mouse
        const float sensitivity = 0.1f;
        const glm::vec2 mouse_movement = window.mouse_movement();
        yaw += mouse_movement.x * sensitivity;
        pitch += mouse_movement.y * sensitivity;

        // Confine rotation
        pitch = std::max(std::min(pitch, 90.0f), -90.0f);
    }

    glm::vec3 position = {};
    glm::vec3 velocity = {};
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
};