#pragma once
#include "common.h"
#include "entity.h"

class Door : public Entity
{
public:
    Door(World& world, const glm::vec3 position, const glm::vec3 rotation);
    void update(World& world, const float delta) override;

private:
    float slide_amount = 0.0f;
    bool is_open = false;
    bool opening = false;
};