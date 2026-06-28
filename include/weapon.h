#pragma once
#include "model.h"

class Weapon
{
public:
    Weapon(const Model::ID model);

    glm::mat4 get_model_matrix(
        const glm::mat4& view_matrix,
        const float bob_amount
    ) const;

    Model::ID model;
};