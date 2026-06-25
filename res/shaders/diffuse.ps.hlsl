Texture2D diffuse_texture : register(t0, space2);
SamplerState diffuse_sampler : register(s0, space2);

Texture2DArray shadow_map_texture : register(t1, space2);
SamplerState shadow_map_sampler : register(s1, space2);

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

cbuffer UniformBlock : register(b0, space1)
{
    Spotlight spotlight;
};

#define GAMMA 2.2f
#define AMBIENT 0.01f

float sample_shadow(float4 light_space_pos, float bias)
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
        float3(proj_coords.xy, 0)
    ).r;

    return (current_depth - bias > closest) ? 0.0f : 1.0f;
}

float3 calculate_spotlight(Spotlight s, float3 world_pos, float3 normal)
{
    // Unpack
    float inner_cutoff = s.params.x;
    float outer_cutoff = s.params.y;
    float range = s.params.z;

    float3 fragment_to_light = s.position - world_pos;
    float distance = length(fragment_to_light);

    // --- 1. Prevent division by zero (clamp to a small physical epsilon) ---
    float dist = max(distance, 0.001f);
    float3 light_dir = fragment_to_light / dist;

    // --- 2. Diffuse factor ---
    float diff_factor = max(dot(normal, light_dir), 0.0f);

    // --- 3. Physically correct inverse-square attenuation ---
    // This gives infinite intensity at distance 0, which is correct for a point source in HDR.
    float attenuation = 1.0f / (dist * dist);

    // --- 4. Smooth range window (no hard cut-off) ---
    // Prevents the light from abruptly disappearing at 'range'.
    float range_atten = saturate(1.0f - pow(dist / range, 4.0f));
    range_atten = range_atten * range_atten; // Square it for smoother falloff
    attenuation *= range_atten;

    // --- 5. Spotlight cone (unchanged, still physically correct) ---
    float theta = dot(-light_dir, s.direction);
    float epsilon = inner_cutoff - outer_cutoff;
    float cone_intensity = clamp((theta - outer_cutoff) / epsilon, 0.0f, 1.0f);

    // --- 6. Final output ---
    // 's.colour' should now represent the luminous intensity (e.g., 10, 100, 5000).
    // For a standard bright bulb, try s.colour = float3(10, 10, 10).
    return s.colour * diff_factor * attenuation * cone_intensity;
}

float4 main(VertexOutput input) : SV_TARGET
{
    // Shadow
    float bias = 0.5f / 10000.0f;
    float depth = abs(input.view_pos.z);
    float shadow = sample_shadow(
        mul(spotlight.shadow, input.world_pos),
        bias
    );

    // Diffuse
    float3 diffuse = diffuse_texture.Sample(diffuse_sampler, input.uv).xyz;
    diffuse = pow(diffuse, GAMMA);

    // Lighting
    float3 normal = normalize(input.normal);
    float3 lighting = calculate_spotlight(
        spotlight,
        input.world_pos.xyz,
        normal
    );

    // Composite
    float3 final_color = diffuse * max(lighting * shadow, AMBIENT);
    return float4(final_color, 1.0f);
}