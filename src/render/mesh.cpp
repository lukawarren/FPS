#include "render/mesh.h"

Mesh::Mesh(
    const std::vector<uint32_t>& vertex_data,
    const std::vector<uint8_t>& lighting_data
)
{
    // Create and bind VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Make VAOs
    make_vao(
        0,
        GL_UNSIGNED_INT,
        1,
        &vertex_data[0],
        sizeof(vertex_data[0]) * vertex_data.size(),
        sizeof(vertex_data[0])
    );
    make_vao(
        1,
        GL_UNSIGNED_BYTE,
        1,
        &lighting_data[0],
        sizeof(lighting_data[0]) * lighting_data.size(),
        sizeof(lighting_data[0])
    );

    // Unbind VAO but *not* EBO (as this is bound by the VAO for us)
    glBindVertexArray(0);
    draw_count = vertex_data.size();
}

Mesh::Mesh(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices,
    const std::vector<float>& texture_coords,
    const std::optional<std::vector<uint8_t>>& lighting_values
)
{
    // Create and bind VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create, bind and upload indices' element buffer object (EBO)
    unsigned int gl_ebo;
    glGenBuffers(1, &gl_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);
    ebo = gl_ebo;

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
        2,
        &texture_coords[0],
        sizeof(texture_coords[0]) * texture_coords.size(),
        sizeof(texture_coords[0])
    );
    if (lighting_values.has_value())
    {
        make_vao(
            2,
            GL_UNSIGNED_BYTE,
            1,
            &lighting_values.value()[0],
            sizeof(lighting_values.value()[0]) * lighting_values.value().size(),
            sizeof(lighting_values.value()[0])
        );
    }

    // Unbind VAO but *not* EBO (as this is bound by the VAO for us)
    glBindVertexArray(0);
    this->draw_count = indices.size();
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
