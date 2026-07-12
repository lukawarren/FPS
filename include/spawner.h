#pragma once
#include "common.h"
#include "brush_entity.h"

class Spawner : public BrushEntity
{
public:
    Spawner(
        World& world,
        const glm::vec3 min_bounds,
        const glm::vec3 max_bounds
    );

    virtual void on_activate(World& world) override;

    virtual ~Spawner();
};