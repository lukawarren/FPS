#include "weapon.h"
#include "transform.h"

constexpr static inline float SCALE = 0.04f;
constexpr static inline float FIRE_RATE = 15.0f;
constexpr static inline float RECOIL_AMOUNT = 0.01f;

Weapon::Weapon(
    const Model::ID model,
    const std::function<void(World&)> on_fire,
    World& world
) : model(model), on_fire(on_fire), world(world) {}

glm::mat4 Weapon::get_model_matrix(const glm::mat4& view_matrix, const float bob_amount) const
{
    Transform t;
    t.scale = glm::vec3(SCALE);
    t.position.x = 0.10f + bob_amount * 0.05f;
    t.position.y = -0.07f + bob_amount * 0.05f;
    t.position.z = -0.12f + time * FIRE_RATE * RECOIL_AMOUNT;
    t.rotation.y = 180.0f;

    return glm::inverse(view_matrix) * t.matrix();
}

void Weapon::update(const bool fired, const bool fired_this_frame, const float delta)
{
    if (time > 0.0f)
        time -= delta;

    if (fired_this_frame)
        time = 0.0f;

    if (!fired)
        return;

    if (time <= 0.0f)
    {
        on_fire(world);
        time = 1.0f / FIRE_RATE;
    }
}