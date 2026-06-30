#pragma once
#include "common.h"
#include "transform.h"

class Sprite
{
public:
    enum class ID
    {
        BULLET = 0,
        ENEMY = 1,
        FIRE = 2
    };

    Sprite(
        const glm::vec3 position,
        const glm::vec3 direction,
        const ID id,
        const glm::vec3 scale = glm::vec3(1.0f)
    ) : id(id)
    {
        transform.scale = scale;
        transform.position = position;
        transform.rotation.y = glm::degrees(std::atan2(-direction.x, -direction.z));
        transform.rotation.x = glm::degrees(std::asin(direction.y));
    }

    void face(const glm::vec3 direction)
    {
        transform.rotation.y = glm::degrees(std::atan2(-direction.x, -direction.z));
        transform.rotation.x = glm::degrees(std::asin(direction.y));
    }

    virtual glm::vec4 get_spritesheet_info() const
    {
        return { 1.0f, 1.0f, 0.0f, 0.0f };
    }

    static inline constexpr std::array<std::pair<const char*, u32>, 3> SPRITE_NAMES =
    {{
        { "bullet", 1 },
        { "enemy", 1 },
        { "fire", 25 }
    }};

    Transform transform;
    ID id;
};