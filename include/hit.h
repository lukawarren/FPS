#pragma once
#include "common.h"

struct Hit
{
    glm::vec3 position;
    glm::vec3 normal;
    std::optional<JPH::BodyID> body_id;
};