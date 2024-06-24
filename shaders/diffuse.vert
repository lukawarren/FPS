#version 330 core

layout (location = 0) in vec3 vertices;
layout (location = 1) in vec2 texture_coordinates;
layout (location = 2) in vec3 normals;

struct DirectionalLight
{
    vec3 position;
    vec3 colour;
    mat4 matrix;
    sampler2DShadow depth;
};

uniform mat4 view_projection;
uniform mat4 model;
uniform mat3 normal;
uniform DirectionalLight directional_light;

out vec4 out_position;
out vec2 out_texture_coordinates;
out vec3 out_normal;
out vec4 out_position_directional_light_space;

void main()
{
    vec4 world_space = model * vec4(vertices, 1.0);
    out_position = world_space;
    out_position_directional_light_space = directional_light.matrix * world_space;

    gl_Position = view_projection * out_position;
    out_texture_coordinates = texture_coordinates;
    out_normal = normal * normals;
}
