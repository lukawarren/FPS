#include "player.h"
#include "window.h"
#include "physics.h"
#include "map.h"

static constexpr float MOVE_SPEED = 0.1f;
static constexpr float JUMP_SPEED = 0.14f;
static constexpr float GRAVITY = 0.008f;

Player::Player(
    const glm::vec3 position,
    const float yaw,
    Window* window,
    JPH::PhysicsSystem& physics_system
) : position(position), head_yaw(yaw), window(window)
{
    mouse_position = window->get_mouse_position();
    window->capture_mouse();

    // Init physics
    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(
        { PLAYER_RADIUS, PLAYER_HEIGHT / 2.0f, PLAYER_RADIUS }
    );
    JPH::CharacterVirtualSettings settings;
    settings.mShape = shape;
    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -PLAYER_RADIUS);
    settings.mMaxSlopeAngle = glm::radians(45.0f);
    settings.mMass = 70.0f;

    character = new JPH::CharacterVirtual(
        &settings,
        { position.x, position.y, position.z },
        JPH::Quat::sIdentity(),
        0,
        &physics_system
    );
}

void Player::update(
    const Camera& camera,
    const float delta,
    JPH::PhysicsSystem& system,
    JPH::TempAllocator& temp_allocator
)
{
    handle_input(camera, delta, system, temp_allocator);
    flashlight.update(position, camera.pitch, camera.yaw);

    JPH::RVec3 center = character->GetPosition();
    position = {
        center.GetX(),
        center.GetY(),
        center.GetZ()
    };
}

void Player::handle_input(
    const Camera& camera,
    const float delta,
    JPH::PhysicsSystem& system,
    JPH::TempAllocator& temp_allocator
)
{
    glm::vec2 movement = {};
    if (window->get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window->get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window->get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window->get_key(SDL_SCANCODE_D)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    const glm::vec3 forward = { sin(glm::radians(camera.yaw)), 0.0f, -cos(glm::radians(camera.yaw)) };
    const glm::vec3 right   = { cos(glm::radians(camera.yaw)), 0.0f,  sin(glm::radians(camera.yaw)) };
    const glm::vec3 wishdir = (forward * movement.y + right * movement.x) * MOVE_SPEED;

    const bool grounded = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

    JPH::Vec3 current_velocity = character->GetLinearVelocity();
    float vertical_velocity = grounded ? 0.0f : current_velocity.GetY();

    // Gravity
    vertical_velocity -= GRAVITY;
    if (grounded && window->get_key(SDL_SCANCODE_SPACE))
        vertical_velocity = JUMP_SPEED;

    character->SetLinearVelocity({ wishdir.x, vertical_velocity, wishdir.z });

    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    character->ExtendedUpdate(
        delta,
        { 0.0f, GRAVITY, 0.0f },
        update_settings,
        system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        system.GetDefaultLayerFilter(Layers::MOVING),
        {},
        {},
        temp_allocator
    );

    // Mouse Look
    const float sensitivity = 0.2f;
    const glm::vec2 mouse_movement = window->get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;
    head_pitch = std::max(std::min(head_pitch, 89.0f), -89.0f);

    if (window->get_key_pressed(SDL_SCANCODE_F))
        flashlight.enabled = !flashlight.enabled;
}