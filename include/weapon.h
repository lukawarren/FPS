#pragma once
#include "model.h"

class Weapon
{
public:
    Weapon(const Model::ID model);
    glm::mat4 get_view_matrix() const;
    glm::mat4 get_projection_matrix(const float width, const float height) const;
    glm::mat4 get_model_matrix() const;
    glm::mat4 get_true_model_matrix(
        const glm::vec3 player_position,
        const float player_pitch,
        const float player_yaw
    ) const;
    Model::ID model;
};