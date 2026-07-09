#pragma once
#include "common.h"
#include "camera.h"
#include "player.h"
#include "map.h"
#include "physics.h"
#include "decal.h"
#include "animated_sprite.h"
#include "audio.h"
#include "entity.h"

class World
{
public:
    World(
        const std::string& filename,
        SDL_GPUDevice* device,
        SDL_GPUCopyPass* copy_pass,
        Window& window,
        Audio& audio
    );
    ~World();

    struct Hit
    {
        glm::vec3 position;
        glm::vec3 normal;
        std::optional<JPH::BodyID> body_id;
    };

    void update(const float delta);
    void spawn_decal(const glm::vec3 position, const glm::vec3 direction);
    std::vector<glm::vec3> find_path(const glm::vec3& start, const glm::vec3& end) const;
    std::optional<Hit> get_hit(
        const glm::vec3& origin,
        const glm::vec3& forward,
        const float max_distance,
        const std::optional<JPH::BodyID> ignore = std::nullopt
    ) const;

    Camera camera;
    Player* player = nullptr;
    Map* map = nullptr;
    Audio& audio;

    // Lighting
    std::vector<Spotlight> spotlights;

    // Objects; enemies (entities) need constant addresses for physics user pointer
    std::deque<Decal> decals;
    std::vector<std::unique_ptr<Entity>> entities;

    // Physics
    JPH::PhysicsSystem physics_system;
    JPH::TempAllocatorMalloc allocator;

private:
    void setup_physics();
    void update_debug_mode(const float delta);
    void update_freecam(const float delta);

    Window& window;
    float time = 0.0f;
    bool debug_mode = false;
    bool mouse_captured = true;

    // Physics
    JPH::JobSystemThreadPool* job_system;
    BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;
};
