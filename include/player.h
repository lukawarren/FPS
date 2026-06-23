#pragma once
#include "common.h"
#include "camera.h"
#include "window.h"
#include "flashlight.h"

class Player
{
public:
    Player(Window* window);
    void setup_physics(csg::world_t& world);
    void update(csg::world_t& world, const Camera& camera, const float delta);
    void update_camera(Camera& camera) const;
    void set_position(const glm::vec3& pos);

    Flashlight flashlight;

private:
    void handle_input(const Camera& camera, const float delta);
    void handle_physics(csg::world_t& world, const float delta);

    glm::vec3 position = {};
    float head_pitch = 0.0f;
    float head_yaw = 0.0f;
    glm::vec2 mouse_position;

    // Physics
    rp3d::PhysicsCommon physics_common;
    rp3d::PhysicsWorld* physics_world;
    rp3d::RigidBody* rigid_body;
    class RaycastCallback : public rp3d::RaycastCallback
    {
    public:
        bool hit = false;

        virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info)
        {
            (void)info;
            hit = true;
            return rp3d::decimal(1.0);
        }
    };

    Window* window;

    // Movement
    static bool do_ground_trace(rp3d::PhysicsWorld* physics_world, const glm::vec3& position, float height);
    static void do_accelerate(glm::vec3& vel, const glm::vec3& wishdir, float wishspeed, float accel, float dt);
    static void do_air_accelerate(glm::vec3& vel, const glm::vec3& wishdir, float wishspeed, float accel, float dt);
    static void do_friction(glm::vec3& vel, float friction, float stop_speed, float dt);
};