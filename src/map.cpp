#include "map.h"

constexpr double epsilon = 0.00001f;
bool equals(const float a, const float b);

Map::Map(const std::string& filename)
{
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

    // Build geometry
    build_polygons();
    reorder_polygons();
    convert_to_triangles();
}

std::vector<float> Map::get_vertices() const
{
    std::vector<float> vertices;
    for (const auto& brush : brushes)
    {
        for (const auto& face : brush.faces)
        {
            for (const auto& polygon : face.polygons)
            {
                for (const auto& vertex : polygon.vertices)
                {
                    vertices.push_back(float(vertex.x * 0.01905));
                    vertices.push_back(float(vertex.z * 0.01905));
                    vertices.push_back(float(vertex.y * 0.01905));
                }
            }
        }
    }

    return vertices;
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
    std::vector<Face> faces;
    while (std::getline(stream, line))
    {
        // End of brush
        if (line == "}")
        {
            brushes.emplace_back(faces);
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

            const std::array<glm::dvec3, 3> vertices = {
                parse_vertex(iss),
                parse_vertex(iss),
                parse_vertex(iss)
            };

            // Convert three vertices to a plane, consisting of a normal and a direction
            const std::pair<glm::dvec3, double> plane = plane_from_points(
                vertices[0],
                vertices[1],
                vertices[2]
            );

            // Get texture info
            std::string texture_name;
            float shift_x, shift_y, rotation, scale_x, scale_y;
            iss >> texture_name >> shift_x >> shift_y >> rotation >> scale_x >> scale_y;

            faces.push_back(Face {
                .normal = plane.first,
                .distance = plane.second,
                .texture_name = texture_name,
                .texture_offset = { shift_x, shift_y },
                .texture_rotation = rotation,
                .texture_scale = { scale_x, scale_y }
            });
        }
    }
}

void Map::build_polygons()
{
    for (auto& brush : brushes)
    {
        for (auto& face : brush.faces)
        {
            std::vector<Polygon> polygons;
            polygons.resize(brush.faces.size());

            for (size_t i = 0; i < brush.faces.size() - 2; i++)
            {
                for (size_t j = i; j < brush.faces.size() - 1; ++j)
                {
                    for (size_t k = j; k < brush.faces.size(); ++k)
                    {
                        // If any of i, j or k are the same, ignore
                        if (i == j || i == k || j == k) continue;

                        const std::optional<glm::dvec3> vertex = get_intersection(
                            brush.faces[i].normal,
                            brush.faces[j].normal,
                            brush.faces[k].normal,
                            brush.faces[i].distance,
                            brush.faces[j].distance,
                            brush.faces[k].distance
                        );

                        if (!vertex.has_value()) continue;
                        bool legal = true;

                        for (size_t m = 0; m < brush.faces.size(); ++m)
                        {
                            if (glm::dot(brush.faces[m].normal, *vertex) + brush.faces[m].distance > 0.0)
                                legal = false;
                        }

                        if (legal)
                        {
                            polygons[i].normal = brush.faces[i].normal;
                            polygons[j].normal = brush.faces[j].normal;
                            polygons[k].normal = brush.faces[k].normal;
                            polygons[i].vertices.push_back(*vertex);
                            polygons[j].vertices.push_back(*vertex);
                            polygons[k].vertices.push_back(*vertex);
                        }
                    }
                }
            }

            face.polygons = polygons;
        }
    }

    // Check each polygon has at least 3 vertices
    for (const auto& brush : brushes)
        for (const auto& face : brush.faces)
            if (face.polygons.size() < 3)
                throw std::runtime_error("invalid brush");
}

void Map::reorder_polygons()
{
    for (auto& brush : brushes)
    {
        for (auto& face : brush.faces)
        {
            for (auto& polygon : face.polygons)
            {
                // Find centre
                glm::dvec3 centre(0.0f);
                for (const auto& vertex : polygon.vertices)
                    centre += vertex;
                centre /= polygon.vertices.size();

                // Reorder
                for (size_t i = 0; i < polygon.vertices.size() - 2; ++i)
                {
                    const glm::dvec3 a = glm::normalize(polygon.vertices[i] - centre);
                    const std::pair<glm::dvec3, double> plane = plane_from_points(
                        polygon.vertices[i],
                        centre,
                        centre + polygon.normal
                    );

                    double smallest_angle = -1.0;
                    size_t smallest_index = 0;

                    for (size_t m = i + 1; m < polygon.vertices.size(); ++m)
                    {
                        if (classify_point(plane.first, plane.second, polygon.vertices[0]) != PlaneClassification::BACK)
                        {
                            const glm::dvec3 b = glm::normalize(polygon.vertices[m] - centre);
                            const double angle = glm::dot(a, b);

                            if (angle > smallest_angle)
                            {
                                smallest_angle = angle;
                                smallest_index = m;
                            }
                        }
                    }

                    // Swap
                    const glm::dvec3 temp = polygon.vertices[i + 1];
                    polygon.vertices[i + 1] = polygon.vertices[smallest_index];
                    polygon.vertices[smallest_index] = temp;
                }

                // Reverse if need be
                const auto new_normal = plane_from_points(polygon.vertices);
                if (new_normal.has_value() && glm::dot(new_normal.value().first, polygon.normal) < epsilon)
                    std::reverse(polygon.vertices.begin(), polygon.vertices.end());
            }
        }
    }
}

void Map::convert_to_triangles()
{
    for (auto& brush : brushes)
    {
        for (auto& face : brush.faces)
        {
            std::vector<Polygon> new_polygons;

            for (auto& polygon : face.polygons)
            {
                if (polygon.vertices.size() < 3) continue;

                for (size_t i = 1; i < polygon.vertices.size() - 1; ++i)
                {
                    Polygon triangle;
                    triangle.vertices.push_back(polygon.vertices[0]);
                    triangle.vertices.push_back(polygon.vertices[i]);
                    triangle.vertices.push_back(polygon.vertices[i + 1]);
                    triangle.normal = polygon.normal;
                    new_polygons.push_back(triangle);
                }
            }

            face.polygons = new_polygons;
        }
    }
}

std::optional<glm::dvec3> Map::get_intersection(
    const glm::dvec3& n1,
    const glm::dvec3& n2,
    const glm::dvec3& n3,
    const double d1,
    const double d2,
    const double d3
) const
{
    const double denominator = glm::dot(n1, glm::cross(n2, n3));
    if (equals(denominator, 0)) return std::nullopt;

    glm::vec3 intersection =
        -d1 * glm::cross(n2, n3) +
        -d2 * glm::cross(n3, n1) +
        -d3 * glm::cross(n1, n2);
    intersection /= denominator;

    return intersection;
}

std::pair<glm::dvec3, double> Map::plane_from_points(
    const glm::dvec3& p1,
    const glm::dvec3& p2,
    const glm::dvec3& p3
) const
{
    const glm::dvec3 normal = glm::normalize(glm::cross(
        p2 - p1,
        p3 - p1
    ));
    const double distance = glm::dot(p1, normal);
    return { normal, distance };
}

std::optional<std::pair<glm::dvec3, double>> Map::plane_from_points(const std::vector<glm::dvec3>& points) const
{
    glm::dvec3 normal(0.0);
    glm::dvec3 centre_of_mass(0.0);

    if (points.size() < 3)
        return std::nullopt;

    for (size_t i = 0; i < points.size(); ++i)
    {
        size_t j = i + 1;
        if (j >= points.size())
            j = 0;

        normal.x += (points[i].y - points[j].y) * (points[i].z - points[j].z);
        normal.y += (points[i].z - points[j].z) * (points[i].x - points[j].x);
        normal.z += (points[i].x - points[j].x) * (points[i].y - points[j].y);

        centre_of_mass += points[i];
    }

    if (fabs(normal.x) < epsilon && fabs(normal.y) < epsilon && fabs(normal.z) < epsilon)
        return std::nullopt;

    const double magnitude = glm::length(normal);
    if (magnitude < epsilon)
        return std::nullopt;

    normal /= magnitude;
    centre_of_mass /= (double)points.size();

    return std::pair<glm::dvec3, double> {
        normal, -glm::dot(centre_of_mass, normal)
    };
}

Map::PlaneClassification Map::classify_point(
    const glm::dvec3& normal,
    const double distance,
    const glm::dvec3& point
) const
{
    const double x = glm::dot(normal, point) + distance;

    if (x > epsilon)
        return PlaneClassification::FRONT;

    if (x < -epsilon)
        return PlaneClassification::BACK;

    return PlaneClassification::SPANNING;
}

bool equals(const float a, const float b)
{
    return fabs(a - b) < epsilon;
}