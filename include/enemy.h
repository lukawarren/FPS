#pragma once
#include "common.h"
#include "sprite.h"
#include "map.h"

class World;

class Enemy
{
public:
    Enemy(const glm::vec3 position);
    void update(World& world, const float delta, const glm::vec3 direction);

    Sprite sprite;

    constexpr static inline float ENEMY_RADIUS = 16 * Map::METRES_PER_UNIT;
    constexpr static inline float ENEMY_HEIGHT = 120 * Map::METRES_PER_UNIT;

private:
    std::vector<glm::vec3> path;
    size_t path_index = 0;
    float repath_timer = 0.0f;
};