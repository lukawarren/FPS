struct VertexInput
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position         : SV_POSITION;
    float2 uv               : TEXCOORD;
    float3 normal           : NORMAL;
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
    float4 spritesheet_info;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    float4 world_pos = mul(model, float4(input.position, 0.0f, 1.0f));
    float4 view_pos  = mul(view, world_pos);
    output.position = mul(projection, view_pos);
    output.uv = input.uv * spritesheet_info.xy + spritesheet_info.zw;
    output.normal = mul((float3x3)model, float3(0.0f, 0.0f, -1.0f));
    output.world_pos = world_pos;
    return output;
}