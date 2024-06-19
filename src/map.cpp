#include "map.h"

constexpr double metres_per_unit = 0.01905;
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

    // Build mesh
    world.rebuild();
    csg::brush_t* brush = world.first();
    while (brush != nullptr)
    {
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
                    vertices.push_back(vertex.position.y * metres_per_unit);
                }

                // Record indices
                std::vector<csg::triangle_t> triangles = csg::triangulate(fragment);
                for (const auto& triangle : triangles)
                {
                    indices.push_back(triangle.i + offset);
                    indices.push_back(triangle.j + offset);
                    indices.push_back(triangle.k + offset);
                }
            }
        }

        brush = world.next(brush);
    }
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

    // Brush setup
    brush->set_volume_operation(csg::make_fill_operation(volume_solid));

    while (std::getline(stream, line))
    {
        // End of brush
        if (line == "}")
        {
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
                double x, y, z;
                iss >> x >> y >> z;
                iss.ignore(3);
                return glm::dvec3(x, y, z);
            };

            // Convert three vertices to a plane, consisting of a normal and a direction
            const std::pair<glm::vec3, float> plane = plane_from_points(
                parse_vertex(iss),
                parse_vertex(iss),
                parse_vertex(iss)
            );

            // Get texture info
            std::string texture_name;
            float shift_x, shift_y, rotation, scale_x, scale_y;
            iss >> texture_name >> shift_x >> shift_y >> rotation >> scale_x >> scale_y;

            planes.push_back(csg::plane_t {
                .normal = plane.first,
                .offset = plane.second
                // .texture_name = texture_name,
                // .texture_offset = { shift_x, shift_y },
                // .texture_rotation = rotation,
                // .texture_scale = { scale_x, scale_y }
            });
        }
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
