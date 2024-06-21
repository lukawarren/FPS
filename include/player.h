#pragma once
#include "pch.h"
#include "camera.h"

class Player
{
public:
    Player();
    void update(csg::world_t& world, const Camera& camera, const float delta);
    void update_camera(Camera& camera) const;

private:
    void handle_input(const Camera& camera, const float delta);
    void handle_physics(csg::world_t& world, const float delta);

    glm::vec3 position = {};
    glm::vec3 velocity = {};
    float head_pitch = 0.0f;
    float head_yaw = 0.0f;
    glm::vec2 mouse_position;
};