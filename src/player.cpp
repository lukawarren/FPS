#include "player.h"
#include "window.h"
#include "map.h"

constexpr float gravity = 15.0f;
constexpr float walk_speed = 5.0f;
constexpr float jump_height = 1.2f;
constexpr float height = 1.72;
constexpr float eye_height = height - 0.5;
constexpr float radius = 0.35f;

const float jump_speed = std::sqrtf(2.0f * gravity * jump_height);

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

    // Player body
    rp3d::Vector3 rb_position(position.x, position.y, position.z);
    rp3d::Quaternion rb_orientation = rp3d::Quaternion::identity();
    rp3d::Transform rb_transform(rb_position, rb_orientation);
    rigid_body = physics_world->createRigidBody(rb_transform);

    rp3d::CapsuleShape* collider = physics_common.createCapsuleShape(radius, height - radius * 2.0f);
    rigid_body->addCollider(collider, rp3d::Transform::identity());
    rigid_body->getCollider(0)->getMaterial().setBounciness(0.0f);
    rigid_body->setAngularLockAxisFactor(rp3d::Vector3(0, 0, 0));

    // Build triangle mesh for level geometry
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

            // Add vertices
            for (const auto& vertex : face.vertices)
            {
                vertices.emplace_back(
                    vertex.position.x * METRES_PER_UNIT,
                    vertex.position.z * METRES_PER_UNIT,
                    -vertex.position.y * METRES_PER_UNIT
                );
            }

            // Fan triangulation
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

    // Create TriangleVertexArray
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

    // Create TriangleMesh from the vertex array
    std::vector<rp3d::Message> messages;
    rp3d::TriangleMesh* triangleMesh = physics_common.createTriangleMesh(vertexArray, messages);

    if (!messages.empty())
        for (const auto& msg : messages)
            dbg(msg.text);

    // Create ConcaveMeshShape
    rp3d::ConcaveMeshShape* meshShape = physics_common.createConcaveMeshShape(triangleMesh);

    // Create static rigid body for the level
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
    handle_input(camera);
    handle_physics(world, delta);
}

void Player::handle_input(const Camera& camera)
{
    // WASD
    glm::vec2 movement = {};
    if (window->get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window->get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window->get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window->get_key(SDL_SCANCODE_D)) movement.x += 1.0f;
    if (movement.x != 0 || movement.y != 0)
        movement = glm::normalize(movement);

    // Apply relative to rotation
    glm::vec3 velocity = { 0.0f, rigid_body->getLinearVelocity().y, 0.0f };
    velocity += glm::vec3 { sin(glm::radians(camera.yaw)), 0, -cos(glm::radians(camera.yaw)) } * movement.y * walk_speed;
    velocity += glm::vec3 { cos(glm::radians(camera.yaw)), 0,  sin(glm::radians(camera.yaw)) } * movement.x * walk_speed;

    // Vertical movement
    RaycastCallback callback;
    rp3d::RaycastInfo info;
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
    if (window->get_key(SDL_SCANCODE_SPACE) && callback.hit)
        velocity.y = jump_speed;

    rigid_body->setLinearVelocity({
        velocity.x,
        velocity.y,
        velocity.z
    });

    // Mouse
    const float sensitivity = 0.2f;
    const glm::vec2 mouse_movement = window->get_mouse_movement();
    head_yaw += mouse_movement.x * sensitivity;
    head_pitch += mouse_movement.y * sensitivity;

    // Confine rotation
    head_pitch = std::max(std::min(head_pitch, 90.0f), -90.0f);
}

void Player::handle_physics(csg::world_t& world, const float delta)
{
    // Restrict physics time-scale to minimum of 60 FPS
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