#pragma once
#include "common.h"
#include "camera.h"
#include "window.h"
#include "flashlight.h"
#include "weapon.h"
#include "map.h"

class World;

class Player
{
public:
    Player(
        const glm::vec3 position,
        const float yaw,
        Window* window,
        World& world
    );

    void update(World& world, const float delta);

    float head_pitch = 0.0f;
    float head_yaw = 0.0f;
    float head_bob_offset = 0.0f;

    glm::vec3 position = {};
    JPH::Ref<JPH::CharacterVirtual> character;
    Flashlight flashlight;
    Weapon weapon;

    constexpr static inline float PLAYER_HEIGHT     = 72 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_EYE_HEIGHT = 64 * Map::METRES_PER_UNIT;
    constexpr static inline float PLAYER_RADIUS     = 16 * Map::METRES_PER_UNIT;

private:
    void handle_input(World& world, const float delta);
    void on_fire(World& world);

    glm::vec2 read_movement_input() const;
    void update_velocity(const glm::vec2& wishdir, const float wishspeed, const float delta);
    void update_mouse_look();
    void update_view_juice(const glm::vec2& movement, const float delta);

    struct Hit
    {
        glm::vec3 position;
        glm::vec3 normal;
        std::optional<JPH::BodyID> body_id;
    };

    std::optional<Hit> get_hit(const World& world) const;

    float bob_time = 0.0f;
    glm::vec2 mouse_position;
    Window* window;
};