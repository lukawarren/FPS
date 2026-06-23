#include "player.h"
#include "window.h"
#include "map.h"

// Player dimensions
constexpr float height = 1.72f;
constexpr float eye_height = height - 0.5f;
constexpr float radius = 0.35f;

// Movement
constexpr float gravity         = 15.0f;
constexpr float jump_height     = 1.2f;
const     float jump_speed      = std::sqrtf(2.0f * gravity * jump_height);
constexpr float max_speed       = 320.0f * Map::METRES_PER_UNIT; // sv_maxspeed
constexpr float accelerate      = 10.0f;                          // sv_accelerate (unitless scalar)
constexpr float air_accelerate  = 10.0f;                          // sv_airaccelerate
constexpr float friction        = 4.0f;                           // sv_friction
constexpr float stop_speed      = 100.0f * Map::METRES_PER_UNIT;  // sv_stopspeed
constexpr float air_cap         = 30.0f * Map::METRES_PER_UNIT;   // air wishspeed cap

Player::Player(Window* window) : window(window)
{
    mouse_position = window->get_mouse_position();
    window->capture_mouse();
    position.y = 1.0f;
}

void Player::setup_physics(csg::world_t& world)
{
    physics_world = physics_common.createPhysicsWorld();
    physics_world->setGravity({ 0.0f, -gravity, 0.0f });

    rp3d::Vector3 rb_position(position.x, position.y, position.z);
    rp3d::Quaternion rb_orientation = rp3d::Quaternion::identity();
    rp3d::Transform rb_transform(rb_position, rb_orientation);
    rigid_body = physics_world->createRigidBody(rb_transform);

    rp3d::CapsuleShape* collider = physics_common.createCapsuleShape(radius, height - radius * 2.0f);
    rigid_body->addCollider(collider, rp3d::Transform::identity());
    rigid_body->getCollider(0)->getMaterial().setBounciness(0.0f);
    rigid_body->setAngularLockAxisFactor(rp3d::Vector3(0, 0, 0));

    std::vector<rp3d::Vector3> vertices;
    std::vector<u32> indices;

    csg::brush_t* brush = world.first();
    while (brush != nullptr)
    {
        for (const auto& face : brush->faces)
        {
            if (face.vertices.size() < 3)
                continue;

            const u32 base_index = (u32)vertices.size();

            for (const auto& vertex : face.vertices)
            {
                vertices.emplace_back(
                    vertex.position.x * Map::METRES_PER_UNIT,
                    vertex.position.z * Map::METRES_PER_UNIT,
                    -vertex.position.y * Map::METRES_PER_UNIT
                );
            }

            for (size_t i = 1; i + 1 < face.vertices.size(); ++i)
            {
                indices.push_back(base_index);
                indices.push_back(base_index + i);
                indices.push_back(base_index + i + 1);
            }
        }
        brush = world.next(brush);
    }

    if (vertices.empty())
    {
        dbg("No level geometry found!");
        return;
    }

    rp3d::TriangleVertexArray vertexArray(
        vertices.size(),
        vertices.data(),
        3 * sizeof(float),
        indices.size() / 3,
        indices.data(),
        3 * sizeof(u32),
        rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
        rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE
    );

    std::vector<rp3d::Message> messages;
    rp3d::TriangleMesh* triangleMesh = physics_common.createTriangleMesh(vertexArray, messages);

    if (!messages.empty())
        for (const auto& msg : messages)
            dbg(msg.text);

    rp3d::ConcaveMeshShape* meshShape = physics_common.createConcaveMeshShape(triangleMesh);

    rp3d::RigidBody* levelBody = physics_world->createRigidBody(rp3d::Transform::identity());
    levelBody->setType(rp3d::BodyType::STATIC);
    levelBody->addCollider(meshShape, rp3d::Transform::identity());
    levelBody->getCollider(0)->getMaterial().setFrictionCoefficient(1.0f);
    levelBody->getCollider(0)->getMaterial().setBounciness(0.0f);
}

void Player::update(
    csg::world_t& world,
    const Camera& camera,
    const float delta
)
{
    handle_input(camera, delta);
    handle_physics(world, delta);
    flashlight.update(position, camera.pitch, camera.yaw);
}

bool Player::do_ground_trace(rp3d::PhysicsWorld* physics_world, const glm::vec3& position, float height)
{
    RaycastCallback callback;
    physics_world->raycast({
        {
            position.x,
            position.y - height / 2.0f + 0.05f,
            position.z
        },
        {
            position.x,
            position.y - height / 2.0f - 0.1f,
            position.z
        },
    }, &callback);
    return callback.hit;
}

void Player::do_accelerate(glm::vec3& vel, const glm::vec3& wishdir, float wishspeed, float accel, float dt)
{
    const float current_speed = glm::dot(vel, wishdir);
    const float add_speed = wishspeed - current_speed;
    if (add_speed <= 0.0f)
        return;

    float accel_speed = accel * dt * wishspeed;
    accel_speed = std::min(accel_speed, add_speed);

    vel += wishdir * accel_speed;
}

void Player::do_air_accelerate(glm::vec3& vel, const glm::vec3& wishdir, float wishspeed, float accel, float dt)
{
    const float capped_wishspeed = std::min(wishspeed, air_cap);

    const float current_speed = glm::dot(vel, wishdir);
    const float add_speed = capped_wishspeed - current_speed;
    if (add_speed <= 0.0f)
        return;

    float accel_speed = accel * wishspeed * dt;
    accel_speed = std::min(accel_speed, add_speed);

    vel += wishdir * accel_speed;
}

void Player::do_friction(glm::vec3& vel, float friction, float stop_speed, float dt)
{
    const float speed = glm::length(vel);
    if (speed < 0.1f)
    {
        vel = {};
        return;
    }

    const float control = speed < stop_speed ? stop_speed : speed;
    const float drop = control * friction * dt;

    float new_speed = speed - drop;
    if (new_speed < 0.0f)
        new_speed = 0.0f;

    vel *= (new_speed / speed);
}

void Player::handle_input(const Camera& camera, const float delta)
{
    // WASD -> wishdir
    glm::vec2 movement = {};
    if (window->get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window->get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window->get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window->get_key(SDL_SCANCODE_D)) movement.x += 1.0f;
    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    const glm::vec3 forward = { sin(glm::radians(camera.yaw)), 0.0f, -cos(glm::radians(camera.yaw)) };
    const glm::vec3 right   = { cos(glm::radians(camera.yaw)), 0.0f,  sin(glm::radians(camera.yaw)) };

    glm::vec3 wishdir = forward * movement.y + right * movement.x;
    const float wishspeed = max_speed;
    if (glm::length(wishdir) > 0.0001f)
        wishdir = glm::normalize(wishdir);

    // Pull current horizontal velocity out of the rigid body
    const rp3d::Vector3 rb_vel = rigid_body->getLinearVelocity();
    glm::vec3 velocity = { rb_vel.x, 0.0f, rb_vel.z };
    float vertical_velocity = rb_vel.y;

    const bool on_ground = do_ground_trace(physics_world, position, height);

    if (on_ground)
    {
        do_friction(velocity, friction, stop_speed, delta);
        do_accelerate(velocity, wishdir, wishspeed, accelerate, delta);

        if (window->get_key(SDL_SCANCODE_SPACE))
            vertical_velocity = jump_speed;
    }
    else
    {
        do_air_accelerate(velocity, wishdir, wishspeed, air_accelerate, delta);
    }

    rigid_body->setLinearVelocity({ velocity.x, vertical_velocity, velocity.z });

    // Mouse
    const float sensitivity = 0.2f;
    const glm::vec2 mouse_movement = window->get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;
    head_pitch = std::max(std::min(head_pitch, 90.0f), -90.0f);
}

void Player::handle_physics(csg::world_t& world, const float delta)
{
    const float physics_delta = std::min(delta, 1.0f / 60.0f);
    (void)world;

    physics_world->update(physics_delta);

    const rp3d::Vector3 rb_position = rigid_body->getTransform().getPosition();
    position.x = rb_position.x;
    position.y = rb_position.y;
    position.z = rb_position.z;
}

void Player::update_camera(Camera& camera) const
{
    camera.pitch = head_pitch;
    camera.yaw = head_yaw;
    camera.position = {
        position.x,
        position.y + eye_height / 2.0f,
        position.z
    };
}

void Player::set_position(const glm::vec3& pos)
{
    position = pos;
    position.y += radius;
}