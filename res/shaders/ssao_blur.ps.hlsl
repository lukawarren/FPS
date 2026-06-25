Texture2D input_texture : register(t0, space2);
SamplerState input_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET
{
    float texel_width;
    float texel_height;
    input_texture.GetDimensions(texel_width, texel_height);
    float2 texel_size = 1.0f / float2(texel_width, texel_height);

	float result = 0.0f;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			float2 offset = float2((float)x, (float)y) * texel_size;
            result += input_texture.Sample(input_sampler, input.uv + offset).r;
		}
	}

    return result / 16.0f;
}