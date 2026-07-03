#include "render/texture_manager.h"

TextureManager::TextureManager(const Device& device) : device(device)
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

    const auto create_sampler = [&](const SDL_GPUSamplerCreateInfo& info)
    {
        return SDL_CreateGPUSampler(device.device, &info);
    };

    sampler = create_sampler({
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

    shadow_map_sampler = create_sampler({
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

    bloom_sampler = create_sampler({
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

    create_diffuse_texture();
    create_depth_texture();
    create_shadow_map();
    create_bloom_textures();
}

TextureManager::~TextureManager()
{
    SDL_ReleaseGPUSampler(device.device, sampler);
    SDL_ReleaseGPUTexture(device.device, diffuse_texture);

    SDL_ReleaseGPUTexture(device.device, shadow_map);
    SDL_ReleaseGPUTexture(device.device, depth_texture);
    SDL_ReleaseGPUSampler(device.device, shadow_map_sampler);

    for (size_t i = 0; i < bloom_textures.size(); i++)
        SDL_ReleaseGPUTexture(device.device, bloom_textures[i]);

    SDL_ReleaseGPUSampler(device.device, bloom_sampler);
}

u32 TextureManager::get_bloom_texture_width(const u32 level)
{
    u32 x = device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale;
    for (u32 i = 0; i <= level; i++)
        x = std::max(x / 2U, 16U);
    return x;
}

u32 TextureManager::get_bloom_texture_height(const u32 level)
{
    u32 x = device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale;
    for (u32 i = 0; i <= level; i++)
        x = std::max(x / 2U, 16U);
    return x;
}

SDL_GPUTexture* TextureManager::create_texture(const SDL_GPUTextureCreateInfo& info)
{
    return SDL_CreateGPUTexture(device.device, &info);
}

void TextureManager::on_swapchain_format_change()
{
    SDL_ReleaseGPUTexture(device.device, depth_texture);
    SDL_ReleaseGPUTexture(device.device, diffuse_texture);

    for (size_t i = 0; i < bloom_textures.size(); i++)
            SDL_ReleaseGPUTexture(device.device, bloom_textures[i]);

    create_depth_texture();
    create_diffuse_texture();
    create_bloom_textures();
}

void TextureManager::create_diffuse_texture()
{
    diffuse_texture = create_texture({
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale,
        .height = device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

void TextureManager::create_depth_texture()
{
    depth_texture = create_texture({
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale,
        .height = device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
        .props = 0
    });
}

void TextureManager::create_shadow_map()
{
    shadow_map = create_texture({
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

void TextureManager::create_bloom_textures()
{
    for (u32 i = 0; i < (u32)bloom_textures.size(); i++)
    {
        bloom_textures[i] = create_texture({
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = get_bloom_texture_width(i),
            .height = get_bloom_texture_height(i),
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .props = 0
        });
    }
}
