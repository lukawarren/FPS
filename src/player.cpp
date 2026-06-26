#include "player.h"
#include "window.h"
#include "map.h"

// Player dimensions (metres)
constexpr float PLAYER_HEIGHT     = 1.4f;
constexpr float PLAYER_EYE_HEIGHT = PLAYER_HEIGHT - 0.1f;
constexpr float PLAYER_RADIUS     = 0.35f;

Player::Player(Window* window) : window(window)
{
    mouse_position = window->get_mouse_position();
    window->capture_mouse();
    position.y = 1.0f;
}

void Player::update(Camera& camera, const float delta)
{
    handle_input(camera, delta);
    flashlight.update(position, camera.pitch, camera.yaw);

    camera.pitch = head_pitch;
    camera.yaw = head_yaw;
    camera.position = {
        position.x,
        position.y + PLAYER_EYE_HEIGHT / 2.0f,
        position.z
    };
}

void Player::handle_input(const Camera& camera, const float delta)
{
    const float freecam_speed = 5.0f;

    glm::vec2 movement = {};
    if (window->get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window->get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window->get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window->get_key(SDL_SCANCODE_D)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    const float pitch_rad = glm::radians(-camera.pitch);
    const float yaw_rad = glm::radians(-camera.yaw);

    const glm::vec3 forward = {
        -std::sin(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        -std::cos(yaw_rad) * std::cos(pitch_rad)
    };

    const glm::vec3 right = {
        std::cos(yaw_rad),
        0.0f,
        -std::sin(yaw_rad)
    };

    const glm::vec3 up = {
        std::sin(yaw_rad) * std::sin(pitch_rad),
        std::cos(pitch_rad),
        std::cos(yaw_rad) * std::sin(pitch_rad)
    };

    // Apply movement
    const float speed = freecam_speed * delta;
    position += forward * movement.y * speed;
    position += right * movement.x * speed;

    // Vertical movement
    if (window->get_key(SDL_SCANCODE_Q)) position -= up * speed;
    if (window->get_key(SDL_SCANCODE_E)) position += up * speed;

    // Mouse Look
    const float sensitivity = 0.2f;
    const glm::vec2 mouse_movement = window->get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;
    head_pitch = std::max(std::min(head_pitch, 89.0f), -89.0f);

    // Cursor Toggle
    if (window->get_key_pressed(SDL_SCANCODE_LSHIFT))
    {
        static bool mouse_captured = true;
        mouse_captured = !mouse_captured;
        if (mouse_captured)
            window->capture_mouse();
        else
            window->uncapture_mouse();
    }

    // Flashlight
    if (window->get_key_pressed(SDL_SCANCODE_F))
        flashlight.enabled = !flashlight.enabled;
}

void Player::set_position(const glm::vec3& pos)
{
    position = pos;
    position.y += PLAYER_RADIUS;
}