#pragma once
#include "common.h"
#include "transform.h"
#include "sprite.h"

class Decal : public Sprite
{
public:
    Decal(const glm::vec3 position, const glm::vec3 direction, const ID id) :
        Sprite(position, direction, id)
    {
        transform.scale = glm::vec3(0.1f);
        this->face_camera = false;
    }
};