#include "enemy.h"
#include "player.h"
#include "world.h"

constexpr static inline float SPEED = 4.0f;
constexpr static inline float WAYPOINT_THRESHOLD = 0.05f;
constexpr static inline float REPATH_INTERVAL = 0.5f;
constexpr static inline float DETECT_RADIUS = 10.0f;

Enemy::Enemy(World& world, const glm::vec3 position) : sprite(
    position,
    { 1.0f, 0.0f, 0.0f },
    Sprite::ID::ENEMY
)
{
    sprite.transform.scale = {
        ENEMY_RADIUS,
        ENEMY_HEIGHT / 2.0f,
        ENEMY_RADIUS
    };

    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(
        { ENEMY_RADIUS, ENEMY_HEIGHT / 2.0f, ENEMY_RADIUS }
    );

    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();

    // Add physics
    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y, position.z),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::MOVING
    );
	JPH::Body* body = body_interface.CreateBody(settings);
    body->SetUserData((u64)this);
    body_id = body->GetID();
	body_interface.AddBody(body_id, JPH::EActivation::DontActivate);
}

void Enemy::update(World& world, const float delta, const glm::vec3 direction)
{
    sprite.face(direction * glm::vec3(1.0f, 0.0f, 1.0f));

    if (!can_see_player(world))
    {
        path.clear();
        repath_timer = 0.0f;
        return;
    }

    // Continuously replan toward the player's current position
    repath_timer -= delta;
    if (repath_timer <= 0.0f)
    {
        path = world.find_path(
            sprite.transform.position - glm::vec3(0.0f, ENEMY_HEIGHT / 2.0f, 0.0f),
            world.player->position
        );

        for (auto& point : path)
            point.y += ENEMY_HEIGHT / 2.0f;

        path_index = 0;
        repath_timer = REPATH_INTERVAL;
    }

    if (path.empty() || path_index >= path.size())
        return;

    // Get true length as likely to normalise anyway later
    glm::vec3 to_target = path[path_index] - sprite.transform.position;
    float distance = glm::length(to_target);

    // Go to next node when within reach of current
    if (distance < WAYPOINT_THRESHOLD)
    {
        path_index++;
        if (path_index >= path.size())
            return;

        to_target = path[path_index] - sprite.transform.position;
        distance = glm::length(to_target);
    }

    if (distance > 0.01f)
    {
        const glm::vec3 move_direction = to_target / distance;
        const float step = glm::min(SPEED * delta, distance);
        sprite.transform.position += move_direction * step;
    }

    // Update physics
    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    body_interface.SetPosition(
        body_id,
        JPH::RVec3(
            sprite.transform.position.x,
            sprite.transform.position.y,
            sprite.transform.position.z
        ),
        JPH::EActivation::DontActivate
    );
}

void Enemy::damage(const float damage)
{
    health -= damage;
}

bool Enemy::can_see_player(const World& world) const
{
    return glm::length2(world.player->position - sprite.transform.position) <
        DETECT_RADIUS * DETECT_RADIUS;
}