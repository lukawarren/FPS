#include "common.hlsl"

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
};

float4 main(VertexOutput input) : SV_TARGET
{
    // Diffuse
    float3 diffuse = diffuse_texture.Sample(diffuse_sampler, input.uv).xyz;
    diffuse = pow(diffuse, GAMMA);

    return float4(
        lighting_from_diffuse(
            diffuse,
            normalize(input.normal),
            input.world_pos,
            spotlights,
            shadow_map_texture,
            shadow_map_sampler
        ),
        1.0f
    );
}