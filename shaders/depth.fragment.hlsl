Texture2D diffuse_texture : register(t0, space2);
SamplerState diffuse_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

void main(VertexOutput input)
{
    if (diffuse_texture.Sample(diffuse_sampler, input.uv).a <= 0.0f)
        discard;
}
