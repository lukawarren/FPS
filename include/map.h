#pragma once
#include "pch.h"

class Map
{
public:
    Map(const std::string& filename);
    std::vector<float> get_vertices() const;
    std::vector<float> get_normals() const;

private:
    void parse_entity(std::ifstream& stream);
    void parse_brush(std::ifstream& stream);
    void build_polygons();
    void reorder_polygons();
    void convert_to_triangles();

    std::optional<glm::dvec3> get_intersection(
        const glm::dvec3& n1,
        const glm::dvec3& n2,
        const glm::dvec3& n3,
        const double d1,
        const double d2,
        const double d3
    ) const;

    std::pair<glm::dvec3, double> plane_from_points(
        const glm::dvec3& p1,
        const glm::dvec3& p2,
        const glm::dvec3& p3
    ) const;

    std::optional<std::pair<glm::dvec3, double>> plane_from_points(
        const std::vector<glm::dvec3>& points
    ) const;

    enum class PlaneClassification
    {
        FRONT,
        BACK,
        SPANNING
    };

    PlaneClassification classify_point(
        const glm::dvec3& normal,
        const double distance,
        const glm::dvec3& point
    ) const;

    struct Polygon
    {
        std::vector<glm::dvec3> vertices;
        glm::dvec3 normal;
    };

    struct Face
    {
        // Geometry
        std::vector<Polygon> polygons = {};
        glm::dvec3 normal;
        double distance;

        // Texture
        std::string texture_name;
        glm::vec2 texture_offset;
        float texture_rotation;
        glm::vec2 texture_scale;
    };

    struct Brush
    {
        std::vector<Face> faces;
    };

    std::vector<Brush> brushes;
};
