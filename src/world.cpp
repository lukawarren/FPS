#include "world.h"

World::World(
    const std::string& filename,
    Window* window,
    SDL_GPUDevice* device,
    SDL_GPUCopyPass* copy_pass
) : player(window)
{
    map = new Map(filename, device, copy_pass);

    // Extract entities
    for (const auto& entity : map->entities)
    {
        if (entity.properties.count("classname") == 0) continue;
        const std::string& class_name = entity.properties.at("classname");

        if (class_name == "light_spotlight")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            const glm::vec3 colour = entity.parse_vec3("colour", glm::vec3(255.0f), false);
            const float intensity = entity.parse_float("intensity", 10.0f);
            const glm::vec3 angles = entity.parse_vec3("angles", { 0.0f, -1.0f, 0.0f }, false);
            const float near = entity.parse_float("near", 0.1f);
            const float far = entity.parse_float("far", 10.0f);
            const float angle = entity.parse_float("angle", 60.0f);

            spotlights.emplace_back(
                position,
                glm::normalize(angles),
                glm::normalize(colour / 255.0f) * intensity,
                near,
                far,
                glm::radians(angle)
            );
        }

        else if (class_name == "info_player_start")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            player.set_position(position);
        }
    }

    player.setup_physics(map->world);
}

World::~World()
{
    delete map;
}