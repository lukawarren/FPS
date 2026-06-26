struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

cbuffer PushBlock : register(b0, space1)
{
    // Need both because otherwise get tiny floating-point differences
    // when we use the output as a depth-prepass (diffuse uses two too)
    float4x4 view;
    float4x4 projection;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(projection, mul(view, float4(input.position, 1.0)));
    return output;
}