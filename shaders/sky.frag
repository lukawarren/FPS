#version 330 core

uniform vec3 zenith_colour;
uniform vec3 horizon_colour;
uniform vec2 screen_size;
uniform mat4 inverse_view;

out vec4 frag_colour;

void main()
{
    // Work out mix amount
    vec4 view_dir = inverse_view * vec4(gl_FragCoord.xy / screen_size * 2.0 - 1.0, 1.0, 1.0);
    view_dir = normalize(view_dir);
    float t = pow(abs(view_dir.y), 1.2);

    // Work out colour
    vec3 colour = mix(horizon_colour, zenith_colour, t);
    frag_colour = vec4(colour, 1.0);
}
