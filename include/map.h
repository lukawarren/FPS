#pragma once
#include "pch.h"

class Map
{
public:
    Map(const std::string& filename);
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

private:
    void parse_entity(std::ifstream& stream);
    void parse_brush(std::ifstream& stream);

    csg::world_t world;

    std::pair<glm::vec3, float> plane_from_points(
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& p3
    ) const;
};
