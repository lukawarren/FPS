#pragma once
#include "common.h"
#include "camera.h"
#include "player.h"
#include "spotlight.h"
#include "map.h"
#include "physics.h"

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

    Camera camera;
    Player* player = nullptr;
    Map* map = nullptr;

    // Lighting
    std::vector<Spotlight> spotlights;

private:
    void setup_physics();

    // Physics
    JPH::JobSystemThreadPool* job_system;
    JPH::PhysicsSystem physics_system;
    BPLayerInterfaceImpl broad_phase_layer_interface;
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    JPH::TempAllocatorMalloc allocator;
    JPH::Ref<JPH::CharacterVirtual> character;
};
