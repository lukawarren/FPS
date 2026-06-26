#include "render/texture_manager.h"

TextureManager::TextureManager(const Device& device) : device(device.device)
{
    // Not guaranteed that we support 32-bit float depth
    const bool supports_32_array = SDL_GPUTextureSupportsFormat(
        device.device,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTURETYPE_2D_ARRAY,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER
    );
    const bool supports_32 = SDL_GPUTextureSupportsFormat(
        device.device,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTURETYPE_2D,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER
    );

    depth_texture_array_format = supports_32_array ? SDL_GPU_TEXTUREFORMAT_D32_FLOAT : SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depth_texture_format = supports_32 ? SDL_GPU_TEXTUREFORMAT_D32_FLOAT : SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    sampler = SDL_CreateGPUSampler(
        device.device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_NEAREST,
            .mag_filter = SDL_GPU_FILTER_NEAREST,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 16.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = true,
            .enable_compare = false,
            .props = 0
        }
    );

    shadow_map_sampler = SDL_CreateGPUSampler(
        device.device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 0.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = false,
            .enable_compare = false,
            .props = 0
        }
    );

    bloom_sampler = SDL_CreateGPUSampler(
        device.device,
        &(SDL_GPUSamplerCreateInfo) {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0.0f,
            .max_anisotropy = 0.0f,
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .min_lod = 0.0f,
            .max_lod = FLT_MAX,
            .enable_anisotropy = false,
            .enable_compare = false,
            .props = 0
        }
    );

    create_diffuse_texture(device);
    create_depth_texture(device);
    create_ssao_textures(device);
    create_shadow_map(device);
    create_bloom_textures(device);
}

TextureManager::~TextureManager()
{
    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseGPUTexture(device, diffuse_texture);

    SDL_ReleaseGPUTexture(device, shadow_map);
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUSampler(device, shadow_map_sampler);

    SDL_ReleaseGPUTexture(device, ssao_texture);
    SDL_ReleaseGPUTexture(device, ssao_blur_texture);

    for (size_t i = 0; i < bloom_textures.size(); i++)
        SDL_ReleaseGPUTexture(device, bloom_textures[i]);

    SDL_ReleaseGPUSampler(device, bloom_sampler);
}

u32 TextureManager::get_bloom_texture_width(const Device& device, const u32 level)
{
    u32 x = device.swapchain_width;
    for (u32 i = 0; i <= level; i++)
        x = std::max(x / 2U, 16U);
    return x;
}

u32 TextureManager::get_bloom_texture_height(const Device& device, const u32 level)
{
    u32 x = device.swapchain_height;
    for (u32 i = 0; i <= level; i++)
        x = std::max(x / 2U, 16U);
    return x;
}

void TextureManager::on_swapchain_format_change(const Device& device)
{
    SDL_ReleaseGPUTexture(device.device, depth_texture);
    SDL_ReleaseGPUTexture(device.device, diffuse_texture);

    SDL_ReleaseGPUTexture(device.device, ssao_texture);
    SDL_ReleaseGPUTexture(device.device, ssao_blur_texture);

    for (size_t i = 0; i < bloom_textures.size(); i++)
            SDL_ReleaseGPUTexture(device.device, bloom_textures[i]);

    create_depth_texture(device);
    create_diffuse_texture(device);
    create_ssao_textures(device);
    create_bloom_textures(device);
}

void TextureManager::create_diffuse_texture(const Device& device)
{
    diffuse_texture = SDL_CreateGPUTexture(device.device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = device.swapchain_width,
        .height = device.swapchain_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

void TextureManager::create_depth_texture(const Device& device)
{
    depth_texture = SDL_CreateGPUTexture(device.device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = device.swapchain_width,
        .height = device.swapchain_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

void TextureManager::create_shadow_map(const Device& device)
{
    shadow_map = SDL_CreateGPUTexture(device.device, &(SDL_GPUTextureCreateInfo){
        .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
        .format = depth_texture_array_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = QUALITY_SETTINGS.shadow_map_width,
        .height = QUALITY_SETTINGS.shadow_map_height,
        .layer_count_or_depth = QUALITY_SETTINGS.max_spotlights,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

void TextureManager::create_ssao_textures(const Device& device)
{
    ssao_texture = SDL_CreateGPUTexture(
        device.device,
        &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = device.swapchain_width / 2,
            .height = device.swapchain_height / 2,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .props = 0
        }
    );

    ssao_blur_texture = SDL_CreateGPUTexture(
        device.device,
        &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = device.swapchain_width / 2,
            .height = device.swapchain_height / 2,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .props = 0
        }
    );
}


void TextureManager::create_bloom_textures(const Device& device)
{
    for (u32 i = 0; i < (u32)bloom_textures.size(); i++)
    {
        bloom_textures[i] = SDL_CreateGPUTexture(
            device.device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
                .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
                .width = get_bloom_texture_width(device, i),
                .height = get_bloom_texture_height(device, i),
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .props = 0
            }
        );
    }
}
