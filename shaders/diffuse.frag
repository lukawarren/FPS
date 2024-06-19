#version 330 core

in vec3 out_normal;
out vec4 out_colour;

void main()
{
    out_colour = vec4(normalize(out_normal), 1);
    out_colour = vec4(max(dot(normalize(out_normal), vec3(0.3,1,0)), 0.2) * vec3(1,1,1), 1.0);
}
