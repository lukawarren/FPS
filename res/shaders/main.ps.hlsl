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
    const float gamma = 2.2;
    float3 diffuse = texture.Sample(texture_sampler, input.uv).xyz;
    float3 light_dir = normalize(float3(5.0f, 3.0f, 5.0f));
    float3 norm = normalize(input.normal);
    float amount = max(dot(light_dir, norm), 0.3f);
    float3 light = amount * float3(1.0f, 1.0f, 1.0f) * 2.0f;
    return float4(pow(diffuse * light, 1.0f / gamma), 1.0f);
}
