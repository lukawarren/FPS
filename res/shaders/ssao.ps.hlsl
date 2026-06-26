/*
    https://ogldev.org/www/tutorial46/tutorial46.html
*/

Texture2D depth_texture : register(t0, space2);
SamplerState depth_sampler : register(s0, space2);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
	float2 view_ray : POSITIONT;
};

cbuffer UniformBlock : register(b0, space1)
{
    float4x4 projection;
};

#define KERNEL_SIZE 64
#define RADIUS      0.3f
#define BIAS        0.025f
#define STRENGTH    2.0f

static float3 kernel[KERNEL_SIZE] = {
    float3(-0.067789, -0.056263, 0.0137418), float3(0.0332316, -0.0220365, 0.0010635), float3(-0.0251516, 0.0258325, -0.0435419), float3(0.012877, 0.0484775, 0.0143943), float3(0.0569343, 0.07484, 0.0341956), float3(-0.0307775, -0.0112173, 0.0605997), float3(0.0532909, 0.0612538, -0.0444987), float3(0.0296096, -0.0149905, 0.0317492), float3(-0.0788856, 0.00613351, -0.0430664), float3(0.00969777, 0.00226998, 0.00473796), float3(0.0693313, -0.0428967, 0.0630397), float3(-0.000364268, 0.0130466, -0.0204818), float3(-0.0115193, 0.011685, -0.0173392), float3(0.0180985, 0.012274, 0.0371148), float3(-0.0205396, -0.0295602, 0.137326), float3(0.00969152, -0.0422682, 0.0249679), float3(-0.00213375, -0.0204499, -0.0179127), float3(-0.00954083, -0.0758218, 0.000682236), float3(0.00200819, 0.00442507, -0.0103334), float3(0.0211149, -0.00699494, -0.0196193), float3(0.0967999, 0.10361, -0.0815772), float3(0.0588628, 0.0831308, 0.0549119), float3(-0.0820252, -0.0575629, 0.07024), float3(0.042308, -0.024349, -0.0322174), float3(-0.0335519, -0.130822, 0.0479437), float3(-0.128569, -0.11959, -0.140996), float3(0.0160725, -0.180911, 0.0197654), float3(-0.00181075, -0.00404243, -0.0577308), float3(-0.0987923, 0.0240355, -0.129448), float3(-0.135127, 0.0811105, -0.104856), float3(-0.17735, -0.126193, -0.0518749), float3(-0.0718347, 0.21891, 0.0427693), float3(0.079075, -0.0742936, 0.134848), float3(0.164505, -0.206766, 0.0500637), float3(0.188552, 0.171922, 0.118204), float3(0.0224182, -0.152621, -0.107976), float3(0.162266, -0.12694, 0.0630018), float3(0.0801225, 0.000963091, 0.0949688), float3(0.165482, 0.27211, -0.246049), float3(-0.1202, -0.0222568, 0.049238), float3(0.227656, -0.242024, -0.143964), float3(-0.00304448, 0.023141, 0.00544342), float3(0.0403147, -0.030707, 0.00616735), float3(0.214279, 0.0166029, -0.15327), float3(0.14698, -0.0745493, -0.0262622), float3(0.138494, 0.319285, -0.19835), float3(-0.00580167, 0.019073, -0.0195851), float3(0.0218921, 0.00418283, 0.027782), float3(-0.391309, 0.282688, -0.34549), float3(-0.106609, -0.151676, 0.175116), float3(0.288157, 0.0947677, 0.384749), float3(-0.014007, 0.0186668, 0.00747273), float3(0.0262856, 0.51069, 0.363255), float3(0.222646, -0.297228, -0.165316), float3(-0.235798, -0.124437, 0.119727), float3(-0.126909, -0.0181069, -0.0566167), float3(0.0613816, -0.320515, 0.354396), float3(0.0824159, -0.078399, -0.0168964), float3(-0.235013, 0.24144, 0.58243), float3(-0.171478, -0.298458, -0.218531), float3(-0.44448, -0.544843, -0.538478), float3(-0.313524, 0.30967, 0.640595), float3(-0.00657296, -0.13337, -0.122436), float3(0.149394, -0.355745, -0.183805)
};

float get_view_z(float2 coords)
{
    float depth = depth_texture.Sample(depth_sampler, coords).r;
    return -projection[2][3] / (depth + projection[2][2]);
}

float4 main(VertexOutput input) : SV_TARGET
{
    float view_z = get_view_z(input.uv);
    float view_x = input.view_ray.x * view_z;
    float view_y = input.view_ray.y * view_z;
    float3 pos = float3(view_x, view_y, view_z);

    float total = 0.0f;

    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        float3 sample_pos = pos + kernel[i] * RADIUS;

        float4 offset = float4(sample_pos, 1.0f);
        offset = mul(projection, offset);
        offset.xy /= offset.w;
        offset.x = offset.x * 0.5f + 0.5f;
        offset.y = -offset.y * 0.5f + 0.5f;

        if (offset.x < 0.0f || offset.x > 1.0f || offset.y < 0.0f || offset.y > 1.0f)
        {
            // Out of bounds
            continue;
        }

        float sample_depth = get_view_z(offset.xy);
        float occluded = (sample_pos.z <= sample_depth - BIAS) ? 1.0f : 0.0f;
        float depth_diff = abs(pos.z - sample_depth);
        float range_check = smoothstep(0.0f, 1.0f, RADIUS / depth_diff);
        total += occluded * range_check;
    }

    float occlusion = 1.0f - (total * STRENGTH / (float)KERNEL_SIZE);
    return saturate(pow(occlusion, 5.0f));
}