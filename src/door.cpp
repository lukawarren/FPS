#include "door.h"
#include "map.h"
#include "world.h"

constexpr static inline float SMALL_WIDTH = 96.0f * Map::METRES_PER_UNIT;
constexpr static inline float SMALL_HEIGHT = 128.0f * Map::METRES_PER_UNIT;
constexpr static inline float SMALL_DEPTH = 16.0f * Map::METRES_PER_UNIT;
constexpr static inline float SMALL_SLIDE_AMOUNT = (SMALL_WIDTH / Map::METRES_PER_UNIT - 8.0f) * Map::METRES_PER_UNIT;

constexpr static inline float BIG_WIDTH = 256.0f * Map::METRES_PER_UNIT;
constexpr static inline float BIG_HEIGHT = 160.0f * Map::METRES_PER_UNIT;
constexpr static inline float BIG_DEPTH = 16.0f * Map::METRES_PER_UNIT;
constexpr static inline float BIG_SLIDE_AMOUNT = (BIG_HEIGHT / Map::METRES_PER_UNIT - 64.0f) * Map::METRES_PER_UNIT;

constexpr static inline float OFFSET = -8.0f * Map::METRES_PER_UNIT;
constexpr static inline float SLIDE_SPEED = 5.0f;
constexpr static inline float MIN_DISTANCE = 5.0f;

Door::Door(
    World& world,
    const glm::vec3 position,
    const glm::vec3 rotation,
    const bool is_big
) : Entity(position), is_big(is_big)
{
    model.emplace(Model(is_big ? Model::ID::BIG_DOOR : Model::ID::DOOR, {}, glm::vec3(Map::METRES_PER_UNIT)));

    // Small doors slide the other way
    transform.rotation = rotation + (is_big ? glm::vec3(0.0f) : glm::vec3(0.0f, 180.0f, 0.0f));

    JPH::Ref<JPH::BoxShape> shape = is_big ? new JPH::BoxShape(
        { BIG_WIDTH / 2.0f, BIG_HEIGHT / 2.0f, BIG_DEPTH / 2.0f }
    ) : new JPH::BoxShape(
        { SMALL_WIDTH / 2.0f, SMALL_HEIGHT / 2.0f, SMALL_DEPTH / 2.0f }
    );

    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();

    // Add physics
    const float pitch = glm::radians(rotation.x);
    const float yaw = glm::radians(rotation.y + 90.0f);
    const float roll = glm::radians(rotation.z);
    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y + (is_big ? BIG_HEIGHT : SMALL_HEIGHT) / 2.0f + OFFSET, position.z),
        JPH::Quat::sEulerAngles(JPH::Vec3(pitch, yaw, roll)),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );
	JPH::Body* body = body_interface.CreateBody(settings);
    body->SetUserData((u64)this);
    this->body.emplace(body->GetID());
	body_interface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
}

void Door::update(World& world, const float delta)
{
    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    const glm::vec3 to_player = world.player->position - transform.position;
    const bool player_is_near = glm::length2(to_player) <= MIN_DISTANCE * MIN_DISTANCE;

    if (!is_open && !opening && player_is_near)
    {
        // Start opening process
        opening = true;
        world.audio.play_3d(
            Audio::ID::DOOR_OPEN,
            { transform.position.x, transform.position.y, transform.position.z },
            2.0f
        );
    }
    else if (is_open && opening && !player_is_near)
    {
        // Start closing process
        opening = false;
        world.audio.play_3d(
            Audio::ID::DOOR_CLOSE,
            { transform.position.x, transform.position.y, transform.position.z },
            2.0f
        );
    }

    float& axis = is_big ? model->transform.position.y : model->transform.position.z;

    if (opening && !is_open)
    {
        // Must finish opening completely
        const float slide_amount = is_big ? BIG_SLIDE_AMOUNT : SMALL_SLIDE_AMOUNT;

        axis += SLIDE_SPEED * delta;
        if (axis >= slide_amount)
        {
            axis = slide_amount;
            body_interface.SetObjectLayer(body.value(), Layers::NO_COLLISION);
            is_open = true;
        }
    }
    else if (!opening && is_open)
    {
        // Must finish closing completely
        axis -= SLIDE_SPEED * delta;
        if (axis <= 0.0f)
        {
            axis = 0.0f;
            body_interface.SetObjectLayer(body.value(), Layers::NON_MOVING);
            is_open = false;
        }
    }

    // Update physics
    body_interface.SetPosition(
        body.value(),
        JPH::RVec3(
            transform.position.x,
            transform.position.y + (is_big ? BIG_HEIGHT : SMALL_HEIGHT) / 2.0f + OFFSET,
            transform.position.z
        ),
        JPH::EActivation::DontActivate
    );
}
