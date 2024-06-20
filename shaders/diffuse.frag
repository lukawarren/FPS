#version 330 core

in vec4 out_position;
in vec2 out_texture_coordinates;
in vec3 out_normal;

uniform sampler2D diffuse;
uniform samplerCube point_depth;
uniform vec3 light_position;
uniform vec3 light_colour;
uniform float light_far_plane;

out vec4 out_colour;

float get_shadow()
{
    // Sample cube map
    vec3 frag_to_light = out_position.xyz - light_position;
    float closest_depth = texture(point_depth, frag_to_light).r;

    // Transform to appropriate range
    closest_depth *= light_far_plane;

    // Perform depth test
    float current_depth = length(frag_to_light);
    float bias = 0.05;
    float shadow = current_depth -  bias > closest_depth ? 1.0 : 0.0;

    return shadow;
}

void main()
{
    vec4 diffuse_colour = texture(diffuse, out_texture_coordinates);

    vec3 normal = normalize(out_normal);
    vec3 light_direction = normalize(light_position - out_position.xyz);
    float light = max(dot(normal, light_direction), 0.0);

    float shadow = get_shadow();
    light = clamp(light * (1.0 - shadow), 0.2, 1.0);

    diffuse_colour.rgb *= light * light_colour;

    out_colour = diffuse_colour;
}
