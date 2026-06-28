#pragma once
#include "common.h"
#include "transform.h"

class Decal
{
public:
    enum class ID
    {
        BULLET = 0
    };

    Decal(const glm::vec3 position, const glm::vec3 direction, const ID id) : id(id)
    {
        transform.position = position;
        transform.scale = glm::vec3(0.1f);
    }

    static inline constexpr std::array<const char*, 1> SPRITE_NAMES =
    {
        "bullet"
    };

    Transform transform;
    ID id;
};