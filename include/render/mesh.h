#pragma once
#include "pch.h"

class Mesh
{
public:
    Mesh(
        const std::vector<float>& vertices,
        const std::vector<float>& normals
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
        const void* data,
        const size_t length,
        const size_t unit_length
    );

    // OpenGL state
    unsigned int vao;
    std::vector<unsigned int> vbos;
    std::optional<unsigned int> ebo;

    // Mesh info - number of vertices (or indices) for draw commands
    size_t draw_count;
};