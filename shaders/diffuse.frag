#version 330 core

in vec2 out_texture_coords;
uniform sampler2D diffuse;
out vec4 out_colour;

void main()
{
    out_colour = texture(diffuse, out_texture_coords);
}
