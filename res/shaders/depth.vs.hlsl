struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

cbuffer PushBlock : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
};

cbuffer PushBlockTwo : register(b1, space1)
{
    float4x4 model;
};

VertexOutput main(VertexInput input)
{
    // Need all because otherwise get tiny floating-point differences
    // when we use the output as a depth-prepass
    VertexOutput output;
    output.position = mul(projection, mul(view, mul(model, float4(input.position, 1.0))));
    return output;
}