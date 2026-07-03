struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position         : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float4 world_pos        : POSITIONT;
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
    VertexOutput output;

    float4 world_pos = mul(model, float4(input.position, 1.0f));
    float4 view_pos  = mul(view, world_pos);
    output.position = mul(projection, view_pos);

    output.normal = mul((float3x3)model, input.normal);
    output.uv = input.uv;
    output.world_pos = world_pos;
    return output;
}