#version 330 core

in vec2 out_texture_coordinates;
uniform sampler2D diffuse;
out vec4 out_colour;

void main()
{
    //out_colour = vec4(normalize(out_normal), 1);
    //out_colour = vec4(max(dot(normalize(out_normal), vec3(0.3,1,0)), 0.2) * vec3(1,1,1), 1.0);
    out_colour = texture(diffuse, out_texture_coordinates);
}
