#include "player.h"
#include "window.h"
#include "physics.h"
#include "map.h"

static constexpr float MOVE_SPEED     = 0.12f;
static constexpr float JUMP_SPEED     = 0.14f;
static constexpr float GRAVITY        = 0.008f;
static constexpr float ACCEL_RATE     = 0.2f;
static constexpr float AIR_ACCEL_RATE = 5.0f;
static constexpr float FRICTION       = 0.1f;
static constexpr float STOP_SPEED     = 0.3f * MOVE_SPEED;
static constexpr float AIR_CAP        = 0.3f * MOVE_SPEED;
static constexpr float MAX_SPEED      = 1.5f * MOVE_SPEED;

static constexpr float BOB_FREQUENCY = 0.25f;
static constexpr float BOB_AMOUNT    = 0.10f;

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

glm::vec2 Player::read_movement_input() const
{
    glm::vec2 movement = {};
    if (window->get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window->get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window->get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window->get_key(SDL_SCANCODE_D)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    return movement;
}

static void apply_friction(glm::vec2& velocity, const float delta)
{
    const float speed = glm::length(velocity);
    if (speed < 0.001f)
    {
        velocity = {};
        return;
    }

    const float control = std::max(speed, STOP_SPEED);
    const float drop = control * FRICTION * delta;

    const float new_speed = std::max(speed - drop, 0.0f);
    velocity *= new_speed / speed;
}

static void accelerate(glm::vec2& velocity, const glm::vec2& wishdir, const float wishspeed, const float accel, const float delta)
{
    const float current_speed = glm::dot(velocity, wishdir);
    const float add_speed = wishspeed - current_speed;
    if (add_speed <= 0.0f)
        return;

    const float accel_speed = std::min(accel * wishspeed * delta, add_speed);
    velocity += wishdir * accel_speed;
}

static void air_accelerate(glm::vec2& velocity, const glm::vec2& wishdir, const float wishspeed, const float delta)
{
    accelerate(velocity, wishdir, std::min(wishspeed, AIR_CAP), AIR_ACCEL_RATE, delta);
}

void Player::update_velocity(const glm::vec2& wishdir, const float wishspeed, const float delta)
{
    const bool grounded = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

    const JPH::Vec3 current_velocity = character->GetLinearVelocity();
    glm::vec2 horizontal = { current_velocity.GetX(), current_velocity.GetZ() };
    float vertical_velocity = grounded ? 0.0f : current_velocity.GetY();

    vertical_velocity -= GRAVITY;
    if (grounded && window->get_key(SDL_SCANCODE_SPACE))
        vertical_velocity = JUMP_SPEED;

    if (grounded)
    {
        apply_friction(horizontal, delta);
        accelerate(horizontal, wishdir, wishspeed, ACCEL_RATE, delta);
    }
    else
    {
        air_accelerate(horizontal, wishdir, wishspeed, delta);
    }

    // Cap speed
    const float speed = glm::length(horizontal);
    if (speed > MAX_SPEED)
        horizontal *= MAX_SPEED / speed;

    character->SetLinearVelocity({ horizontal.x, vertical_velocity, horizontal.y });
}

void Player::update_mouse_look()
{
    const float sensitivity = 0.2f;
    const glm::vec2 mouse_movement = window->get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;
    head_pitch = std::max(std::min(head_pitch, 89.0f), -89.0f);
}

void Player::update_view_juice(const glm::vec2& movement, const float delta)
{
    const bool grounded = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    const JPH::Vec3 velocity = character->GetLinearVelocity();
    const float speed_ratio = glm::min(glm::length(glm::vec2(velocity.GetX(), velocity.GetZ())) / MOVE_SPEED, 1.0f);

    // View bob, only while walking on the ground
    if (grounded && speed_ratio > 0.01f)
    {
        bob_time += delta * BOB_FREQUENCY * speed_ratio;
        head_bob_offset = std::sin(bob_time) * BOB_AMOUNT * speed_ratio;
    }
    else
    {
        head_bob_offset = glm::mix(head_bob_offset, 0.0f, 1.0f - std::exp(-BOB_FREQUENCY * delta));
    }
}

void Player::handle_input(
    const Camera& camera,
    const float delta,
    JPH::PhysicsSystem& system,
    JPH::TempAllocator& temp_allocator
)
{
    const glm::vec2 movement = read_movement_input();

    const glm::vec3 forward = { sin(glm::radians(camera.yaw)), 0.0f, -cos(glm::radians(camera.yaw)) };
    const glm::vec3 right   = { cos(glm::radians(camera.yaw)), 0.0f,  sin(glm::radians(camera.yaw)) };

    glm::vec3 wishdir3 = forward * movement.y + right * movement.x;
    if (glm::length(wishdir3) > 0.0001f)
        wishdir3 = glm::normalize(wishdir3);

    update_velocity({ wishdir3.x, wishdir3.z }, MOVE_SPEED, delta);

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

    update_mouse_look();
    update_view_juice(movement, delta);

    if (window->get_key_pressed(SDL_SCANCODE_F))
        flashlight.enabled = !flashlight.enabled;
}