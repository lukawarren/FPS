#pragma once
#include "common.h"
#include "sprite.h"
#include "map.h"

class World;

class Enemy
{
public:
    Enemy(World& world, const glm::vec3 position);
    void update(World& world, const float delta, const glm::vec3 direction);
    void damage(const float damage);

    inline bool is_dead() const { return health <= 0.0f; }

    Sprite sprite;
    JPH::BodyID body_id;

    constexpr static inline float ENEMY_RADIUS = 32 * Map::METRES_PER_UNIT;
    constexpr static inline float ENEMY_HEIGHT = 120 * Map::METRES_PER_UNIT;

private:
    std::vector<glm::vec3> path;
    size_t path_index = 0;
    float repath_timer = 0.0f;
    float health = 100.0f;

    bool can_see_player(const World& world) const;
};