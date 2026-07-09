#include "player.h"
#include "window.h"
#include "physics.h"
#include "map.h"
#include "world.h"
#include "enemy.h"

static constexpr float MOVE_SPEED       = 7.2f;
static constexpr float JUMP_SPEED       = 10.4f;
static constexpr float GRAVITY          = 32.0f;
static constexpr float ACCEL_RATE       = 12.0f;
static constexpr float AIR_ACCEL_RATE   = 300.0f;
static constexpr float FRICTION         = 6.0f;
static constexpr float STOP_SPEED       = 0.3f * MOVE_SPEED;
static constexpr float AIR_CAP          = 0.3f * MOVE_SPEED;
static constexpr float MAX_SPEED        = 1.5f * MOVE_SPEED;
static constexpr float BOB_FREQUENCY    = 15.0f;
static constexpr float BOB_AMOUNT       = 0.10f;

static constexpr float MAX_RAY_DISTANCE = 100.0f;

static constexpr float HUD_PADDING      = 10.0f;
static constexpr float CROSSHAIR_SIZE   = 10.0f;

Player::Player(
    const glm::vec3 position,
    const float yaw,
    Window& window,
    World& world,
    Audio& audio
) : position(position),
    head_yaw(yaw),
    weapon(Model::ID::WEAPON_5, [&]() { on_fire(); }),
    window(window),
    world(world),
    audio(audio)
{
    mouse_position = window.get_mouse_position();
    window.capture_mouse();

    // Init physics
    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(
        { PLAYER_RADIUS, PLAYER_HEIGHT / 2.0f, PLAYER_RADIUS }
    );
    JPH::CharacterVirtualSettings settings;
    settings.mShape = shape;
    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -PLAYER_RADIUS);
    settings.mMaxSlopeAngle = glm::radians(45.0f);
    settings.mMass = 70.0f;
    settings.mInnerBodyShape = shape;

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
        window.get_mouse_button(SDL_BUTTON_LEFT),
        window.get_mouse_button_pressed(SDL_BUTTON_LEFT),
        delta
    );

    // Update position from physics
    JPH::RVec3 center = character->GetPosition();
    position = {
        center.GetX(),
        center.GetY(),
        center.GetZ()
    };

    update_audio(original_position, grounded_this_frame, delta);
    draw_hud();
}

void Player::damage(const float amount)
{
    health -= amount;
}

void Player::update_audio(const glm::vec3 original_position, const bool grounded_this_frame, const float delta)
{
    if (is_walking() && glm::length2(position - original_position) > 0.001f)
    {
        const float bob_sign = std::sin(bob_time);
        const bool crossed_up = bob_sign >= 0.0f && last_bob_sign < 0.0f;

        if (crossed_up)
        {
            audio.play(
                (Audio::ID)(
                    (u32)Audio::ID::STEPS_BEGIN + (
                        step %
                        ((u32)Audio::ID::STEPS_FINAL - (u32)Audio::ID::STEPS_BEGIN + 1)
                    )
                )
            );
            step++;
        }

        last_bob_sign = bob_sign;
    }

    // Landing sound
    if (!grounded_this_frame && character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround)
        audio.play(Audio::ID::STEPS_BEGIN);
}

void Player::draw_hud() const
{
    auto draw = ImGui::GetBackgroundDrawList();
    const float offset_y = window.size.y - ImGui::GetTextLineHeight() - HUD_PADDING;

    const ImVec2 centre = { window.size.x / 2.0f, window.size.y / 2.0f };

    draw->AddLine(
        { centre.x, centre.y - CROSSHAIR_SIZE + 1.0f },
        { centre.x, centre.y + CROSSHAIR_SIZE },
        IM_COL32_WHITE
    );

    draw->AddLine(
        { centre.x - CROSSHAIR_SIZE, centre.y },
        { centre.x + CROSSHAIR_SIZE, centre.y },
        IM_COL32_WHITE
    );

    const std::string health_text = "Health: " + std::to_string(health);
    draw->AddText({ HUD_PADDING, offset_y }, IM_COL32_WHITE, health_text.c_str());

    ImVec2 ammo_size = ImGui::CalcTextSize("Ammo: 25");
    draw->AddText({ window.size.x - HUD_PADDING - ammo_size.x, offset_y }, IM_COL32_WHITE, "Ammo: 25");
}

glm::vec2 Player::read_movement_input() const
{
    glm::vec2 movement = {};
    if (window.get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window.get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window.get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window.get_key(SDL_SCANCODE_D)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    return movement;
}

bool Player::is_walking() const
{
    return
        character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround && (
        window.get_key(SDL_SCANCODE_W) ||
        window.get_key(SDL_SCANCODE_S) ||
        window.get_key(SDL_SCANCODE_A) ||
        window.get_key(SDL_SCANCODE_D)
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

    vertical_velocity -= GRAVITY * delta;
    if (grounded && window.get_key(SDL_SCANCODE_SPACE))
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
    const glm::vec2 mouse_movement = window.get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;
    head_pitch = std::max(std::min(head_pitch, 89.0f), -89.0f);
}

void Player::update_view_juice(const glm::vec2& movement, const float delta)
{
    const bool grounded = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    const JPH::Vec3 velocity = character->GetLinearVelocity();
    const float speed_ratio = glm::min(glm::length(glm::vec2(velocity.GetX(), velocity.GetZ())) / MOVE_SPEED, 1.0f);

    if (grounded && speed_ratio > 0.01f)
    {
        bob_time += delta * BOB_FREQUENCY * speed_ratio;
        head_bob_offset = std::sin(bob_time) * BOB_AMOUNT * speed_ratio;
    }
    else
    {
        head_bob_offset = glm::mix(head_bob_offset, 0.0f, 1.0f - std::exp(-BOB_FREQUENCY * delta));
        bob_time = 0.0f;
        last_bob_sign = 0.0f;
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

    if (window.get_key_pressed(SDL_SCANCODE_F))
        flashlight.enabled = !flashlight.enabled;
}

void Player::on_fire()
{
    const auto hit = world.get_hit(
        world.camera.position,
        world.camera.direction_vector(),
        MAX_RAY_DISTANCE,
        character->GetInnerBodyID()
    );
    if (!hit.has_value() || !hit->body_id.has_value()) return;

    const JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    JPH::uint64 data = body_interface.GetUserData(hit->body_id.value());

    audio.play(
        Audio::ID::SHOT_HEAVY,
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

