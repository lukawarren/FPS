#pragma once
#include "common.h"
#include "mesh.h"
#include "texture.h"

constexpr static inline float METRES_PER_UNIT = 0.01905;
constexpr static inline float TRENCHBROOM_TEXTURE_SIZE = 128.0f;

class Map
{
public:
    Map(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
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
                return glm::vec3 { x, z, -y } * METRES_PER_UNIT;

            return { x, y, z };
        }

        float parse_float(
            const std::string& key,
            const float default_value = 0.0f
        ) const
        {
            if (properties.count(key) == 0)
                return default_value;

            std::istringstream iss(properties.at(key));
            float x;
            iss >> x;
            return x;
        }

        bool parse_bool(
            const std::string& key,
            const bool default_value
        ) const
        {
            if (properties.count(key) == 0)
                return default_value;

            std::istringstream iss(properties.at(key));
            bool x;
            iss >> x;
            return x;
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

    void parse_entity(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    void parse_brush(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    void build_meshes(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
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
