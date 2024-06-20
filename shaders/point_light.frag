#version 330 core

in vec4 out_position;
uniform vec3 light_position;
uniform float far_plane;

void main()
{
    // Get distance to light in [0, 1] range
    float light_distance = length(out_position.xyz - light_position);
    light_distance = light_distance / far_plane;

    // Write to depth (manually as we modified it)
    gl_FragDepth = light_distance;
}