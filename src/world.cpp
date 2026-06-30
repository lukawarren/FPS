#include "world.h"

World::World(
    const std::string& filename,
    Window* window,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass
)
{
    map = new Map(filename, device, copy_pass);
    setup_physics();

    glm::vec3 player_position = {};
    float player_yaw = 0.0f;

    // Extract entities
    for (const auto& entity : map->entities)
    {
        if (entity.properties.count("classname") == 0) continue;
        const std::string& class_name = entity.properties.at("classname");

        if (class_name == "light_spotlight")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            const glm::vec3 colour = entity.parse_vec3("colour", glm::vec3(255.0f, 170.0f, 95.0f), false);
            const float intensity = entity.parse_float("intensity", 100.0f);
            const glm::vec3 angles = entity.parse_vec3("angles", { 0.0f, 0.0f, 0.0f }, false);
            const float near = entity.parse_float("near", 0.01f);
            const float far = entity.parse_float("far", 30.0f);
            const float angle = entity.parse_float("angle", 60.0f);

            const float pitch_rad = glm::radians(-angles.x);
            const float yaw_rad = glm::radians(-angles.y);
            const glm::vec3 direction = {
                std::cos(pitch_rad) * std::cos(yaw_rad),
                std::sin(pitch_rad),
                std::cos(pitch_rad) * std::sin(yaw_rad)
            };

            spotlights.emplace_back(
                position - direction * 8.0f * Map::METRES_PER_UNIT,
                direction,
                glm::normalize(colour / 255.0f) * intensity,
                near,
                far,
                glm::radians(angle)
            );

            animated_sprites.emplace_back(AnimatedSprite(
                position + direction * 0.42f + glm::vec3(0.0f, 0.3f, 0.0f),
                { 1.0f, 0.0f, 0.0f },
                Sprite::ID::FIRE,
                glm::vec3(0.4f)
            ));
        }

        else if (class_name == "info_player_start")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            const float angle = entity.parse_float("angle");
            player_position = position +
                glm::vec3(0.0f, Player::PLAYER_HEIGHT / 2.0f, 0.0f);
            player_yaw = 90.0f - angle;
        }

        else if (class_name == "enemy")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            enemies.emplace_back(std::make_unique<Enemy>(
                *this, position
            ));
        }
    }

    player = new Player(player_position, player_yaw, window, *this);
    dbg("TODO: decal limits");
}

void World::update(const float delta)
{
    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

    // Need 1 collision step for every 60 FPS
    const float divisions_of_60 = (1.0f / 60.0f) / delta;
    player->update(*this, delta);
    const JPH::EPhysicsUpdateError error = physics_system.Update(delta, divisions_of_60, &allocator, job_system);

    if (error != JPH::EPhysicsUpdateError::None)
        dbg("Warning: physics update error", (int)error);

    // Enemies
    const glm::vec3 direction = -camera.direction_vector();
    for (auto& e : enemies)
        e->update(*this, delta, direction);

    // Remove dead enemies
    for (const auto& e : enemies)
        if (e->is_dead())
            body_interface.RemoveBody(e->body_id);

    enemies.erase(std::remove_if(
        enemies.begin(), enemies.end(),
        [](const auto& e) {
            return e->is_dead();
        }),
        enemies.end()
    );

    // Sprites
    for (auto& s : animated_sprites)
    {
        s.face(direction);
        s.advance();
    }

    camera.pitch = player->head_pitch;
    camera.yaw = player->head_yaw;
    camera.position = {
        player->position.x,
        player->position.y - Player::PLAYER_HEIGHT / 2.0f + Player::PLAYER_EYE_HEIGHT + player->head_bob_offset,
        player->position.z
    };
}

void World::spawn_decal(const glm::vec3 position, const glm::vec3 direction)
{
    // Prevent clipping
    decals.emplace_back(
        position + direction * (0.001f * (float)decals.size()),
        direction,
        Decal::ID::BULLET
    );

    if (decals.size() > QUALITY_SETTINGS.max_decals)
        decals.pop_front();
}

std::vector<glm::vec3> World::find_path(const glm::vec3& start, const glm::vec3& end) const
{
    return map->find_path(start, end);
}

void World::setup_physics()
{
    job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers);

	const u32 MAX_RIGID_BODIES = 65536;
	const u32 MAX_BODY_PAIRS = 65536;
	const u32 MAX_CONTACT_CONSTRAINTS = 10240;
    physics_system.Init(
        MAX_RIGID_BODIES,
        0,
        MAX_BODY_PAIRS,
        MAX_CONTACT_CONSTRAINTS,
        broad_phase_layer_interface,
        object_vs_broadphase_layer_filter,
        object_vs_object_layer_filter
    );
    physics_system.SetGravity(physics_system.GetGravity() * 30.0f);

    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

    // Add map
    JPH::BodyCreationSettings map_settings(
        map->physics_shape,
        JPH::RVec3(0, 0, 0),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );
	JPH::Body* map_body = body_interface.CreateBody(map_settings);
	body_interface.AddBody(map_body->GetID(), JPH::EActivation::DontActivate);

    // Now that all colliders are added, optimise collisions
    physics_system.OptimizeBroadPhase();
}

World::~World()
{
    delete job_system;
    delete player;
    delete map;
}