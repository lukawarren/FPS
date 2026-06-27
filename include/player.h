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
    glm::vec3 position = {};
    JPH::Ref<JPH::CharacterVirtual> character;

    Flashlight flashlight;

    constexpr static inline float PLAYER_HEIGHT     = 56 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_EYE_HEIGHT = 46 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_RADIUS     = 16 * Map::METRES_PER_UNIT;

private:
    void handle_input(
        const Camera& camera,
        const float delta,
        JPH::PhysicsSystem& system,
        JPH::TempAllocator& temp_allocator
    );

    glm::vec2 mouse_position;
    Window* window;
};