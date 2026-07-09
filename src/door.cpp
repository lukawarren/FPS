#include "door.h"
#include "map.h"
#include "world.h"

constexpr static inline float WIDTH = 96.0f * Map::METRES_PER_UNIT;
constexpr static inline float HEIGHT = 128.0f * Map::METRES_PER_UNIT;
constexpr static inline float DEPTH = 16.0f * Map::METRES_PER_UNIT;
constexpr static inline float OFFSET = -8.0f * Map::METRES_PER_UNIT;

constexpr static inline float SLIDE_AMOUNT = (WIDTH / Map::METRES_PER_UNIT - 8.0f) * Map::METRES_PER_UNIT;
constexpr static inline float SLIDE_SPEED = 5.0f;
constexpr static inline float MIN_DISTANCE = 5.0f;

Door::Door(World& world, const glm::vec3 position, const glm::vec3 rotation) : Entity(position)
{
    model.emplace(Model(Model::ID::DOOR, {}, glm::vec3(Map::METRES_PER_UNIT)));
    transform.rotation = rotation;

    JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(
        { WIDTH / 2.0f, HEIGHT / 2.0f, DEPTH / 2.0f }
    );

    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();

    // Add physics
    const float pitch = glm::radians(rotation.x);
    const float yaw = glm::radians(rotation.y + 90.0f);
    const float roll = glm::radians(rotation.z);
    JPH::BodyCreationSettings settings(
        shape,
        JPH::RVec3(position.x, position.y + HEIGHT / 2.0f + OFFSET, position.z),
        JPH::Quat::sEulerAngles(JPH::Vec3(pitch, yaw, roll)),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );
	JPH::Body* body = body_interface.CreateBody(settings);
    body->SetUserData((u64)this);
    this->body.emplace(body->GetID());
	body_interface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
}

void Door::update(World& world, const float delta, const glm::vec3 view_direction)
{
    JPH::BodyInterface& body_interface = world.physics_system.GetBodyInterface();
    const glm::vec3 to_player = world.player->position - transform.position;
    const bool player_is_near = glm::length2(to_player) <= MIN_DISTANCE * MIN_DISTANCE;

    if (!is_open && !opening && player_is_near)
    {
        opening = true; // Start opening process
    }
    else if (is_open && opening && !player_is_near)
    {
        opening = false; // Start closing process
    }

    if (opening && !is_open)
    {
        // Must finish opening completely
        model->transform.position.z -= SLIDE_SPEED * delta;
        if (model->transform.position.z <= -SLIDE_AMOUNT)
        {
            model->transform.position.z = -SLIDE_AMOUNT;
            body_interface.SetObjectLayer(body.value(), Layers::NO_COLLISION);
            is_open = true;
        }
    }
    else if (!opening && is_open)
    {
        // Must finish closing completely
        model->transform.position.z += SLIDE_SPEED * delta;
        if (model->transform.position.z >= 0.0f)
        {
            model->transform.position.z = 0.0f;
            body_interface.SetObjectLayer(body.value(), Layers::NON_MOVING);
            is_open = false;
        }
    }

    // Update physics
    body_interface.SetPosition(
        body.value(),
        JPH::RVec3(
            transform.position.x,
            transform.position.y + HEIGHT / 2.0f + OFFSET,
            transform.position.z
        ),
        JPH::EActivation::DontActivate
    );

    (void)view_direction;
}
