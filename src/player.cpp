#include "player.h"
#include "window.h"
#include "physics.h"
#include "map.h"
#include "world.h"

static constexpr float MOVE_SPEED       = 7.2f;
static constexpr float JUMP_SPEED       = 8.4f;
static constexpr float GRAVITY          = 0.48f;
static constexpr float ACCEL_RATE       = 12.0f;
static constexpr float AIR_ACCEL_RATE   = 300.0f;
static constexpr float FRICTION         = 6.0f;
static constexpr float STOP_SPEED       = 0.3f * MOVE_SPEED;
static constexpr float AIR_CAP          = 0.3f * MOVE_SPEED;
static constexpr float MAX_SPEED        = 1.5f * MOVE_SPEED;
static constexpr float BOB_FREQUENCY    = 15.0f;
static constexpr float BOB_AMOUNT       = 0.10f;

static constexpr float MAX_RAY_DISTANCE = 100.0f;

static constexpr float STEP_DELAY       = 0.4f;

Player::Player(
    const glm::vec3 position,
    const float yaw,
    Window* window,
    World& world,
    Audio& audio
) : position(position),
    head_yaw(yaw),
    weapon(Model::ID::WEAPON_5, [&]() { on_fire(); }),
    window(window),
    world(world),
    audio(audio)
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
        &world.physics_system
    );
}

void Player::update(const float delta)
{
    const bool grounded_this_frame =
        character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

    const glm::vec3 original_position = position;

    handle_input(delta);
    flashlight.update(position, world.camera.pitch, world.camera.yaw);
    weapon.update(
        window->get_mouse_button(SDL_BUTTON_LEFT),
        window->get_mouse_button_pressed(SDL_BUTTON_LEFT),
        delta
    );

    // Update position from physics
    JPH::RVec3 center = character->GetPosition();
    position = {
        center.GetX(),
        center.GetY(),
        center.GetZ()
    };

    if (window->get_key_pressed(SDL_SCANCODE_K))
        character->SetPosition(character->GetPosition() + JPH::RVec3(0.0f, 100.0f, 0.0f));

    // Walking sounds
    if (is_walking())
    {
        if (step_time <= 0.0f)
        {
            if (glm::length2(position - original_position) > 0.001f)
                audio.play(
                    (Audio::ID)(
                        (u32)Audio::ID::STEPS_BEGIN + (
                            step %
                            ((u32)Audio::ID::STEPS_FINAL - (u32)Audio::ID::STEPS_BEGIN + 1)
                        )
                    )
                );
            step_time = STEP_DELAY + delta;
            step++;
        }

        step_time -= delta;
    }
    else step_time = 0.0f;

    // Landing sound
    if (!grounded_this_frame && character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround)
        audio.play(Audio::ID::STEPS_BEGIN);
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

bool Player::is_walking() const
{
    return
        character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && (
        window->get_key(SDL_SCANCODE_W) ||
        window->get_key(SDL_SCANCODE_S) ||
        window->get_key(SDL_SCANCODE_A) ||
        window->get_key(SDL_SCANCODE_D)
    );
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

void Player::handle_input(const float delta)
{
    const glm::vec2 movement = read_movement_input();

    const glm::vec3 forward = { sin(glm::radians(world.camera.yaw)), 0.0f, -cos(glm::radians(world.camera.yaw)) };
    const glm::vec3 right   = { cos(glm::radians(world.camera.yaw)), 0.0f,  sin(glm::radians(world.camera.yaw)) };

    glm::vec3 wishdir = forward * movement.y + right * movement.x;
    const float length = glm::length(wishdir);
    if (length > 0.0001f) wishdir = glm::normalize(wishdir);

    update_velocity({ wishdir.x, wishdir.z }, MOVE_SPEED, delta);

    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    character->ExtendedUpdate(
        delta,
        { 0.0f, GRAVITY, 0.0f },
        update_settings,
        world.physics_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        world.physics_system.GetDefaultLayerFilter(Layers::MOVING),
        {},
        {},
        world.allocator
    );

    update_mouse_look();
    update_view_juice(movement, delta);

    if (window->get_key_pressed(SDL_SCANCODE_F))
        flashlight.enabled = !flashlight.enabled;
}

void Player::on_fire()
{
    const auto hit = get_hit();
    if (!hit.has_value() || !hit->body_id.has_value()) return;

    const JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    JPH::uint64 data = body_interface.GetUserData(hit->body_id.value());

    audio.play(
        Audio::ID::SHOOT,
        0.9f + (rand() % 200) / 1000.0f
    );

    if (data == 0)
    {
        // Hit map
        world.spawn_decal(
            hit->position,
            hit->normal
        );
    }
    else
    {
        Enemy* enemy = (Enemy*)data;
        enemy->damage(Weapon::DAMAGE);
    }
}

std::optional<Player::Hit> Player::get_hit() const
{
    const glm::vec3 forward = world.camera.direction_vector();
    JPH::RVec3 start = {
        world.camera.position.x + forward.x,
        world.camera.position.y + forward.y,
        world.camera.position.z + forward.z
    };

    JPH::RVec3 direction = JPH::RVec3 { forward.x, forward.y, forward.z } * MAX_RAY_DISTANCE;
    JPH::RRayCast ray { start, direction };

    JPH::RayCastSettings settings;
    settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    world.physics_system.GetNarrowPhaseQuery().CastRay(ray, settings, collector);

    if (collector.HadHit())
    {
        // Sort hits to get the closest one if necessary
        collector.Sort();

        const JPH::RayCastResult& hit = collector.mHits[0];
        JPH::Vec3 position = start + direction * hit.mFraction;
        std::optional<JPH::BodyID> body_id = std::nullopt;

        // Get surface normal
        JPH::BodyLockRead lock(world.physics_system.GetBodyLockInterface(), hit.mBodyID);
        JPH::Vec3 jolt_normal = JPH::Vec3::sAxisY();
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            jolt_normal = body.GetShape()->GetSurfaceNormal(
                hit.mSubShapeID2,
                ray.GetPointOnRay(hit.mFraction)
            );
            jolt_normal = body.GetWorldTransform().Multiply3x3(jolt_normal);
            body_id = body.GetID();
        }

        return std::optional<Hit>(Hit {
            .position = glm::vec3(
                position.GetX(),
                position.GetY(),
                position.GetZ()
            ),
            .normal = glm::vec3(
                jolt_normal.GetX(),
                jolt_normal.GetY(),
                jolt_normal.GetZ()
            ),
            .body_id = body_id
        });
    }

    return std::nullopt;
}