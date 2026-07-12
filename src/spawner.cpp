#include "spawner.h"
#include "world.h"
#include "enemy.h"

Spawner::Spawner(
    World& world,
    const glm::vec3 min_bounds,
    const glm::vec3 max_bounds
) : BrushEntity(world, min_bounds, max_bounds)
{}

Spawner::~Spawner() = default;

void Spawner::on_activate(World& world)
{
    const int enemies = 3 + (rand() % 3);

    for (int i = 0; i < enemies; i++)
    {
        glm::vec3 position;
        position.x = (float)rand() / (float)RAND_MAX * (max_bounds.x - min_bounds.x) + min_bounds.x;
        position.z = (float)rand() / (float)RAND_MAX * (max_bounds.z - min_bounds.z) + min_bounds.z;
        position.y = min_bounds.y + Enemy::ENEMY_HEIGHT / 2.0f;

        world.pending_entities.emplace_back(std::make_unique<Enemy>(
            world,
            position
        ));
    }
}
