#include "render/mesh.h"

Mesh::Mesh(
    const std::vector<float>& vertices,
    const std::vector<float>& normals
)
{
    dbg("TODO: use EBOs");
    // Create and bind VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Make VAOs
    make_vao(
        0,
        GL_FLOAT,
        3,
        &vertices[0],
        sizeof(vertices[0]) * vertices.size(),
        sizeof(vertices[0])
    );
    make_vao(
        1,
        GL_FLOAT,
        3,
        &normals[0],
        sizeof(normals[0]) * normals.size(),
        sizeof(normals[0])
    );

    // Unbind VAO but *not* EBO (as this is bound by the VAO for us)
    glBindVertexArray(0);
    draw_count = vertices.size();
}

void Mesh::make_vao(
    const unsigned int attribute,
    const unsigned int format,
    const unsigned int dimensions,
    const void* data,
    const size_t length,
    const size_t unit_length
)
{
    // Make and fill VBO
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, length, data, GL_STATIC_DRAW);

    // Inform OpenGL of the nature of the data - GL_FALSE disables normalisation of data
    if (format == GL_FLOAT)
        glVertexAttribPointer(attribute, dimensions, format, GL_FALSE, dimensions * unit_length, (void*)0);
    else
        glVertexAttribIPointer(attribute, dimensions, format, dimensions * unit_length, (void*)0);

    glEnableVertexAttribArray(attribute);

    // Unbind then keep track of VBO for future clean-up
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vbos.push_back(vbo);
}

void Mesh::bind() const
{
    glBindVertexArray(vao);
}

void Mesh::unbind() const
{
    glBindVertexArray(0);
}

void Mesh::draw() const
{
    if (ebo.has_value())
        glDrawElements(GL_TRIANGLES, draw_count, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, draw_count);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &vao);

    for (const auto vbo: vbos)
        glDeleteBuffers(1, &vbo);

    if (ebo.has_value())
        glDeleteBuffers(1, &ebo.value());
}
