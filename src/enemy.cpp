#include "enemy.h"
#include "player.h"
#include "world.h"
#include "render/debug_renderer.h"
#include "trace.h"

constexpr static inline float SPEED = 8.0f;

constexpr static inline float WAYPOINT_THRESHOLD    = 0.05f;

constexpr static inline float VIEW_DISTANCE         = 25.0f;
constexpr static inline float COVER_MIN_RADIUS      = 2.0f;
constexpr static inline float COVER_MAX_RADIUS      = 5.0f;
constexpr static inline u32   COVER_POINTS          = 10;
constexpr static inline float REPATH_TIME           = 5.0f;
constexpr static inline float STILL_RADIUS          = 10.0f;

constexpr static inline u32   IDLE_FRAMES           = 134;
constexpr static inline u32   RUNNING_FRAMES        = 41;
constexpr static inline u32   SHOOTING_FRAMES       = 35;
constexpr static inline float ANIMATION_FRAME_TIME  = 1.0f / 60.0f;

constexpr static inline float DETECT_DELAY          = 0.1f;
constexpr static inline float SHOOT_CHANCE          = 0.5f;
constexpr static inline float SHOOT_DAMAGE          = 5.0f;

Enemy::Enemy(World& world, const glm::vec3 position) : Entity(position)
{
    sprite.emplace(AnimatedSprite(
        {},
        { 1.0f, 0.0f, 0.0f },
        Sprite::ID::ENEMY
    ));

    sprite->transform.scale = glm::vec3 {
        ENEMY_RADIUS,
        ENEMY_HEIGHT / 2.0f,
        ENEMY_RADIUS
    } * 1.2f;

    sprite->transform.position.y = sprite->transform.scale.y / 3.0f;

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

void Enemy::update(World& world, const float delta)
{
    if (!world.player->is_dead())
    {
        // AI
        const auto last_state = state;
        think(world, delta);

        if (last_state != state)
            animation_time = 0.0f;
    }
    else state = State::Inactive;

    // Animation
    animate(world, delta);

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

void Enemy::think(World& world, const float delta)
{
    if (state == State::Inactive)
    {
        const auto hit = get_hit_from(world, transform.position, true);
        if (hit.has_value() && did_hit_player(world, *hit))
        {
            const glm::vec3 to_player = world.player->position - transform.position;

            if (glm::length2(to_player) <= STILL_RADIUS * STILL_RADIUS)
            {
                state = State::Shooting;
                repath_timer = REPATH_TIME * ((rand() / (float)RAND_MAX) + 0.5f);
            }
            else
            {
                state = State::Delayed;
                delayed_timer = DETECT_DELAY;
            }
        }
    }

    else if (state == State::Delayed)
    {
        delayed_timer -= delta;
        if (delayed_timer < 0.0f)
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
        if (path.empty() || path_index >= path.size())
        {
            state = State::Shooting;
            return;
        }

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
    }

    else if (state == State::Shooting)
    {
        if (repath_timer <= 0.0f)
        {
            state = State::Activated;
            return;
        }

        repath_timer -= delta;
    }
}

void Enemy::animate(World& world, const float delta)
{
    animation_time += delta;

    if (animation_time >= ANIMATION_FRAME_TIME)
    {
        animation_time = 0;
        animation_frame++;
    }

    if (state == State::Inactive)
    {
        sprite->frame = animation_frame % IDLE_FRAMES;
    }

    else if (state == State::Moving)
    {
        sprite->frame = IDLE_FRAMES + (animation_frame % RUNNING_FRAMES);

        if ((animation_frame % RUNNING_FRAMES) == 0)
            shoot(world);
    }

    else if (state == State::Shooting)
    {
        sprite->frame = IDLE_FRAMES + RUNNING_FRAMES + (animation_frame % SHOOTING_FRAMES);

        if ((animation_frame % SHOOTING_FRAMES) == 0)
            shoot(world);
    }
}

void Enemy::shoot(World& world)
{
    const auto hit = get_hit_from(world, transform.position, false);
    if (!hit.has_value()) return;

    world.audio.play_3d(
        Audio::ID::SHOT_LIGHT,
        { transform.position.x, transform.position.y, transform.position.z }
    );

    world.pending_entities.emplace_back(std::make_unique<Trace>(
        world,
        transform.position + glm::vec3(0.0f, 1.0f, 0.0f),
        hit->position,
        Sprite::ID::TRACE_RED,
        true
    ));

    float chance = (float)rand() / (float)RAND_MAX;
    if (chance > SHOOT_CHANCE && did_hit_player(world, *hit))
        world.player->damage(SHOOT_DAMAGE);
}

glm::vec3 Enemy::get_cover_pos(const World& world) const
{
    std::array<glm::vec3, COVER_POINTS> points;
    srand((u32)SDL_GetTicksNS());

    for (u32 i = 0; i < COVER_POINTS; i++)
    {
        bool found = false;
        u32 limit = 0;

        while (!found)
        {
            const float angle = rand() / (float)RAND_MAX * glm::two_pi<float>();
            const float distance = rand() / (float)RAND_MAX * (COVER_MAX_RADIUS - COVER_MIN_RADIUS) + COVER_MIN_RADIUS;

            points[i] = world.player->position + glm::vec3 {
                std::cos(angle) * distance,
                0.0f,
                std::sin(angle) * distance
            };

            const auto hit = get_hit_from(world, points[i], true);
            found = hit.has_value() && did_hit_player(world, *hit);

            if (limit++ > 10)
            {
                points[i] = world.player->position;
                break;
            }
        }
    }

    return points[rand() % points.size()] - glm::vec3(0.0f, Player::PLAYER_HEIGHT / 2.0f, 0.0f);
}

std::optional<Hit> Enemy::get_hit_from(
    const World& world,
    const glm::vec3 position,
    const bool aim_for_head
) const
{
    // Find player's head, etc.
    const glm::vec3 pos = transform.position + glm::vec3(0.0f, ENEMY_HEIGHT / 2.0f, 0.0f);
    const glm::vec3 detect_pos = world.player->position + glm::vec3 {
        0.0f,
        aim_for_head ? Player::PLAYER_EYE_HEIGHT / 2.0f : 0.0f,
        0.0f
    };
    const glm::vec3 to_player = glm::normalize(detect_pos - pos);
    return world.get_hit(
        pos,
        to_player,
        VIEW_DISTANCE,
        body.value()
    );
}

bool Enemy::did_hit_player(const World& world, const Hit& hit) const
{
    return
        hit.body_id.has_value() &&
        hit.body_id.value() == world.player->character->GetInnerBodyID();
}
