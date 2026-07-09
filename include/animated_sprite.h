#pragma once
#include "common.h"
#include "sprite.h"

class AnimatedSprite : public Sprite
{
public:
    AnimatedSprite(
        const glm::vec3 position,
        const glm::vec3 direction,
        const ID id,
        const glm::vec3 scale = glm::vec3(1.0f)
    ) : Sprite(position, direction, id, scale) {}

    virtual glm::vec4 get_spritesheet_info() const override
    {
        return {
            1.0f / (float)SPRITE_NAMES[(size_t)id].second.first,
            1.0f / (float)SPRITE_NAMES[(size_t)id].second.second,
            1.0f / (float)SPRITE_NAMES[(size_t)id].second.first * (float)(frame % SPRITE_NAMES[(size_t)id].second.first),
            1.0f / (float)SPRITE_NAMES[(size_t)id].second.second * (float)(frame / SPRITE_NAMES[(size_t)id].second.first)
        };
    }

    u32 frame = 0;
};