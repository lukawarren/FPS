#include "common.hlsl"

Texture2D sprite_texture : register(t0, space2);
SamplerState sprite_sampler : register(s0, space2);

Texture2DArray shadow_map_texture : register(t1, space2);
SamplerState shadow_map_sampler : register(s1, space2);

struct VertexOutput
{
    float4 position         : SV_POSITION;
    float2 uv               : TEXCOORD;
    float3 normal           : NORMAL;
    float4 world_pos        : POSITIONT;
};

float4 main(VertexOutput input) : SV_TARGET
{
    float4 colour = sprite_texture.Sample(sprite_sampler, input.uv);

    if (colour.a <= 0.0f)
        discard;

    colour.xyz = pow(colour.xyz, GAMMA);

    return float4(
        lighting_from_diffuse(
            colour.xyz,
            normalize(input.normal),
            input.world_pos,
            spotlights,
            shadow_map_texture,
            shadow_map_sampler
        ),
        colour.a
    );
}