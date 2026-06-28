Texture2D sprite_texture : register(t0, space2);
SamplerState sprite_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET
{
    float4 colour = sprite_texture.Sample(sprite_sampler, input.uv);
    return colour;
}