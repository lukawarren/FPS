#include "map.h"

constexpr csg::volume_t volume_air = 0;
constexpr csg::volume_t volume_solid = 1;

Map::Map(const std::string& filename)
{
    // Setup CSG "world"
    world.set_void_volume(volume_air);

    // Load text file
    std::ifstream file("../assets/maps/" + filename + ".map");
    if (!file) throw std::runtime_error("unable to load map " + filename);

    // Parse
    std::string line;
    while (std::getline(file, line))
    {
        if (line == "{")
            parse_entity(file);

        continue;
    }

    build_meshes();
    textures.clear();
    texture_infos.clear();
}

void Map::parse_entity(std::ifstream& stream)
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
            parse_brush(stream);
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

void Map::parse_brush(std::ifstream& stream)
{
    std::string line;
    csg::brush_t* brush = world.add();
    std::vector<csg::plane_t> planes;
    std::vector<size_t> texture_info_indices;

    // Brush setup
    brush->set_volume_operation(csg::make_fill_operation(volume_solid));

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

void Map::build_meshes()
{
    world.rebuild();
    csg::brush_t* brush = world.first();
    size_t face_index = 0;

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
        for (const csg::face_t& face : faces)
        {
            // Identify (or create) correct mesh
            const TextureInfo& info  = texture_infos[face_index];
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
                if (front != volume_air)
                    continue;

                // Record vertices
                const size_t offset = mesh.vertices.size() / 3;
                for (const auto& vertex : fragment.vertices)
                {
                    mesh.vertices.push_back(vertex.position.x * metres_per_unit);
                    mesh.vertices.push_back(vertex.position.z * metres_per_unit);
                    mesh.vertices.push_back(vertex.position.y * metres_per_unit * -1.0f);
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

            face_index++;
        }

        brush = world.next(brush);
    }

    for (const auto &[key, value] : meshes)
    {
        draw_calls.emplace_back(DrawCall {
            .mesh = new Mesh(
                value.vertices,
                value.texture_coordinates,
                value.normals,
                value.indices
            ),
            .texture = new Texture(key + ".png")
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
        u /= (float)trenchbroom_texture_size;
        v /= (float)trenchbroom_texture_size;

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
