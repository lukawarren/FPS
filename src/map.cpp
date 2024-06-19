#include "map.h"

constexpr double metres_per_unit = 0.01905;
constexpr csg::volume_t volume_air = 0;
constexpr csg::volume_t volume_solid = 1;
constexpr float texture_size = 128.0f;

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

    build_mesh();
}

void Map::parse_entity(std::ifstream& stream)
{
    std::string line;
    std::getline(stream, line);

    // Get properties
    std::string class_name = line.substr(13, line.length() - 14);
    dbg(class_name);

    while (std::getline(stream, line))
    {
        // Brush data
        if (line == "{")
            parse_brush(stream);

        // End of entity
        if (line == "}")
            return;
    }
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

            texture_info_indices.push_back(texture_infos.size() - 1);
        }
    }
}

void Map::build_mesh()
{
    world.rebuild();
    csg::brush_t* brush = world.first();
    size_t face_index = 0;

    while (brush != nullptr)
    {
        // Retrieve texture info (per face)
        std::vector<size_t> texture_info_indices = std::any_cast<std::vector<size_t>>(brush->userdata);

        auto faces = brush->get_faces();
        for (const csg::face_t& face : faces)
        {
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
                const size_t offset = vertices.size() / 3;
                for (const auto& vertex : fragment.vertices)
                {
                    vertices.push_back(vertex.position.x * metres_per_unit);
                    vertices.push_back(vertex.position.z * metres_per_unit);
                    vertices.push_back(vertex.position.y * metres_per_unit * -1.0f);
                }

                // Record normals
                for (size_t i = 0; i < fragment.vertices.size(); ++i)
                {
                    normals.push_back(face.plane->normal.x);
                    normals.push_back(face.plane->normal.z);
                    normals.push_back(face.plane->normal.y);
                }

                // Record indices
                std::vector<csg::triangle_t> triangles = csg::triangulate(fragment);
                for (const auto& triangle : triangles)
                {
                    indices.push_back(triangle.i + offset);
                    indices.push_back(triangle.j + offset);
                    indices.push_back(triangle.k + offset);
                }

                const TextureInfo& info  = texture_infos[face_index];
                calculate_uvs(fragment.vertices, info);
            }

            face_index++;
        }

        brush = world.next(brush);
    }
}

void Map::calculate_uvs(const std::vector<csg::vertex_t>& vertices, const TextureInfo& info)
{
    const glm::vec3 u_axis = info.axes[0].normal / info.scale.x;
    const glm::vec3 v_axis = info.axes[1].normal / info.scale.y;

    for (const auto& vertex : vertices)
    {
        float u = glm::dot(vertex.position, u_axis);
        float v = glm::dot(vertex.position, v_axis);
        u += info.axes[0].offset;
        v += info.axes[1].offset;
        u /= texture_size;
        v /= texture_size;

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
