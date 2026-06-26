#pragma once
#include "common.h"
#include "camera.h"
#include "window.h"
#include "flashlight.h"

class Player
{
public:
    Player(Window* window);
    void update(Camera& camera, const float delta);

    void set_position(const glm::vec3& pos);

    float head_pitch = 0.0f;
    float head_yaw = 0.0f;

    Flashlight flashlight;

private:
    void handle_input(const Camera& camera, const float delta);

    glm::vec3 position = {};
    glm::vec2 mouse_position;

    Window* window;
};