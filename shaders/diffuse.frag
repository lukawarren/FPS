#version 330 core

in vec4 out_position;
in vec2 out_texture_coordinates;
in vec3 out_normal;

uniform sampler2D diffuse;
uniform vec3 light_position;
uniform vec3 light_colour;

out vec4 out_colour;

void main()
{
    vec4 diffuse_colour = texture(diffuse, out_texture_coordinates);

    vec3 normal = normalize(out_normal);
    vec3 light_direction = normalize(light_position - out_position.xyz);
    float light = max(dot(normal, light_direction), 0.1);
    diffuse_colour.rgb *= light * light_colour;

    out_colour = diffuse_colour;
}
