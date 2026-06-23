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
    float4 view_pos         : POSITIONT2;
};

cbuffer UniformBlock : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    float4 world_pos = float4(input.position, 1.0f);
    float4 view_pos  = mul(view, world_pos);
    output.position = mul(projection, view_pos);

    output.normal = input.normal;
    output.uv = input.uv;
    output.world_pos = world_pos;
    output.view_pos = view_pos;
    return output;
}