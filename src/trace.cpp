#include "trace.h"
#include "world.h"

constexpr static inline glm::vec3 SCALE = { 0.05f, 0.05f, 1.0f };
constexpr static inline float SPEED = 50.0f;

Trace::Trace(World& world, const glm::vec3 position, const glm::vec3 destination) :
    Entity(position), direction(glm::normalize(destination - position))
{
    sprite.emplace(AnimatedSprite({}, {}, Sprite::ID::TRACE));
    sprite->face_camera = false;
    sprite->transform.rotation.y = 90.0f;
    sprite->transform.position.z = SCALE.z / 2.0f;
    transform.scale = SCALE;

    const float distance = glm::length(destination - position);
    time = distance / SPEED;

    glm::quat rot = glm::quatLookAt(direction, glm::vec3(0, 1, 0));
    glm::mat4 rotMat = glm::mat4_cast(rot);
    float x, y, z;
    glm::extractEulerAngleXYZ(rotMat, x, y, z);
    transform.rotation = glm::degrees(glm::vec3(x, y, z));
}

void Trace::update(World& world, const float delta)
{
    transform.position += direction * SPEED * delta;
    time -= delta;
}

bool Trace::is_dead() const
{
    return time <= 0.0f;
}