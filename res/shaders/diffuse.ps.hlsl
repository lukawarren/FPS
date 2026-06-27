Texture2D diffuse_texture : register(t0, space2);
SamplerState diffuse_sampler : register(s0, space2);

Texture2DArray shadow_map_texture : register(t1, space2);
SamplerState shadow_map_sampler : register(s1, space2);

Texture2D ssao_texture : register(t2, space2);
SamplerState ssao_sampler : register(s2, space2);

struct VertexOutput
{
    float4 position         : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float4 world_pos        : POSITIONT;
    float4 view_pos         : POSITIONT2;
};

struct Spotlight
{
    float4x4 shadow;
    float3 position;
    float3 colour;
    float3 direction;
    float4 params;
};

#define GAMMA 2.2f
#define AMBIENT 0.3f
#define MAX_SPOTLIGHTS 6
#define POINT_INTENSITY 1.0f

cbuffer UniformBlock : register(b0, space1)
{
    Spotlight spotlights[MAX_SPOTLIGHTS];
};

float sample_shadow(float4 light_space_pos, float bias, int i)
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

    return (current_depth - bias > closest) ? 0.0f : 1.0f;
}

float3 calculate_pointlight(Spotlight s, float3 fragment_to_light, float distance)
{
    // Limit distance so brightness doesn't blow up
    distance = max(distance, 1.0f);

    // Scale up fall-off too
    return s.colour * POINT_INTENSITY / pow(distance, 4.0f);
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

float4 main(VertexOutput input) : SV_TARGET
{
    // Diffuse
    float3 diffuse = diffuse_texture.Sample(diffuse_sampler, input.uv).xyz;
    diffuse = pow(diffuse, GAMMA);

    float bias = 0.5f / 10000.0f;
    float depth = abs(input.view_pos.z);
    float3 normal = normalize(input.normal);
    float3 total = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_SPOTLIGHTS; i++)
    {
        // Params of zero means disabled (i.e. less than MAX_SPOTLIGHTS)
        if (spotlights[i].params.w == 0.0f)
            break;

        float3 fragment_to_light = spotlights[i].position - input.world_pos.xyz;
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
            mul(spotlights[i].shadow, input.world_pos),
            bias,
            i
        );

        total += max(spotlight_lighting * shadow + point_lighting, 0.0f);
    }

    // SSAO
    float ao_width;
    float ao_height;
    ssao_texture.GetDimensions(ao_width, ao_height);
    float2 screen_resolution = float2(ao_width, ao_height) * 2.0f;
    float ao = ssao_texture.Sample(ssao_sampler, input.position.xy / screen_resolution).x;

    float3 ambient = diffuse * AMBIENT * ao;
    float3 direct  = diffuse * total;
    float3 final_colour = ambient + direct;
    return float4(final_colour, 1.0f);
}