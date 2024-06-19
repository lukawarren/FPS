#pragma once
#include "pch.h"

class Mesh
{
public:
    Mesh(
        const std::vector<float>& vertices,
        const std::vector<float>& normals,
        const std::vector<unsigned int>& indices
    );
    Mesh(const Mesh&) = delete;
    ~Mesh();

    void bind() const;
    void unbind() const;
    void draw() const;

private:
    void make_vao(
        const unsigned int attribute,
        const unsigned int format,
        const unsigned int dimensions,
        const std::vector<float>& data
    );

    // OpenGL state
    unsigned int vao;
    std::vector<unsigned int> vbos;
    unsigned int ebo;
    size_t n_indices;
};