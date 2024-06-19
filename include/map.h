#pragma once
#include "pch.h"

class Map
{
public:
    Map(const std::string& filename);
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texture_coordinates;
    std::vector<unsigned int> indices;

private:
    struct TextureInfo
    {
        std::string name;
        std::array<csg::plane_t, 2> axes;
        glm::vec2 scale;
    };

    void parse_entity(std::ifstream& stream);
    void parse_brush(std::ifstream& stream);
    void build_mesh();
    void calculate_uvs(const std::vector<csg::vertex_t>& vertices, const TextureInfo& info);

    csg::world_t world;

    std::vector<TextureInfo> texture_infos;

    std::pair<glm::vec3, float> plane_from_points(
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& p3
    ) const;
};
