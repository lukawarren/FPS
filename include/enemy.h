#pragma once
#include "common.h"
#include "sprite.h"
#include "map.h"

class Enemy
{
public:
    Enemy(const glm::vec3 position);
    void update(const glm::vec3 direction);
    Sprite sprite;

    constexpr static inline float ENEMY_RADIUS = 16 * Map::METRES_PER_UNIT;
    constexpr static inline float ENEMY_HEIGHT = 120 * Map::METRES_PER_UNIT;
};