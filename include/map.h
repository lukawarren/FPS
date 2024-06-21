#pragma once
#include "pch.h"
#include "render/mesh.h"
#include "render/texture.h"
#include "config.h"

class Map
{
public:
    Map(const std::string& filename);
    ~Map();

    struct DrawCall
    {
        Mesh* mesh;
        Texture* texture;
    };

    struct Entity
    {
        std::unordered_map<std::string, std::string> properties;

        glm::vec3 parse_vec3(
            const std::string& key,
            const glm::vec3 default_value = glm::vec3(0.0f),
            const bool scale = true
        ) const
        {
            if (properties.count(key) == 0)
                return default_value;

            std::istringstream iss(properties.at(key));
            float x, y, z;
            iss >> x >> y >> z;

            if (scale)
                return glm::vec3 { x, z, -y } * metres_per_unit;

            return { x, y, z };
        }
    };

    std::vector<DrawCall> draw_calls;
    std::vector<Entity> entities;
    csg::world_t world;

private:
    struct TextureInfo
    {
        std::string name;
        std::array<csg::plane_t, 2> axes;
        glm::vec2 scale;
    };

    void parse_entity(std::ifstream& stream);
    void parse_brush(std::ifstream& stream);
    void build_meshes();
    void calculate_uvs(
        std::vector<float>& texture_coordinates,
        const std::vector<csg::vertex_t>& vertices,
        const TextureInfo& info
    );

    // Temporary variables
    std::set<std::string> textures;
    std::vector<TextureInfo> texture_infos;

    std::pair<glm::vec3, float> plane_from_points(
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& p3
    ) const;
};
