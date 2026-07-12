#pragma once
#include "common.h"
#include "entity.h"

class BrushEntity : public Entity
{
public:
    BrushEntity(
        World& world,
        const glm::vec3 min_bounds,
        const glm::vec3 max_bounds
    );

    void update(World& world, const float delta) override;

protected:
    virtual void on_entered(World& world);
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;

private:
    bool is_inside(const glm::vec3 position) const;
    bool player_is_inside = false;
};