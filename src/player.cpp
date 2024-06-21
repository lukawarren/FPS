#include "player.h"
#include "window.h"
#include "config.h"

constexpr float gravity = 9.81f;
constexpr float walk_speed = 10.0f;
constexpr float jump_height = 1.2f;
constexpr float height = 1.72;
constexpr float eye_height = height - 0.1;
constexpr float radius = 0.25f;

const float jump_speed = std::sqrtf(2.0f * gravity * jump_height);

Player::Player()
{
    mouse_position = Window::window->mouse_position();
    position.y = 20;
}

void Player::update(
    csg::world_t& world,
    const Camera& camera,
    const float delta
)
{
    handle_input(camera, delta);
    handle_physics(world, delta);

    ImGui::Begin("player");
    ImGui::Text("Position: (%f, %f, %f)", position.x, position.y, position.z);
    ImGui::End();
}

void Player::handle_input(const Camera& camera, const float delta)
{
    Window& window = *Window::window;

    // WASD
    glm::vec3 movement = {};
    if (window.get_key(GLFW_KEY_W)) movement.z += 1.0f;
    if (window.get_key(GLFW_KEY_S)) movement.z -= 1.0f;
    if (window.get_key(GLFW_KEY_A)) movement.x -= 1.0f;
    if (window.get_key(GLFW_KEY_D)) movement.x += 1.0f;

    // Apply relative to rotation
    velocity.x = 0;
    velocity.z = 0;
    velocity += glm::vec3 { sin(glm::radians(camera.yaw)), 0, -cos(glm::radians(camera.yaw)) } * movement.z * walk_speed;
    velocity += glm::vec3 { cos(glm::radians(camera.yaw)), 0,  sin(glm::radians(camera.yaw)) } * movement.x * walk_speed;

    // Vertical movement
    if (window.get_key(GLFW_KEY_SPACE)) velocity.y = jump_speed;

    // Mouse
    const float sensitivity = 0.1f;
    const glm::vec2 mouse_movement = window.mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;

    // Confine rotation
    head_pitch = std::max(std::min(head_pitch, 90.0f), -90.0f);
}

void Player::handle_physics(csg::world_t& world, const float delta)
{
    // Gravity
    velocity.y -= gravity * delta;

    // Resolve collisions
    auto resolve_axis = [&](const glm::vec3& direction)
    {
        glm::vec3 adjusted_position = position;

        // Adjust origin based on player's size and direction
        if (direction.x > 0) adjusted_position.x += radius;
        if (direction.x < 0) adjusted_position.x -= radius;
        if (direction.z > 0) adjusted_position.z += radius;
        if (direction.z < 0) adjusted_position.z -= radius;
        if (direction.y > 0) adjusted_position.y += height;
        if (direction.y < 0) adjusted_position.y -= 0.0f;

        // Find nearest collision
        const auto hits = world.query_ray(csg::ray_t{
            .origin = {
                adjusted_position.x / metres_per_unit,
                adjusted_position.z / metres_per_unit * -1.0f,
                adjusted_position.y / metres_per_unit
            },
            .direction = {
                direction.x / metres_per_unit,
                direction.z / metres_per_unit * -1.0f,
                direction.y / metres_per_unit
            }
        });
        if (hits.empty()) return;
        const auto& hit = hits[0];

        // Scale to normal size
        const glm::vec3 collision_position = {
            hit.position.x * metres_per_unit,
            hit.position.z * metres_per_unit,
            hit.position.y * metres_per_unit * -1.0f
        };

        // Work out time until collision
        if (direction.x != 0)
        {
            const float distance = collision_position.x - adjusted_position.x;
            const float time = distance / direction.x;
            if (time >= 0 && time <= delta) velocity.x = 0.0f;
        }
        if (direction.y != 0)
        {
            const float distance = collision_position.y - adjusted_position.y;
            const float time = distance / direction.y;
            if (time >= 0 && time <= delta) velocity.y = 0.0f;
        }
        if (direction.z != 0)
        {
            const float distance = collision_position.z - adjusted_position.z;
            const float time = distance / direction.z;
            if (time >= 0 && time <= delta) velocity.z = 0.0f;
        }
    };
    resolve_axis({ velocity.x, 0.0f, 0.0f });
    resolve_axis({ 0.0f, velocity.y, 0.0f });
    resolve_axis({ 0.0f, 0.0f, velocity.z });

    position += velocity * delta;
}

void Player::update_camera(Camera& camera) const
{
    camera.pitch = head_pitch;
    camera.yaw = head_yaw;
    camera.position = {
        position.x,
        position.y + eye_height,
        position.z
    };
}