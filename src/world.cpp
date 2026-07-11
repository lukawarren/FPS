#include "world.h"
#include "render/renderer.h"
#include "render/debug_renderer.h"
#include "enemy.h"
#include "door.h"

static bool enable_ai = false;

World::World(
    const std::string& filename,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass,
    Window& window,
    Renderer& renderer,
    Audio& audio
) : window(window), renderer(renderer), audio(audio)
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
            const glm::vec3 colour = entity.parse_vec3("colour", glm::vec3(247.0f, 241.0f, 150.0f), false);
            const float intensity = entity.parse_float("intensity", 10.0f);
            const glm::vec3 angles = entity.parse_angles("angles");
            const float near = entity.parse_float("near", 0.01f);
            const float far = entity.parse_float("far", 30.0f);
            const float angle = entity.parse_float("angle", 80.0f);

            spotlights.emplace_back(
                position - angles * 8.0f * Map::METRES_PER_UNIT,
                angles,
                colour / 255.0f,
                intensity,
                near,
                far,
                glm::radians(angle)
            );

            audio.play_3d(
                Audio::ID::LIGHT,
                { position.x, position.y, position.z },
                1.0f,
                0.01f,
                5.0f
            );
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
            const glm::vec3 position = entity.parse_vec3("origin") + glm::vec3 {
                0.0f,
                Enemy::ENEMY_HEIGHT / 2.0f,
                0.0f
            };

            entities.emplace_back(std::make_unique<Enemy>(
                *this, position
            ));
        }

        else if (class_name == "func_door")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            const glm::vec3 rotation = entity.parse_angle("angle");
            const float size = entity.parse_float("size");
            entities.emplace_back(std::make_unique<Door>(
                *this, position, rotation, size == 1.0f
            ));
        }
    }

    player = new Player(player_position, player_yaw, window, *this, audio);
    audio.play(Audio::ID::AMBIENCE);
}

void World::update(const float delta)
{
    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();

    const glm::vec3 direction = -camera.direction_vector();

    if (!debug_mode)
    {
        // Need 1 collision step for every 60 FPS
        const float divisions_of_60 = std::max((1.0f / 60.0f) / delta, 1.0f);
        if (!player->is_dead()) player->update(delta);
        const JPH::EPhysicsUpdateError error = physics_system.Update(delta, (int)divisions_of_60, &allocator, job_system);

        if (error != JPH::EPhysicsUpdateError::None)
            dbg("Warning: physics update error", (int)error);
    }

    // Entities
    for (auto& e : entities)
    {
        if (e->sprite.has_value() && e->sprite->face_camera)
            e->sprite->face(direction * glm::vec3(1.0f, 0.0f, 1.0f));

        if (dynamic_cast<const Enemy*>(e.get()) == nullptr || enable_ai)
            e->update(*this, delta);
    }

    // Remove dead entities
    for (const auto& e : entities)
        if (e->is_dead() && e->body.has_value())
            body_interface.RemoveBody(e->body.value());

    entities.erase(std::remove_if(
        entities.begin(), entities.end(),
        [](const auto& e) {
            return e->is_dead();
        }),
        entities.end()
    );

    if (!pending_entities.empty())
    {
        entities.insert(
            entities.end(),
            std::make_move_iterator(pending_entities.begin()),
            std::make_move_iterator(pending_entities.end())
        );
        pending_entities.clear();
    }

    // Debug toggle
    if (window.get_key_pressed(SDL_SCANCODE_ESCAPE))
    {
        debug_mode = !debug_mode;
        if (!debug_mode && !mouse_captured)
        {
            window.capture_mouse();
            mouse_captured = true;
        }
    }

    // Camera
    if (!debug_mode)
    {
        if (player->is_dead())
        {
            camera.pitch = 0.0f;
            camera.roll = 45.0f;
            camera.position = {
                player->position.x,
                player->position.y - Player::PLAYER_HEIGHT / 2.0f + 0.1f,
                player->position.z
            };
        }
        else
        {
            camera.pitch = player->head_pitch;
            camera.yaw = player->head_yaw;
            camera.position = {
                player->position.x,
                player->position.y - Player::PLAYER_HEIGHT / 2.0f + Player::PLAYER_EYE_HEIGHT + player->head_bob_offset,
                player->position.z
            };
        }
    }
    else update_debug_mode(delta);

    // Audio
    audio.set_listener(
        { camera.position.x, camera.position.y, camera.position.z },
        { -direction.x, -direction.y, -direction.z }
    );

    time += delta;
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

std::optional<Hit> World::get_hit(
    const glm::vec3& origin,
    const glm::vec3& forward,
    const float max_distance,
    const std::optional<JPH::BodyID> ignore
) const
{
    JPH::RVec3 start = {
        origin.x,
        origin.y,
        origin.z
    };

    JPH::RVec3 direction = JPH::RVec3 { forward.x, forward.y, forward.z } * max_distance;
    JPH::RRayCast ray { start, direction };

    JPH::RayCastSettings settings;
    settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);

    JPH::ClosestHitPerBodyCollisionCollector<JPH::CastRayCollector> collector;

    IgnoreLayerFilter layer_filter(Layers::NO_COLLISION);

    if (ignore.has_value())
    {
        const JPH::IgnoreSingleBodyFilter body_filter(ignore.value());
        physics_system.GetNarrowPhaseQuery().CastRay(
            ray,
            settings,
            collector,
            {},
            layer_filter,
            body_filter
        );
    }
    else
        physics_system.GetNarrowPhaseQuery().CastRay(ray, settings, collector, {}, layer_filter);

    if (collector.HadHit())
    {
        collector.Sort();
        const JPH::RayCastResult& hit = collector.mHits[0];
        JPH::Vec3 position = start + direction * hit.mFraction;
        std::optional<JPH::BodyID> body_id = std::nullopt;

        // Get surface normal
        JPH::BodyLockRead lock(physics_system.GetBodyLockInterface(), hit.mBodyID);
        JPH::Vec3 jolt_normal = JPH::Vec3::sAxisY();
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            jolt_normal = body.GetShape()->GetSurfaceNormal(
                hit.mSubShapeID2,
                ray.GetPointOnRay(hit.mFraction)
            );
            jolt_normal = body.GetWorldTransform().Multiply3x3(jolt_normal);
            body_id = body.GetID();
        }

        return std::optional<Hit>(Hit {
            .position = glm::vec3(
                position.GetX(),
                position.GetY(),
                position.GetZ()
            ),
            .normal = glm::vec3(
                jolt_normal.GetX(),
                jolt_normal.GetY(),
                jolt_normal.GetZ()
            ),
            .body_id = body_id
        });
    }

    return std::nullopt;
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

void World::update_debug_mode(const float delta)
{
    update_freecam(delta);

    // Draw player
    player->character->GetShape()->Draw(
        DebugRenderer::debug_renderer,
        player->character->GetWorldTransform(),
        { 1.0f, 1.0f, 1.0f },
        {},
        false,
        true
    );

    // Draw entities
    JPH::BodyInterface& body_interface = physics_system.GetBodyInterface();
    for (const auto& e : entities)
    {
        if (e->body.has_value())
        {
            JPH::RefConst<JPH::Shape> shape = body_interface.GetShape(e->body.value());
            shape->Draw(
                DebugRenderer::debug_renderer,
                body_interface.GetWorldTransform(e->body.value()),
                { 1.0f, 1.0f, 1.0f },
                {},
                false,
                true
            );
        }
    }

    // Draw lights
    for (const auto& spotlight : spotlights)
    {
        const JPH::Vec3 pos(
            spotlight.position.x,
            spotlight.position.y,
            spotlight.position.z
        );

        DebugRenderer::debug_renderer->DrawArrow(
            pos,
            pos + JPH::Vec3(spotlight.direction.x, spotlight.direction.y, spotlight.direction.z) * 2.0f,
            {},
            0.5f
        );
    }

    ImGui::Begin("World", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Checkbox("Enemy AI", &enable_ai);
    if (ImGui::Button("Heal"))
    {
        camera.roll = 0.0f;
        player->health = 100.0f;
    }
    ImGui::End();

    static float fov = 120.0f;
    ImGui::Begin("Post Processing", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::DragFloat("FOV", &fov, 1.0f, 0.0f, 180.0f);
    ImGui::DragFloat("Bloom strength", &renderer.post_processing_settings().bloom_strength, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Exposure", &renderer.post_processing_settings().exposure, 1.0f, 0.0f, 1000.0f);
    ImGui::DragFloat("Gamma", &renderer.post_processing_settings().gamma, 0.01f, 0.0f, 5.0f);
    ImGui::DragFloat("Panini Strength", &renderer.post_processing_settings().panini_strength, 1.0f, 0.0f, 10.0f);
    ImGui::End();
    camera.fov = glm::radians(fov);

    ImGui::Begin("Lights", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    for (size_t i = 0; i < spotlights.size(); i++)
    {
        const std::string label = "Light " + std::to_string(i);
        const std::string colour = label + " colour";
        const std::string intensity = label + " intensity";
        const std::string teleport = "Teleport to light " + std::to_string(i);

        ImGui::ColorEdit3(colour.c_str(), glm::value_ptr(spotlights[i].colour));
        ImGui::DragFloat(intensity.c_str(), &spotlights[i].intensity, 1.0f, 0.0f, 1000.0f);
        if (ImGui::Button(teleport.c_str()))
        {
            camera.position = spotlights[i].position + spotlights[i].direction * 3.0f;
        }

        ImGui::Separator();
    }
    ImGui::End();
}

void World::update_freecam(const float delta)
{
    const float freecam_speed = 20.0f;

    glm::vec2 movement = {};
    if (window.get_key(SDL_SCANCODE_W)) movement.y += 1.0f;
    if (window.get_key(SDL_SCANCODE_S)) movement.y -= 1.0f;
    if (window.get_key(SDL_SCANCODE_A)) movement.x -= 1.0f;
    if (window.get_key(SDL_SCANCODE_D)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
        movement = glm::normalize(movement);

    const float pitch_rad = glm::radians(-camera.pitch);
    const float yaw_rad = glm::radians(-camera.yaw);

    const glm::vec3 forward = {
        -std::sin(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        -std::cos(yaw_rad) * std::cos(pitch_rad)
    };

    const glm::vec3 right = {
        std::cos(yaw_rad),
        0.0f,
        -std::sin(yaw_rad)
    };

    const glm::vec3 up = {
        std::sin(yaw_rad) * std::sin(pitch_rad),
        std::cos(pitch_rad),
        std::cos(yaw_rad) * std::sin(pitch_rad)
    };

    // Apply movement
    const float speed = freecam_speed * delta;
    camera.position += forward * movement.y * speed;
    camera.position += right * movement.x * speed;

    // Vertical movement
    if (window.get_key(SDL_SCANCODE_Q)) camera.position -= up * speed;
    if (window.get_key(SDL_SCANCODE_E)) camera.position += up * speed;

    // Mouse Look
    if (mouse_captured)
    {
        const float sensitivity = 0.2f;
        const glm::vec2 mouse_movement = window.get_mouse_movement();
        camera.yaw += mouse_movement.x * sensitivity;
        camera.pitch += mouse_movement.y * sensitivity;
        camera.pitch = std::max(std::min(camera.pitch, 89.0f), -89.0f);
    }

    // Cursor Toggle
    if (window.get_key_pressed(SDL_SCANCODE_LSHIFT))
    {
        mouse_captured = !mouse_captured;
        if (mouse_captured)
            window.capture_mouse();
        else
            window.uncapture_mouse();
    }
}


World::~World()
{
    delete job_system;
    delete player;
    delete map;
}