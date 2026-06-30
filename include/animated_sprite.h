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
        const float unit = 1.0f / (float)SPRITE_NAMES[(size_t)id].second;

        return {
            unit,
            1.0f,
            unit * (float)frame,
            0.0f
        };
    }

    inline u32 get_frame() const { return frame; }

    void advance()
    {
        frame = (frame + 1) % SPRITE_NAMES[(size_t)id].second;
    }

private:
    u32 frame = 0;
};