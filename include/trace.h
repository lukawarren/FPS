#pragma once
#include "common.h"
#include "entity.h"
#include "map.h"

class World;

class Trace : public Entity
{
public:
    Trace(
        World& world,
        const glm::vec3 position,
        const glm::vec3 destination,
        const Sprite::ID id,
        const bool is_enemy
    );
    void update(World& world, const float delta) override;
    bool is_dead() const override;

private:
    glm::vec3 direction;
    float time = 0.0f;
};