#pragma once
#include "pch.h"
#include "render/mesh.h"
#include "render/texture.h"

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
    std::vector<DrawCall> draw_calls;

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

    csg::world_t world;

    // Temporary variables
    std::set<std::string> textures;
    std::vector<TextureInfo> texture_infos;

    std::pair<glm::vec3, float> plane_from_points(
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& p3
    ) const;
};
