#pragma once
#include "common.h"
#include "transform.h"
#include "animated_sprite.h"
#include "model.h"

class World;

class Entity
{
public:
    Entity(const glm::vec3 position)
    {
        transform.position = position;
    }

    virtual void update(World& world, const float delta) = 0;
    virtual void on_activate(World& world) { (void)world; }
    virtual bool is_dead() const { return false; }

    Transform transform;
    std::optional<AnimatedSprite> sprite;
    std::optional<Model> model;
    std::optional<JPH::BodyID> body;
};