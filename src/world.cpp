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
            const glm::vec3 colour = entity.parse_vec3("colour", glm::vec3(255.0f, 170.0f, 95.0f), false);
            const float intensity = entity.parse_float("intensity", 50.0f);
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
        }

        else if (class_name == "info_player_start")
        {
            const glm::vec3 position = entity.parse_vec3("origin");
            const float angle = entity.parse_float("angle");
            player.set_position(position);
            player.head_yaw = 90.0f - angle;
        }
    }

    player.setup_physics(map->world);
}

World::~World()
{
    delete map;
}