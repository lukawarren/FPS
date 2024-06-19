#version 330 core

layout (location = 0) in vec3 vertices;
layout (location = 1) in vec2 texture_coordinates;
layout (location = 2) in vec3 normals;

uniform mat4 matrix;

out vec2 out_texture_coordinates;

void main()
{
    gl_Position = matrix * vec4(vertices, 1.0);
    out_texture_coordinates = texture_coordinates;
}
