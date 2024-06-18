#version 330 core

layout (location = 0) in vec3 vertices;
uniform mat4 matrix;
out vec2 out_texture_coords;

void main()
{
    gl_Position = matrix * vec4(vertices, 1.0);
}
