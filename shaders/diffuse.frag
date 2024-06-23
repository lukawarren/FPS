#version 330 core
#define MAX_POINT_LIGHTS 4

in vec4 out_position;
in vec2 out_texture_coordinates;
in vec3 out_normal;

struct PointLight
{
    vec3 position;
    vec3 colour;
    float far_plane;
    samplerCubeShadow depth;
};
uniform sampler2D diffuse;
uniform PointLight point_lights[MAX_POINT_LIGHTS];
uniform int n_point_lights;
uniform vec3 ambient;
uniform float min_shadow;

out vec4 out_colour;

float get_shadow(int i)
{
    // Sample cube map
    vec3 frag_to_light = out_position.xyz - point_lights[i].position;
    float current_depth = length(frag_to_light);
    current_depth /= point_lights[i].far_plane;

    float bias = 0.005;
    float shadow = texture(point_lights[i].depth, vec4(frag_to_light, current_depth - bias));
    return shadow;
}

float attenuate(vec3 light_direction)
{
    const float constant = 1.0;
    const float linear = 0.14;
    const float quadratic = 0.07;
    float distance = length(light_direction);
    return 1.0 / (constant + linear * distance + quadratic * (distance * distance));
}

vec3 lighting(int i, vec3 normal)
{
    vec3 light_direction = point_lights[i].position - out_position.xyz;
    float attenuation = attenuate(light_direction);
    float diffuse = dot(normal, normalize(light_direction)) * attenuation;
    diffuse = max(diffuse, 0);
    float shadow = max(get_shadow(i), min_shadow);
    return point_lights[i].colour * diffuse * shadow;
}

void main()
{
    vec4 diffuse_colour = texture(diffuse, out_texture_coordinates);
    vec3 normal = normalize(out_normal);

    vec3 total_lighting = ambient;
    for (int i = 0; i < n_point_lights; ++i)
        total_lighting += lighting(i, normal);

    diffuse_colour.rgb *= total_lighting;
    out_colour = diffuse_colour;
}
