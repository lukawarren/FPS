#pragma once
#include "common.h"
#include "transform.h"

class Sprite
{
public:
    enum class ID
    {
        BULLET = 0,
        ENEMY = 1
    };

    Sprite(const glm::vec3 position, const glm::vec3 direction, const ID id) : id(id)
    {
        transform.position = position;
        transform.rotation.y = glm::degrees(std::atan2(-direction.x, -direction.z));
        transform.rotation.x = glm::degrees(std::asin(direction.y));
    }

    void face(const glm::vec3 direction)
    {
        transform.rotation.y = glm::degrees(std::atan2(-direction.x, -direction.z));
        transform.rotation.x = glm::degrees(std::asin(direction.y));
    }

    static inline constexpr std::array<const char*, 2> SPRITE_NAMES =
    {
        "bullet",
        "enemy"
    };

    Transform transform;
    ID id;
};