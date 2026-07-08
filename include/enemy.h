#pragma once
#include "common.h"
#include "entity.h"
#include "map.h"

class World;

class Enemy : public Entity
{
public:
    Enemy(World& world, const glm::vec3 position);
    ~Enemy();

    void update(World& world, const float delta, const glm::vec3 direction) override;
    bool is_dead() const override;

    void damage(const float damage);

    constexpr static inline float ENEMY_RADIUS = 32 * Map::METRES_PER_UNIT;
    constexpr static inline float ENEMY_HEIGHT = 120 * Map::METRES_PER_UNIT;

private:
    enum class State
    {
        Inactive,
        Activated,
        Moving
    } state = State::Inactive;

    std::vector<glm::vec3> path;
    size_t path_index = 0;
    float repath_timer = 0.0f;

    float health = 100.0f;

    glm::vec3 get_player_detect_pos(const World& world) const;
    glm::vec3 get_cover_pos(const World& world) const;
    bool can_see_player_from(const World& world, const glm::vec3 position) const;
};