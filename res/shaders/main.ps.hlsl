Texture2D texture : register(t0);
SamplerState texture_sampler : register(s0);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET
{
    return texture.Sample(texture_sampler, input.uv);
}
