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
        transform.position = position + direction * 0.01f;
        transform.scale = glm::vec3(0.2f);
        transform.rotation.y = glm::degrees(std::atan2(-direction.x, -direction.z));
        transform.rotation.x = glm::degrees(std::asin(direction.y));
    }

    static inline constexpr std::array<const char*, 1> SPRITE_NAMES =
    {
        "bullet"
    };

    Transform transform;
    ID id;
};