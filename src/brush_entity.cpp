#include "brush_entity.h"
#include "world.h"

BrushEntity::BrushEntity(
    World& world,
    const glm::vec3 min_bounds,
    const glm::vec3 max_bounds
) : Entity({}), min_bounds(min_bounds), max_bounds(max_bounds)
{}

void BrushEntity::update(World& world, const float delta)
{
    const bool inside = is_inside(world.player->position);
    if (inside && !player_is_inside)
        on_entered(world);

    player_is_inside = inside;
}

void BrushEntity::on_entered(World& world)
{
    (void)world;
}

bool BrushEntity::is_inside(const glm::vec3 position) const
{
    return
        position.x >= min_bounds.x &&
        position.x <= max_bounds.x &&
        position.y >= min_bounds.y &&
        position.y <= max_bounds.y &&
        position.z >= min_bounds.z &&
        position.z <= max_bounds.z;
}
