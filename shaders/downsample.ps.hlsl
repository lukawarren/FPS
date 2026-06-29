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

float4 main(VertexOutput input) : SV_TARGET
{
    float width;
    float height;
    input_texture.GetDimensions(width, height);

    float2 source_texel_size = 1.0f / float2(width, height);
    float x = source_texel_size.x;
    float y = source_texel_size.y;

    float3 a = input_texture.Sample(input_sampler, float2(input.uv.x - 2*x, input.uv.y + 2*y)).rgb;
    float3 b = input_texture.Sample(input_sampler, float2(input.uv.x,       input.uv.y + 2*y)).rgb;
    float3 c = input_texture.Sample(input_sampler, float2(input.uv.x + 2*x, input.uv.y + 2*y)).rgb;

    float3 d = input_texture.Sample(input_sampler, float2(input.uv.x - 2*x, input.uv.y)).rgb;
    float3 e = input_texture.Sample(input_sampler, float2(input.uv.x,       input.uv.y)).rgb;
    float3 f = input_texture.Sample(input_sampler, float2(input.uv.x + 2*x, input.uv.y)).rgb;

    float3 g = input_texture.Sample(input_sampler, float2(input.uv.x - 2*x, input.uv.y - 2*y)).rgb;
    float3 h = input_texture.Sample(input_sampler, float2(input.uv.x,       input.uv.y - 2*y)).rgb;
    float3 i = input_texture.Sample(input_sampler, float2(input.uv.x + 2*x, input.uv.y - 2*y)).rgb;

    float3 j = input_texture.Sample(input_sampler, float2(input.uv.x - x, input.uv.y + y)).rgb;
    float3 k = input_texture.Sample(input_sampler, float2(input.uv.x + x, input.uv.y + y)).rgb;
    float3 l = input_texture.Sample(input_sampler, float2(input.uv.x - x, input.uv.y - y)).rgb;
    float3 m = input_texture.Sample(input_sampler, float2(input.uv.x + x, input.uv.y - y)).rgb;

    float3 downsample = e * 0.125f;
    downsample += (a + c + g + i) * 0.03125f;
    downsample += (b + d + f + h) * 0.0625f;
    downsample += (j + k + l + m) * 0.125f;
    downsample = max(downsample, 0.0000001f);

    return float4(downsample, 1.0);
}