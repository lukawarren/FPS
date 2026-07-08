#include "enemy.h"
#include "player.h"
#include "world.h"
#include "render/debug_renderer.h"

constexpr static inline float SPEED = 0.0f;

constexpr static inline float WAYPOINT_THRESHOLD = 0.05f;

constexpr static inline float VIEW_DISTANCE     = 100.0f;
constexpr static inline float COVER_MIN_RADIUS  = 2.0f;
constexpr static inline float COVER_MAX_RADIUS  = 15.0f;
constexpr static inline u32   COVER_POINTS      = 10;
constexpr static inline float REPATH_TIME       = 3.0f;

Enemy::Enemy(World& world, const glm::vec3 position) : Entity(position)
{
    sprite.emplace(Sprite(
        {},
        { 1.0f, 0.0f, 0.0f },
        Sprite::ID::ENEMY
    ));

    sprite->transform.scale = {
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
    this->body.emplace(body->GetID());
	body_interface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
}

Enemy::~Enemy() {}

void Enemy::update(World& world, const float delta, const glm::vec3 view_direction)
{
    // Update sprite
    sprite->face(view_direction * glm::vec3(1.0f, 0.0f, 1.0f));

    if (state == State::Inactive)
    {
        if (can_see_player_from(world, transform.position))
            state = State::Activated;
    }

    else if (state == State::Activated)
    {
        const glm::vec3 destination = get_cover_pos(world);

        path = world.find_path(
            transform.position - glm::vec3(0.0f, ENEMY_HEIGHT / 2.0f, 0.0f),
            destination
        );

        path_index = 0;
        repath_timer = REPATH_TIME * ((rand() / (float)RAND_MAX) + 0.5f);

        state = State::Moving;
    }

    else if (state == State::Moving)
    {
        if (repath_timer <= 0.0f)
        {
            state = State::Activated;
            return;
        }

        repath_timer -= delta;

        if (path.empty() || path_index >= path.size())
            return;

        // Get true length as likely to normalise anyway later
        glm::vec3 to_target = path[path_index] + glm::vec3(0.0f, ENEMY_HEIGHT / 2.0f, 0.0f) - transform.position;
        float distance = glm::length(to_target);

        // Go to next node when within reach of current
        if (distance < WAYPOINT_THRESHOLD)
        {
            path_index++;
            if (path_index >= path.size())
                return;

            to_target = path[path_index] - transform.position;
            distance = glm::length(to_target);
        }

        if (distance > 0.01f)
        {
            const glm::vec3 move_direction = to_target / distance;
            const float step = glm::min(SPEED * delta, distance);
            transform.position += move_direction * step;
        }

        glm::vec3 x = path[path.size() - 1];
        DebugRenderer::debug_renderer->DrawMarker(
            { x.x, x.y, x.z },
            {},
            1.0f
        );
    }

    // Update physics
    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    body_interface.SetPosition(
        body.value(),
        JPH::RVec3(
            transform.position.x,
            transform.position.y,
            transform.position.z
        ),
        JPH::EActivation::DontActivate
    );
}

bool Enemy::is_dead() const
{
    return health <= 0.0f;
}

void Enemy::damage(const float damage)
{
    health -= damage;
}

glm::vec3 Enemy::get_player_detect_pos(const World& world) const
{
    return world.player->position + glm::vec3 {
        0.0f,
        Player::PLAYER_EYE_HEIGHT / 2.0f,
        0.0f
    };
}

glm::vec3 Enemy::get_cover_pos(const World& world) const
{
    std::array<glm::vec3, COVER_POINTS> points;
    srand((u32)SDL_GetTicksNS());

    for (u32 i = 0; i < COVER_POINTS; i++)
    {
        bool found = false;

        while (!found)
        {
            const float angle = rand() / (float)RAND_MAX * glm::two_pi<float>();
            const float distance = rand() / (float)RAND_MAX * (COVER_MAX_RADIUS - COVER_MIN_RADIUS) + COVER_MIN_RADIUS;

            points[i] = world.player->position + glm::vec3 {
                std::cos(angle) * distance,
                0.0f,
                std::sin(angle) * distance
            };

            found = can_see_player_from(world, points[i]);
        }
    }

    return points[rand() % points.size()] - glm::vec3(0.0f, Player::PLAYER_HEIGHT / 2.0f, 0.0f);
}

bool Enemy::can_see_player_from(const World& world, const glm::vec3 position) const
{
    // Find player's head, etc.
    const glm::vec3 detect_pos = get_player_detect_pos(world);
    const glm::vec3 to_player = glm::normalize(detect_pos - position);
    const auto hit = world.get_hit(
        position,
        to_player,
        VIEW_DISTANCE,
        body.value()
    );

    return
        hit.has_value() &&
        hit->body_id.has_value() &&
        hit->body_id.value() == world.player->character->GetInnerBodyID();
}