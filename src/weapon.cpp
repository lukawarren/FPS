#include "weapon.h"
#include "transform.h"

constexpr static inline float SCALE = 0.08f;

Weapon::Weapon(const Model::ID model) : model(model) {}

glm::mat4 Weapon::get_view_matrix() const
{
    glm::mat4 view = glm::mat4(1.0f);
    const float pitch = 0.0f;
    const float yaw = 0.0f;

    // Rotation
    view = glm::rotate(view, glm::radians(pitch), glm::vec3(1, 0, 0));
    view = glm::rotate(view, glm::radians(yaw),   glm::vec3(0, 1, 0));

    // Translation
    view = glm::translate(view, glm::vec3(0.0f));
    return view;
}

glm::mat4 Weapon::get_projection_matrix(const float width, const float height) const
{
    const float fov = glm::radians(60.0f);
    const float near = 0.01f;
    const float far = 100.0f;

    return glm::perspective(
        fov,
        width / height,
        near,
        far
    );
}

glm::mat4 Weapon::get_model_matrix() const
{
    Transform t;
    t.scale = glm::vec3(SCALE);
    t.position.x = 0.1f;
    t.position.y = -0.1f;
    t.position.z = -0.25f;
    t.rotation.x = 10.0f;
    return t.matrix();
}

glm::mat4 Weapon::get_true_model_matrix(
    const glm::vec3 player_position,
    const float player_pitch,
    const float player_yaw
) const
{
    Transform t;
    t.scale = glm::vec3(SCALE);
    t.position = player_position;
    t.rotation.x = -player_pitch;
    t.rotation.y = -player_yaw;
    return t.matrix() * get_model_matrix();
}