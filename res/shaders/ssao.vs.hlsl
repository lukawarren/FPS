/*
    https://ogldev.org/www/tutorial46/tutorial46.html
*/

struct VertexInput
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
	float2 view_ray : POSITIONT;
};

cbuffer UniformBlock : register(b0, space1)
{
	float aspect_ratio;
	float tan_half_fov;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.uv = input.uv;

	output.view_ray.x = -input.position.x * aspect_ratio * tan_half_fov;
	output.view_ray.y = -input.position.y * tan_half_fov;

    return output;
}