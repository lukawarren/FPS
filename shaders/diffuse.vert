#version 330 core

layout (location = 0) in vec3 vertices;
layout (location = 1) in vec2 texture_coords;

uniform mat4 matrix;

out vec2 out_texture_coords;

void main()
{
    out_texture_coords = texture_coords;
    gl_Position = matrix * vec4(vertices, 1.0);
}
