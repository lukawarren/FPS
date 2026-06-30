#pragma once
#include "model.h"

class Weapon
{
public:
    Weapon(
        const Model::ID model,
        const std::function<void()> on_fire
    );

    glm::mat4 get_model_matrix(
        const glm::mat4& view_matrix,
        const float bob_amount
    ) const;

    void update(const bool fired, const bool fired_this_frame, const float delta);

    Model::ID model;
    float time = 0.0f;
    std::function<void()> on_fire;

    constexpr static inline float DAMAGE = 20.0f;
};