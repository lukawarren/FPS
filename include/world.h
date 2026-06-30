#pragma once
#include "common.h"
#include "camera.h"
#include "player.h"
#include "spotlight.h"
#include "map.h"
#include "physics.h"
#include "decal.h"
#include "enemy.h"

class World
{
public:
    World(
        const std::string& filename,
        Window* window,
        SDL_GPUDevice* device,
        SDL_GPUCopyPass* copy_pass
    );
    ~World();

    void update(const float delta);
    void spawn_decal(const glm::vec3 position, const glm::vec3 direction);
    std::vector<glm::vec3> find_path(const glm::vec3& start, const glm::vec3& end) const;

    Camera camera;
    Player* player = nullptr;
    Map* map = nullptr;

    // Lighting
    std::vector<Spotlight> spotlights;

    // Sprites
    std::deque<Decal> decals;
    std::vector<Enemy> enemies;

    // Physics
    JPH::PhysicsSystem physics_system;
    JPH::TempAllocatorMalloc allocator;

private:
    void setup_physics();

    // Physics
    JPH::JobSystemThreadPool* job_system;
    BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    JPH::Ref<JPH::CharacterVirtual> character;
};
