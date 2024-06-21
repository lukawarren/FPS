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

out vec4 out_colour;

float get_shadow(int i)
{
    // Sample cube map
    vec3 frag_to_light = out_position.xyz - point_lights[i].position;
    float current_depth = length(frag_to_light);
    current_depth /= point_lights[i].far_plane;

    // // Perform PCF depth test
    // float shadow  = 0.0;
    // float bias    = 0.005;
    // float samples = 4.0;
    // float offset  = 0.01;
    // for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    // {
    //     for(float y = -offset; y < offset; y += offset / (samples * 0.5))
    //     {
    //         for(float z = -offset; z < offset; z += offset / (samples * 0.5))
    //         {
    //             float closest_depth = texture(
    //                 point_lights[i].depth,
    //                 vec4(frag_to_light + vec3(x, y, z), current_depth - bias)
    //             );
    //             shadow += closest_depth;
    //         }
    //     }
    // }
    // shadow /= (samples * samples * samples);
    // return shadow;

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
    float shadow = get_shadow(i);
    return vec3(1,1,1) * shadow;
    return point_lights[i].colour * diffuse * (1.0 - shadow);
}

void main()
{
    vec4 diffuse_colour = texture(diffuse, out_texture_coordinates);
    vec3 normal = normalize(out_normal);

    vec3 total_lighting = vec3(0, 0, 0);
    for (int i = 0; i < n_point_lights; ++i)
        total_lighting += lighting(i, normal);

    const vec3 ambient = vec3(0.75, 1.0, 1.0) * 0.3;
    total_lighting += ambient;

    diffuse_colour.rgb *= total_lighting;
    out_colour = diffuse_colour;
}
