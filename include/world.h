#pragma once
#include "camera.h"
#include "map.h"

struct World
{
    Camera camera;
    Map* map = nullptr;

    World() {}

    World(const World&) = delete;

    void load_map(const std::string& filename)
    {
        if (map != nullptr) delete map;
        map = new Map(filename);
    }

    ~World()
    {
        delete map;
    }
};
