#pragma once
#include "pch.h"

class Mesh
{
public:
    Mesh(
        const std::vector<float>& vertices,
        const std::vector<float>& texture_coordinates,
        const std::vector<float>& normals,
        const std::vector<unsigned int>& indices
    );
    Mesh(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices
    );
    Mesh(const std::string& filename);
    Mesh(const Mesh&) = delete;
    ~Mesh();

    void bind() const;
    void unbind() const;
    void draw() const;

private:
    void construct(
        const std::vector<float>& vertices,
        const std::vector<float>& texture_coordinates,
        const std::vector<float>& normals,
        const std::vector<unsigned int>& indices
    );

    void make_vao(
        const unsigned int attribute,
        const unsigned int format,
        const unsigned int dimensions,
        const std::vector<float>& data
    );

    void make_mesh_from_assimp(const aiMesh* assimp_mesh);

    // OpenGL state
    unsigned int vao;
    std::vector<unsigned int> vbos;
    unsigned int ebo;
    size_t n_indices;
};