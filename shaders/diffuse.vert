#version 330 core

layout (location = 0) in vec3 vertices;
layout (location = 1) in vec3 normals;

uniform mat4 matrix;

out vec3 out_normal;

void main()
{
    gl_Position = matrix * vec4(vertices, 1.0);
    out_normal = normals;
}
