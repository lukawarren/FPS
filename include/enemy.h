#pragma once
#include "common.h"
#include "entity.h"
#include "map.h"
#include "hit.h"

class World;

class Enemy : public Entity
{
public:
    Enemy(World& world, const glm::vec3 position);
    ~Enemy();

    void update(World& world, const float delta) override;
    bool is_dead() const override;

    void damage(const float damage);

    constexpr static inline float ENEMY_RADIUS = 16 * Map::METRES_PER_UNIT;
    constexpr static inline float ENEMY_HEIGHT = 72 * Map::METRES_PER_UNIT;

private:
    enum class State
    {
        Inactive,
        Activated,
        Moving,
        Shooting
    } state = State::Inactive;
#
    std::vector<glm::vec3> path;
    size_t path_index = 0;
    float repath_timer = 0.0f;

    float health = 100.0f;

    float animation_time = 0.0f;
    u32 animation_frame = 0;

    void think(World& world, const float delta);
    void animate(World& world, const float delta);
    void shoot(World& world);
    glm::vec3 get_cover_pos(const World& world) const;
    std::optional<Hit> get_hit_from(
        const World& world,
        const glm::vec3 position,
        const bool aim_for_head
    ) const;
    bool did_hit_player(const World& world, const Hit& hit) const;
};