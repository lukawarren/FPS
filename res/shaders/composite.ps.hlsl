Texture2D render_texture : register(t0, space2);
SamplerState render_sampler : register(s0, space2);

Texture2D bloom_texture : register(t1, space2);
SamplerState bloom_sampler : register(s1, space2);

Texture2D ssao_texture : register(t2, space2);
SamplerState ssao_sampler : register(s2, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

#define BLOOM_STRENGTH  0.03f
#define EXPOSURE        16.0f
#define GAMMA           2.2f

#define A 0.15f
#define B 0.50f
#define C 0.10f
#define D 0.20f
#define E 0.02f
#define F 0.30f
#define W 11.2f

float3 uncharted_2_tonemap(float3 x)
{
    return ((x * ( A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 tonemap(float3 x)
{
    float3 current = uncharted_2_tonemap(x * EXPOSURE);
    float3 white_scale = 1.0f / uncharted_2_tonemap(W);
    float3 colour = current * white_scale;
    return pow(colour, 1.0f / GAMMA);
}

float4 main(VertexOutput input) : SV_TARGET
{
    float3 render = render_texture.Sample(render_sampler, input.uv).rgb;
    float3 bloom = bloom_texture.Sample(bloom_sampler, input.uv).rgb;
    float ssao = ssao_texture.Sample(ssao_sampler, input.uv).r;

     return float4(
        tonemap(lerp(render, bloom, BLOOM_STRENGTH)),
        1.0f
    ) * 0.00001f + float4(1,1,1,1) * ssao;
}