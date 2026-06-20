#include "map.h"

constexpr static inline csg::volume_t VOLUME_AIR = 0;
constexpr static inline csg::volume_t VOLUME_SOLID = 1;

Map::Map(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Setup CSG "world"
    world.set_void_volume(VOLUME_AIR);

    // Load text file
    std::ifstream file(MAP_ROOT + filename);
    if (!file) throw std::runtime_error("Unable to load map " + filename);

    // Parse
    std::string line;
    while (std::getline(file, line))
    {
        if (line == "{")
            parse_entity(file, device, copy_pass);
    }

    build_meshes(device, copy_pass);
    textures.clear();
    texture_infos.clear();
}

void Map::parse_entity(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Get properties
    const auto get_property = [](auto& line)
    {
        std::istringstream iss(line);
        std::string name, value;
        iss >> name;
        iss.ignore(1);
        std::getline(iss, value);

        // Remove surrounding quotes
        if (name[0] == '"') name.erase(0, 1);
        if (name[name.size() - 1] == '"') name.erase(name.size() - 1, 1);
        if (value[0] == '"') value.erase(0, 1);
        if (value[value.size() - 1] == '"') value.erase(value.size() - 1, 1);

        return std::pair {
            name,
            value
        };
    };

    Entity entity = {};

    std::string line;
    while (std::getline(stream, line))
    {
        // Comment
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
            continue;

        // Brush data
        if (line == "{")
        {
            parse_brush(stream, device, copy_pass);
            continue;
        }

        // End of entity
        if (line == "}")
            break;

        // Property
        const auto [name, value] = get_property(line);
        entity.properties[name] = value;
    }

    entities.push_back(entity);
}

void Map::parse_brush(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    std::string line;
    csg::brush_t* brush = world.add();
    std::vector<csg::plane_t> planes;
    std::vector<size_t> texture_info_indices;

    // Brush setup
    brush->set_volume_operation(csg::make_fill_operation(VOLUME_SOLID));

    while (std::getline(stream, line))
    {
        // End of brush
        if (line == "}")
        {
            brush->userdata = texture_info_indices;
            brush->set_planes(planes);
            return;
        }

        else
        {
            // Parse face
            std::istringstream iss(line);

            const auto parse_vertex = [](auto& iss)
            {
                iss.ignore(1);
                float x, y, z;
                iss >> x >> y >> z;
                iss.ignore(3);
                return glm::vec3(x, y, z);
            };

            // Convert three vertices to a plane, consisting of a normal and a direction
            const glm::vec3 a = parse_vertex(iss);
            const glm::vec3 b = parse_vertex(iss);
            const glm::vec3 c = parse_vertex(iss);
            const std::pair<glm::vec3, float> plane = plane_from_points(c, b, a);

            const auto parse_plane = [](auto& iss)
            {
                iss.ignore(2);
                float a, b, c, d;
                iss >> a >> b >> c >> d;
                iss.ignore(3);

                return csg::plane_t {
                    .normal = { a, b, c },
                    .offset = d
                };
            };

            // Get texture info
            std::string texture_name;
            iss >> texture_name;
            const csg::plane_t u_plane = parse_plane(iss);
            const csg::plane_t v_plane = parse_plane(iss);
            float rotation, scale_x, scale_y;
            iss >> rotation >> scale_x >> scale_y;

            // Rotation not needed
            (void)rotation;

            planes.push_back(csg::plane_t {
                .normal = plane.first,
                .offset = plane.second
            });

            texture_infos.emplace_back(TextureInfo {
                .name = texture_name,
                .axes = { u_plane, v_plane },
                .scale = { scale_x, scale_y },
            });

            textures.insert(texture_name);

            texture_info_indices.push_back(texture_infos.size() - 1);
        }
    }
}

void Map::build_meshes(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    world.rebuild();
    csg::brush_t* brush = world.first();

    // Build separates mesh per texture
    struct TexturedMesh
    {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texture_coordinates;
        std::vector<unsigned int> indices;
    };
    std::unordered_map<std::string, TexturedMesh> meshes;

    while (brush != nullptr)
    {
        // Retrieve texture info (per face)
        std::vector<size_t> texture_info_indices = std::any_cast<std::vector<size_t>>(brush->userdata);

        auto faces = brush->get_faces();
        for (size_t local_index = 0; local_index < faces.size(); ++local_index)
        {
            const csg::face_t& face = faces[local_index];

            // "No-draw" faces
            const TextureInfo& info = texture_infos[texture_info_indices[local_index]];
            if (info.name == "__TB_empty")
                continue;

            // Identify (or create) correct mesh
            if (meshes.count(info.name) == 0)
                meshes[info.name] = TexturedMesh {};
            TexturedMesh& mesh = meshes[info.name];

            for (const csg::fragment_t& fragment : face.fragments)
            {
                csg::volume_t front = fragment.front_volume;
                csg::volume_t back = fragment.back_volume;

                // Discard polygons who are "air-air" or "solid-solid"
                if (front == back)
                    continue;

                // Discard non-frontface polygons
                if (front != VOLUME_AIR)
                    continue;

                // Record vertices
                const size_t offset = mesh.vertices.size() / 3;
                for (const auto& vertex : fragment.vertices)
                {
                    mesh.vertices.push_back(vertex.position.x * METRES_PER_UNIT);
                    mesh.vertices.push_back(vertex.position.z * METRES_PER_UNIT);
                    mesh.vertices.push_back(vertex.position.y * METRES_PER_UNIT * -1.0f);
                }

                // Record normals
                for (size_t i = 0; i < fragment.vertices.size(); ++i)
                {
                    mesh.normals.push_back(face.plane->normal.x);
                    mesh.normals.push_back(face.plane->normal.z);
                    mesh.normals.push_back(face.plane->normal.y * -1.0f);
                }

                // Record indices
                std::vector<csg::triangle_t> triangles = csg::triangulate(fragment);
                for (const auto& triangle : triangles)
                {
                    mesh.indices.push_back(triangle.i + offset);
                    mesh.indices.push_back(triangle.j + offset);
                    mesh.indices.push_back(triangle.k + offset);
                }

                calculate_uvs(mesh.texture_coordinates, fragment.vertices, info);
            }
        }

        brush = world.next(brush);
    }

    for (const auto &[key, value] : meshes)
    {
        std::vector<Mesh::Vertex> vertices;
        vertices.reserve(value.vertices.size() / 3);

        for (size_t i = 0; i < value.vertices.size() /  3; i++)
        {
            vertices.emplace_back(Mesh::Vertex {
                .position = { value.vertices[i * 3 + 0], value.vertices[i * 3 + 1], value.vertices[i * 3 + 2] },
                .normal = { value.normals[i * 3 + 0], value.normals[i * 3 + 1], value.normals[i * 3 + 2] },
                .uv = { value.texture_coordinates[i * 2 + 0], value.texture_coordinates[i * 2 + 1] },
            });
        }

        draw_calls.emplace_back(DrawCall {
            .mesh = new Mesh(
                vertices,
                value.indices,
                device,
                copy_pass
            ),
            .texture = new Texture(key + ".png", device, copy_pass)
        });
    }
}

void Map::calculate_uvs(
    std::vector<float>& texture_coordinates,
    const std::vector<csg::vertex_t>& vertices,
    const TextureInfo& info
)
{
    const glm::vec3 u_axis = info.axes[0].normal / info.scale.x;
    const glm::vec3 v_axis = info.axes[1].normal / info.scale.y;

    for (const auto& vertex : vertices)
    {
        float u = glm::dot(vertex.position, u_axis);
        float v = glm::dot(vertex.position, v_axis);
        u += info.axes[0].offset;
        v += info.axes[1].offset;
        u /= TRENCHBROOM_TEXTURE_SIZE;
        v /= TRENCHBROOM_TEXTURE_SIZE;

        texture_coordinates.emplace_back(u);
        texture_coordinates.emplace_back(v);
    }
}

std::pair<glm::vec3, float> Map::plane_from_points(
    const glm::vec3& p1,
    const glm::vec3& p2,
    const glm::vec3& p3) const
{
    const glm::vec3 normal = glm::normalize(glm::cross(
        p3 - p2,
        p1 - p2
    ));
    const float distance = -glm::dot(normal, p1);
    return { normal, distance };
}

Map::~Map()
{
    for (auto& draw_call : draw_calls)
    {
        delete draw_call.mesh;
        delete draw_call.texture;
    }
}