#pragma once
#include "common.h"
#include "brush_entity.h"

class Trigger : public BrushEntity
{
public:
    Trigger(
        World& world,
        const glm::vec3 min_bounds,
        const glm::vec3 max_bounds,
        Entity& entity,
        const int count
    );

    virtual ~Trigger();

protected:
    void on_entered(World& world) override;

private:
    Entity& entity;
    int count;
    int current_count = 0;
};