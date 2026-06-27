#pragma once
#include "common.h"
#include "camera.h"
#include "window.h"
#include "flashlight.h"
#include "map.h"

class Player
{
public:
    Player(
        const glm::vec3 position,
        const float yaw,
        Window* window,
        JPH::PhysicsSystem& physics_system
    );

    void update(
        const Camera& camera,
        const float delta,
        JPH::PhysicsSystem& system,
        JPH::TempAllocator& temp_allocator
    );

    float head_pitch = 0.0f;
    float head_yaw = 0.0f;
    float head_bob_offset = 0.0f;

    glm::vec3 position = {};
    Flashlight flashlight;
    JPH::Ref<JPH::CharacterVirtual> character;

    constexpr static inline float PLAYER_HEIGHT     = 72 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_EYE_HEIGHT = 64 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_RADIUS     = 16 * Map::METRES_PER_UNIT;

private:
    void handle_input(
        const Camera& camera,
        const float delta,
        JPH::PhysicsSystem& system,
        JPH::TempAllocator& temp_allocator
    );

    glm::vec2 read_movement_input() const;
    void update_velocity(const glm::vec2& wishdir, const float wishspeed, const float delta);
    void update_mouse_look();
    void update_view_juice(const glm::vec2& movement, const float delta);

    float bob_time = 0.0f;
    glm::vec2 mouse_position;
    Window* window;
};