#pragma once
#include "common.h"
#include "render/mesh.h"
#include "render/texture.h"

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
        ) const;

        glm::vec3 parse_angles(
            const std::string& key,
            const glm::vec3 default_value = {}
        ) const;

        glm::vec3 parse_angle(
            const std::string& key,
            const float default_value = 0.0f
        ) const;

        float parse_float(
            const std::string& key,
            const float default_value = 0.0f
        ) const;

        bool parse_bool(
            const std::string& key,
            const bool default_value
        ) const;
    };

    std::vector<glm::vec3> find_path(const glm::vec3& start, const glm::vec3& end) const;

    std::vector<DrawCall> draw_calls;
    std::vector<Entity> entities;
    JPH::Ref<JPH::Shape> physics_shape;

    constexpr static inline float METRES_PER_UNIT = 0.0254;
    constexpr static inline float TRENCHBROOM_TEXTURE_SIZE = 128.0f;

private:
    struct TextureInfo
    {
        std::string name;
        std::array<csg::plane_t, 2> axes;
        glm::vec2 scale;
    };

    struct TexturedMesh
    {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texture_coordinates;
        std::vector<unsigned int> indices;
    };

    void parse_entity(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    void parse_brush(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    void build_meshes(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    void calculate_uvs(
        std::vector<float>& texture_coordinates,
        const std::vector<csg::vertex_t>& vertices,
        const TextureInfo& info
    );
    void extract_meshes_from_world(
        csg::world_t* src_world,
        std::unordered_map<std::string, TexturedMesh>& meshes
    );

    // Temporary variables
    std::set<std::string> textures;
    std::vector<TextureInfo> texture_infos;
    csg::world_t* solid_world = nullptr;
    csg::world_t* transparent_world = nullptr;

    // Navigation
    dtNavMesh* nav_mesh = nullptr;
    dtNavMeshQuery* nav_query = nullptr;

    std::pair<glm::vec3, float> plane_from_points(
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& p3
    ) const;

    void build_navmesh(const std::vector<float>& vertices, const std::vector<int>& indices);
};
