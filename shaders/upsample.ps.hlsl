/*
    https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
*/

Texture2D input_texture : register(t0, space2);
SamplerState input_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

#define FILTER_RADIUS 0.005f

float4 main(VertexOutput input) : SV_TARGET
{
    float x = FILTER_RADIUS;
    float y = FILTER_RADIUS;

    float3 a = input_texture.Sample(input_sampler, float2(input.uv.x - x, input.uv.y + y)).rgb;
    float3 b = input_texture.Sample(input_sampler, float2(input.uv.x,     input.uv.y + y)).rgb;
    float3 c = input_texture.Sample(input_sampler, float2(input.uv.x + x, input.uv.y + y)).rgb;

    float3 d = input_texture.Sample(input_sampler, float2(input.uv.x - x, input.uv.y)).rgb;
    float3 e = input_texture.Sample(input_sampler, float2(input.uv.x,     input.uv.y)).rgb;
    float3 f = input_texture.Sample(input_sampler, float2(input.uv.x + x, input.uv.y)).rgb;

    float3 g = input_texture.Sample(input_sampler, float2(input.uv.x - x, input.uv.y - y)).rgb;
    float3 h = input_texture.Sample(input_sampler, float2(input.uv.x,     input.uv.y - y)).rgb;
    float3 i = input_texture.Sample(input_sampler, float2(input.uv.x + x, input.uv.y - y)).rgb;

    float3 upsample = e * 4.0f;
    upsample += (b + d + f + h) * 2.0f;
    upsample += (a + c + g + i);
    upsample *= 1.0f / 16.0f;

    return float4(upsample, 1.0f);
}