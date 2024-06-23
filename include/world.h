#pragma once
#include "camera.h"
#include "map.h"
#include "config.h"
#include "player.h"
#include "transform.h"

struct PointLight
{
    glm::vec3 position;
    glm::vec3 colour;
    bool dynamic;
    float distance = 24.0f;
};

struct Entity
{
    Transform transform = {};
    static auto get_mesh() { return "door.obj"; }
    static auto get_texture() { return "door.jpg"; }
};

struct World
{
    Camera camera = {};
    Player player = {};
    Map* map = nullptr;
    std::vector<PointLight> point_lights;
    std::vector<Entity> entities;

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
                const glm::vec3 colour = entity.parse_vec3("colour", glm::vec3(1.0f), false);
                const bool dynamic = entity.parse_bool("dynamic", false);
                point_lights.emplace_back(position, colour, dynamic);
            }

            else if (class_name == "info_player_start")
            {
                const glm::vec3 position = entity.parse_vec3("origin");
                camera.position = position;
            }

            else if (class_name == "door")
            {
                const glm::vec3 position = entity.parse_vec3("origin");
                const float angle = entity.parse_float("angle");

                Entity& e = entities.emplace_back();
                e.transform.position = position;
                e.transform.rotation.y = angle;
            }
        }
    }

    ~World()
    {
        delete map;
    }
};
