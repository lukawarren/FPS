Texture2D render_texture : register(t0, space2);
SamplerState render_sampler : register(s0, space2);

Texture2D bloom_texture : register(t1, space2);
SamplerState bloom_sampler : register(s1, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

#define A 0.15f
#define B 0.50f
#define C 0.10f
#define D 0.20f
#define E 0.02f
#define F 0.30f
#define W 11.2f

cbuffer UniformBlock : register(b0, space3)
{
    float4 settings;
};

#define BLOOM_STRENGTH  settings.x
#define EXPOSURE        settings.y
#define GAMMA           settings.z
#define PANINI_STRENGTH settings.w

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

float2 panini(float2 view_pos, float d)
{
    float view_dist = 1.0 + d;
    float view_hyp_sq = view_pos.x * view_pos.x + view_dist * view_dist;

    float isect_D = view_pos.x * d;
    float isect_discrim = view_hyp_sq - isect_D * isect_D;

    float cyl_dist_minus_d = (-isect_D * view_pos.x + view_dist * sqrt(isect_discrim)) / view_hyp_sq;
    float cyl_dist = cyl_dist_minus_d + d;

    float2 cyl_pos = view_pos * (cyl_dist / view_dist);
    return cyl_pos / (cyl_dist - d);
}

float4 main(VertexOutput input) : SV_TARGET
{
    // Panini projection
    float2 screen_pos = input.uv * 2.0f - 1.0f;
    const float d = PANINI_STRENGTH;
    float2 edge_pos = panini(float2(1.0f, 0.0f), d);
    float scale = 1.0f / edge_pos.x;
    float2 warped_pos = panini(screen_pos, d) * scale;
    float2 panini_uv = warped_pos * 0.5f + 0.5f;
    panini_uv = clamp(panini_uv, 0.0f, 1.0f);

    // 6. Sample your textures using the new Panini UVs
    float3 render = render_texture.Sample(render_sampler, panini_uv).rgb;
    float3 bloom = bloom_texture.Sample(bloom_sampler, panini_uv).rgb;

    return float4(
        tonemap(lerp(render, bloom, BLOOM_STRENGTH)),
        1.0f
    );
}