#pragma once
#include "common.h"
#include "entity.h"

class Door : public Entity
{
public:
    Door(
        World& world,
        const glm::vec3 position,
        const glm::vec3 rotation,
        const bool is_big
    );
    void update(World& world, const float delta) override;

private:
    float slide_amount = 0.0f;
    bool is_open = false;
    bool opening = false;
    bool is_big;
};