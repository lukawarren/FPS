Texture2D input_texture : register(t0, space2);
SamplerState input_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET
{
    return input_texture.Sample(input_sampler, input.uv);
}