#include "player.h"
#include "window.h"
#include "config.h"

constexpr float gravity = 9.81f;
constexpr float walk_speed = 5.0f;
constexpr float jump_height = 1.2f;
constexpr float height = 1.72;
constexpr float eye_height = height - 0.1;
constexpr float radius = 0.25f;

const float jump_speed = std::sqrtf(2.0f * gravity * jump_height);

Player::Player()
{
    mouse_position = Window::window->mouse_position();
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
    if (window.get_key(GLFW_KEY_SPACE) && grounded)
    {
        velocity.y = jump_speed;
        grounded = false;
    }

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
    // Restrict physics time-scale to minimum of 10 FPS
    const float physics_delta = std::min(delta, 1.0f / 10.0f);

    // Gravity
    velocity.y -= gravity * delta;

    // Resolve collisions
    const int steps = 32;
    const float epsilon = 0.0000001f;
    bool clipped_stairs = false;
    const auto resolve = [&](const glm::vec3 direction)
    {
        const glm::vec3 new_position = position + velocity * direction * physics_delta;
        const csg::box_t bounds = {
            .min = {
                (new_position.x - radius) / metres_per_unit,
                (new_position.z + radius) / metres_per_unit * -1.0f,
                (new_position.y) / metres_per_unit,
            },
            .max = {
                (new_position.x + radius) / metres_per_unit,
                (new_position.z - radius) / metres_per_unit * -1.0f,
                (new_position.y + height) / metres_per_unit,
            }
        };
        const std::vector<csg::brush_t*> collisions = world.query_box(bounds);
        if (collisions.size() != 0)
        {
            if (direction.y == 0.0f)
            {
                // Allow stair clipping
                if (collisions[0]->box.max.z * metres_per_unit - position.y < 16 * metres_per_unit &&
                    !clipped_stairs)
                {
                    position.y = collisions[0]->box.max.z * metres_per_unit + 0.05f;
                    clipped_stairs = true;
                }

                if (direction.x != 0.0f)
                {
                    velocity.x = 0.0f;
                    position.x += velocity.x <= 0.0f ? epsilon : -epsilon;
                }

                if (direction.z != 0.0f)
                {
                    velocity.z = 0.0f;
                    position.z += velocity.z <= 0.0f ? epsilon : -epsilon;
                }
            }

            if (direction.y != 0.0f)
            {
                velocity.y = 0.0f;
                position.y += velocity.y <= 0.0f ? epsilon : -epsilon;
                if (velocity.y <= 0.0f)
                    grounded = true;
            }
        }
    };
    for (int i = 0; i < steps; ++i)
    {
        resolve(glm::vec3 { 1.0f, 0.0f, 0.0f } * (float)i / (float)steps);
        resolve(glm::vec3 { 0.0f, 1.0f, 0.0f } * (float)i / (float)steps);
        resolve(glm::vec3 { 0.0f, 0.0f, 1.0f } * (float)i / (float)steps);
    }

    position += velocity * physics_delta;
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