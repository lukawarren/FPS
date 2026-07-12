#include "trigger.h"
#include "world.h"

Trigger::Trigger(
    World& world,
    const glm::vec3 min_bounds,
    const glm::vec3 max_bounds,
    Entity& entity,
    const int count
) : BrushEntity(world, min_bounds, max_bounds), entity(entity), count(count)
{}

Trigger::~Trigger() = default;

void Trigger::on_entered(World& world)
{
    current_count++;
    if (current_count != count) return;
    entity.on_activate(world);
}
