#version 330 core

layout (location = 0) in vec3 vertices;
layout (location = 1) in vec2 texture_coordinates;
layout (location = 2) in vec3 normals;

uniform mat4 view_projection;
uniform mat4 model;
uniform mat3 normal;

out vec4 out_position;
out vec2 out_texture_coordinates;
out vec3 out_normal;

void main()
{
    out_position = model * vec4(vertices, 1.0);
    gl_Position = view_projection * out_position;
    out_texture_coordinates = texture_coordinates;
    out_normal = normal * normals;
}
