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
#define RADIUS      0.5f
#define BIAS        0.025f

static float3 kernel[KERNEL_SIZE] = {
    float3(-0.622651, -0.516783, 0.374449), float3(0.301514, -0.199939, 0.165905), float3(-0.38758, 0.398072, 0.0731924), float3(0.0964864, 0.363239, 0.347272), float3(0.505438, 0.664398, 0.487284), float3(-0.265563, -0.0967887, 0.588656), float3(0.558119, 0.641515, 0.114473), float3(0.258135, -0.130686, 0.297014), float3(-0.548736, 0.0426653, 0.566417), float3(0.0499953, 0.0117025, 0.0782948), float3(0.283543, -0.175434, 0.776356), float3(-0.00530914, 0.190153, 0.0249714), float3(-0.126593, 0.128413, 0.0192292), float3(0.0926566, 0.0628378, 0.293504), float3(-0.129973, -0.187055, 0.965809), float3(0.0525745, -0.229296, 0.238291), float3(-0.0179082, -0.171632, 0.0260816), float3(-0.0513358, -0.40797, 0.222265), float3(0.0267781, 0.0590061, 0.0158284), float3(0.151492, -0.0501862, 0.0434536), float3(0.592261, 0.633925, 0.0733402), float3(0.192698, 0.272144, 0.483945), float3(-0.354293, -0.248632, 0.405415), float3(0.229132, -0.13187, 0.0571746), float3(-0.124102, -0.483885, 0.388049), float3(-0.692169, -0.643828, 0.0820197), float3(0.0558091, -0.628182, 0.377663), float3(-0.0905577, -0.202167, 0.0212559), float3(-0.577092, 0.140403, 0.112945), float3(-0.543233, 0.326079, 0.200942), float3(-0.582333, -0.414356, 0.232253), float3(-0.181223, 0.552261, 0.478873), float3(0.208751, -0.196128, 0.448963), float3(0.413797, -0.5201, 0.431887), float3(0.494428, 0.450822, 0.428185), float3(0.0732595, -0.498744, 0.0777295), float3(0.326741, -0.255607, 0.376051), float3(0.187846, 0.00225796, 0.246615), float3(0.500341, 0.822733, 0.0542489), float3(-0.202125, -0.0374265, 0.223305), float3(0.539415, -0.573457, 0.152488), float3(-0.00563813, 0.0428553, 0.027185), float3(0.0700563, -0.0533606, 0.0566281), float3(0.509906, 0.039509, 0.101491), float3(0.244467, -0.123996, 0.160484), float3(0.287112, 0.66191, 0.14069), float3(-0.0143867, 0.0472964, 0.00178313), float3(0.0369718, 0.00706403, 0.0478053), float3(-0.788807, 0.569846, 0.10893), float3(-0.148261, -0.210936, 0.314104), float3(0.439096, 0.144408, 0.596403), float3(-0.018441, 0.0245758, 0.0196887), float3(0.0310836, 0.603909, 0.67149), float3(0.316043, -0.421911, 0.208209), float3(-0.258912, -0.136635, 0.264538), float3(-0.143384, -0.0204573, 0.112294), float3(0.0621013, -0.324273, 0.513577), float3(0.0918034, -0.0873289, 0.0625252), float3(-0.258121, 0.26518, 0.71133), float3(-0.224162, -0.390155, 0.140621), float3(-0.581331, -0.712594, 0.377169), float3(-0.317983, 0.314075, 0.719894), float3(-0.00937166, -0.190157, 0.0231137), float3(0.165882, -0.395007, 0.0987293)
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

        float sample_depth = get_view_z(offset.xy);
        float depth_diff = abs(pos.z - sample_depth);
        float range_check = smoothstep(0.0, 1.0, RADIUS / depth_diff);
        float occluded = step(sample_pos.z + BIAS, sample_depth);
        total += occluded * range_check;
    }

    float occlusion = 1.0f - (total / (float)KERNEL_SIZE);
    return saturate(pow(occlusion, 5.0f));
}