struct VertexInput
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

cbuffer PushBlock : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
};

cbuffer PushBlockTwo : register(b1, space1)
{
    float4x4 model;
    float4 spritesheet_info;
};

VertexOutput main(VertexInput input)
{
    // Need all because otherwise get tiny floating-point differences
    // when we use the output as a depth-prepass
    VertexOutput output;
    output.position = mul(projection, mul(view, mul(model, float4(input.position, 0.0f, 1.0f))));
    output.uv = input.uv * spritesheet_info.xy + spritesheet_info.zw;
    return output;
}