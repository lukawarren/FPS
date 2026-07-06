#pragma once
#include "common.h"
#include "camera.h"
#include "window.h"
#include "flashlight.h"
#include "weapon.h"
#include "map.h"
#include "audio.h"

class World;

class Player
{
public:
    Player(
        const glm::vec3 position,
        const float yaw,
        Window& window,
        World& world,
        Audio& audio
    );

    void update(const float delta);

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
    void handle_input(const float delta);
    void update_audio(const glm::vec3 original_position, const bool grounded_this_frame, const float delta);
    void draw_hud() const;
    void on_fire();

    glm::vec2 read_movement_input() const;
    bool is_walking() const;
    void update_velocity(const glm::vec2& wishdir, const float wishspeed, const float delta);
    void update_mouse_look();
    void update_view_juice(const glm::vec2& movement, const float delta);

    u32 step = 0;
    float last_bob_sign = 0.0f;
    float bob_time = 0.0f;
    glm::vec2 mouse_position;

    World& world;
    Audio& audio;
    Window& window;
};