#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 matrices[6];

out vec4 out_position;

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face;

        // For each triangle
        for(int i = 0; i < 3; ++i)
        {
            out_position = gl_in[i].gl_Position;
            gl_Position = matrices[face] * out_position;
            EmitVertex();
        }
        EndPrimitive();
    }
}