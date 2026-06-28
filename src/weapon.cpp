#include "weapon.h"
#include "transform.h"

constexpr static inline float SCALE = 0.04f;

Weapon::Weapon(const Model::ID model) : model(model) {}

glm::mat4 Weapon::get_model_matrix(const glm::mat4& view_matrix, const float bob_amount) const
{
    Transform t;
    t.scale = glm::vec3(SCALE);
    t.position.x = 0.10f + bob_amount * 0.05f;
    t.position.y = -0.07f + bob_amount * 0.05f;
    t.position.z = -0.12f;
    t.rotation.y = 180.0f;

    return glm::inverse(view_matrix) * t.matrix();
}