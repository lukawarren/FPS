#version 330 core
#define MAX_POINT_LIGHTS 4

in vec4 out_position;
in vec2 out_texture_coordinates;
in vec3 out_normal;
in vec4 out_position_directional_light_space;

struct PointLight
{
    vec3 position;
    vec3 colour;
    float far_plane;
    samplerCubeShadow depth;
};

struct DirectionalLight
{
    vec3 position;
    vec3 colour;
    mat4 matrix;
    sampler2DShadow depth;
};

uniform sampler2D diffuse;
uniform PointLight point_lights[MAX_POINT_LIGHTS];
uniform DirectionalLight directional_light;
uniform int n_point_lights;
uniform vec3 ambient;
uniform float min_shadow;

out vec4 out_colour;

float get_point_shadow(int i)
{
    // Sample cube map
    vec3 frag_to_light = out_position.xyz - point_lights[i].position;
    float current_depth = length(frag_to_light);
    current_depth /= point_lights[i].far_plane;

    float bias = 0.005;
    float shadow = texture(point_lights[i].depth, vec4(frag_to_light, current_depth - bias));
    return shadow;
}

float get_directional_shadow()
{
    vec3 projected_coords = out_position_directional_light_space.xyz / out_position_directional_light_space.w;
    projected_coords = projected_coords * 0.5 + 0.5;

    float bias = 0.0007;
    projected_coords.z -= bias;

    float shadow = texture(directional_light.depth, projected_coords);
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

vec3 point_lighting(int i, vec3 normal)
{
    vec3 light_direction = point_lights[i].position - out_position.xyz;
    float attenuation = attenuate(light_direction);
    float diffuse = dot(normal, normalize(light_direction)) * attenuation;
    diffuse = max(diffuse, 0);
    float shadow = max(get_point_shadow(i), min_shadow);
    return point_lights[i].colour * diffuse * shadow;
}

vec3 directional_lighting(vec3 normal)
{
    vec3 light_direction = directional_light.position;
    float diffuse = dot(normal, normalize(light_direction));
    diffuse = max(diffuse, 0);
    float shadow = max(get_directional_shadow(), min_shadow);
    return vec3(1, 1, 1) * shadow;
    return directional_light.colour * diffuse * shadow;
}

void main()
{
    vec4 diffuse_colour = texture(diffuse, out_texture_coordinates);
    vec3 normal = normalize(out_normal);

    vec3 total_lighting = ambient;
    for (int i = 0; i < n_point_lights; ++i)
        total_lighting += point_lighting(i, normal);
    total_lighting += directional_lighting(normal);

    diffuse_colour.rgb *= total_lighting;
    out_colour = diffuse_colour;
}
