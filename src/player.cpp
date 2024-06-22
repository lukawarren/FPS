#include "player.h"
#include "window.h"
#include "config.h"

constexpr float gravity = 15.0f;
constexpr float walk_speed = 5.0f;
constexpr float jump_height = 1.2f;
constexpr float height = 1.72;
constexpr float eye_height = height - 0.1;
constexpr float radius = 0.25f;

const float jump_speed = std::sqrtf(2.0f * gravity * jump_height);

Player::Player()
{
    mouse_position = Window::window->mouse_position();
    position.y = 10;
}

void Player::setup_physics(csg::world_t& world)
{
    physics_world = physics_common.createPhysicsWorld();
    physics_world->setGravity({ 0.0f, -gravity, 0.0f });

    // Player rigidbody
    rp3d::Vector3 rb_position(position.x, position.y, position.z);
    rp3d::Quaternion rb_orientation = rp3d::Quaternion::identity();
    rp3d::Transform rb_transform(rb_position, rb_orientation);
    rigid_body = physics_world->createRigidBody(rb_transform);

    // Player collider
    rp3d::CapsuleShape* collider = physics_common.createCapsuleShape(radius, height);
    rp3d::Transform collider_transform = rp3d::Transform::identity();
    rigid_body->addCollider(collider, collider_transform);

    // Stop bounciness!
    rigid_body->getCollider(0)->getMaterial().setBounciness(0.0f);

    // Stop rotation
    rigid_body->setAngularLockAxisFactor(rp3d::Vector3(0, 0, 0));

    // Level geometry
    csg::brush_t* brush = world.first();
    while (brush != nullptr)
    {
        std::vector<float> vertices;
        for (const auto& face : brush->faces)
        {
            for (const auto& vertex : face.vertices)
            {
                vertices.push_back(vertex.position.x * metres_per_unit);
                vertices.push_back(vertex.position.z * metres_per_unit);
                vertices.push_back(vertex.position.y * metres_per_unit * -1.0f);
            }
        }

        const rp3d::VertexArray vertex_array(
            vertices.data(),
            3 * sizeof(float),
            vertices.size() / 3,
            rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE
        );

        std::vector<rp3d::Message> messages;
        physics_common.createConvexMesh(vertex_array, messages);
        if (messages.size() != 0)
            dbg("unexpected rp3d message");
        messages.clear();

        rp3d::ConvexMesh* convex_mesh = physics_common.createConvexMesh(vertex_array, messages);
        if (messages.size() != 0)
            dbg("unexpected rp3d message");

        rp3d::ConvexMeshShape* convex_mesh_shape = physics_common.createConvexMeshShape(convex_mesh);

        const rp3d::Transform transform = rp3d::Transform::identity();
        rp3d::RigidBody* brush_body = physics_world->createRigidBody(transform);
        brush_body->setType(rp3d::BodyType::STATIC);
        brush_body->addCollider(convex_mesh_shape, transform);

        // Material settings
        brush_body->getCollider(0)->getMaterial().setBounciness(0.0f);
        brush_body->getCollider(0)->getMaterial().setFrictionCoefficient(1.0f);

        brush = world.next(brush);
    }
}

void Player::update(
    csg::world_t& world,
    const Camera& camera,
    const float delta
)
{
    handle_input(camera);
    handle_physics(world, delta);

    ImGui::Begin("player");
    ImGui::Text("Position: (%f, %f, %f)", position.x, position.y, position.z);
    ImGui::End();
}

void Player::handle_input(const Camera& camera)
{
    Window& window = *Window::window;

    // WASD
    glm::vec2 movement = {};
    if (window.get_key(GLFW_KEY_W)) movement.y += 1.0f;
    if (window.get_key(GLFW_KEY_S)) movement.y -= 1.0f;
    if (window.get_key(GLFW_KEY_A)) movement.x -= 1.0f;
    if (window.get_key(GLFW_KEY_D)) movement.x += 1.0f;
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
            position.y - height / 2.0f - 0.001f,
            position.z
        },
        {
            position.x,
            position.y - height / 2.0f - radius - 0.01f,
            position.z
        },
    }, &callback);
    if (window.get_key(GLFW_KEY_SPACE) && callback.hit)
        velocity.y = jump_speed;

    rigid_body->setLinearVelocity({
        velocity.x,
        velocity.y,
        velocity.z
    });

    // Mouse
    const float sensitivity = 0.1f;
    const glm::vec2 mouse_movement = window.mouse_movement();
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