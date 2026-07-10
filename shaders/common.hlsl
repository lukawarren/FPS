struct Spotlight
{
    float4x4 shadow;
    float3 position;
    float3 colour;
    float3 direction;
    float4 params;
};

#define GAMMA 2.2f
#define AMBIENT 0.0f
#define MAX_SPOTLIGHTS 12
#define POINT_INTENSITY 0.1f
#define BIAS (0.5f / 10000.0f)

cbuffer UniformBlock : register(b0, space3)
{
    Spotlight spotlights[MAX_SPOTLIGHTS];
};

float sample_shadow(
    Texture2DArray shadow_map_texture,
    SamplerState shadow_map_sampler,
    float4 light_space_pos,
    int i
)
{
    float3 proj_coords = light_space_pos.xyz / light_space_pos.w;
    proj_coords.xy = proj_coords.xy * 0.5 + 0.5;
    proj_coords.y = 1.0f - proj_coords.y;

    float current_depth = proj_coords.z;
    if (current_depth > 1.0f)
        return 0.0f;

    float shadow = 0.0f;

    // If sample is outside the texture, treat as fully lit
    bool in_bounds = (proj_coords.x >= 0.0 && proj_coords.x <= 1.0 &&
                        proj_coords.y >= 0.0 && proj_coords.y <= 1.0);

    if (!in_bounds)
        return 0.0f;

    float closest = shadow_map_texture.Sample(
        shadow_map_sampler,
        float3(proj_coords.xy, i)
    ).r;

    return (current_depth - BIAS > closest) ? 0.0f : 1.0f;
}

float3 calculate_pointlight(Spotlight s, float3 fragment_to_light, float distance)
{
    float atten = max(pow(distance, 2.0f), 1.0f);
    return s.colour * POINT_INTENSITY / atten;
}

float3 calculate_spotlight(Spotlight s, float3 normal, float3 fragment_to_light, float distance)
{
    // Unpack
    float inner_cutoff = s.params.x;
    float outer_cutoff = s.params.y;
    float range = s.params.z;

    float dist = max(distance, 0.001f);
    float3 light_dir = fragment_to_light / dist;

    float diff_factor = max(dot(normal, light_dir), 0.0f);

    float attenuation = 1.0f / (dist * dist);

    float range_atten = saturate(1.0f - pow(dist / range, 4.0f));
    range_atten = range_atten * range_atten; // Square it for smoother falloff
    attenuation *= range_atten;

    float theta = dot(-light_dir, s.direction);
    float epsilon = inner_cutoff - outer_cutoff;
    float cone_intensity = clamp((theta - outer_cutoff) / epsilon, 0.0f, 1.0f);

    return s.colour * diff_factor * attenuation * cone_intensity;
}

float3 lighting_from_diffuse(
    float3 diffuse,
    float3 normal,
    float4 world_pos,
    Spotlight spotlights[MAX_SPOTLIGHTS],
    Texture2DArray shadow_map_texture,
    SamplerState shadow_map_sampler
)
{
    float3 total = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_SPOTLIGHTS; i++)
    {
        // Params of zero means disabled (i.e. less than MAX_SPOTLIGHTS)
        if (spotlights[i].params.w == 0.0f)
            break;

        float3 fragment_to_light = spotlights[i].position - world_pos.xyz;
        float distance = length(fragment_to_light);

        // Lighting
        float3 spotlight_lighting = calculate_spotlight(
            spotlights[i],
            normal,
            fragment_to_light,
            distance
        );
        float3 point_lighting = calculate_pointlight(
            spotlights[i],
            fragment_to_light,
            distance
        );

        // Shadow
        float shadow = sample_shadow(
            shadow_map_texture,
            shadow_map_sampler,
            mul(spotlights[i].shadow, world_pos),
            i
        );

        total += max(spotlight_lighting * shadow + point_lighting, 0.0f);
    }

    float3 ambient = diffuse * AMBIENT;
    float3 direct  = diffuse * total;
    return ambient + direct;
}