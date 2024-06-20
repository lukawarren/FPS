#pragma once
#include "camera.h"
#include "map.h"
#include "config.h"

struct PointLight
{
    glm::vec3 position;
    glm::vec3 colour;
    float distance = 100.0f;
};

struct World
{
    Camera camera;
    Map* map = nullptr;
    std::vector<PointLight> point_lights;

    World() {}

    World(const World&) = delete;

    void load_map(const std::string& filename)
    {
        if (map != nullptr) delete map;
        map = new Map(filename);

        // Extract entities
        for (const auto& entity : map->entities)
        {
            if (entity.properties.count("classname") == 0) continue;
            const std::string& class_name = entity.properties.at("classname");

            if (class_name == "light")
            {
                const glm::vec3 position = entity.parse_vec3("origin");
                const glm::vec3 colour = entity.parse_vec3("light_colour", glm::vec3(1.0f), false);
                dbg(colour.x);
                dbg(colour.y);
                dbg(colour.z);
                point_lights.emplace_back(position, colour);
            }

            else if (class_name == "info_player_start")
            {
                const glm::vec3 position = entity.parse_vec3("origin");
                camera.position = position;
            }
        }
    }

    ~World()
    {
        delete map;
    }
};
