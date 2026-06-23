#pragma once
#include "common.h"
#include "camera.h"
#include "player.h"
#include "spotlight.h"
#include "map.h"

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

    Camera camera;
    Player player;
    Map* map = nullptr;

    // Lighting
    std::vector<Spotlight> spotlights;
};
