#pragma once
#include "common.h"
#include "render/device.h"

class TextureManager
{
public:
    TextureManager(const Device& device);
    ~TextureManager();

    void on_swapchain_format_change();

    // Diffuse pass
    SDL_GPUTexture* diffuse_texture;
    SDL_GPUTexture* depth_texture;
    SDL_GPUTextureFormat depth_texture_format;
    SDL_GPUSampler* sampler;

    // Shadow pass
    SDL_GPUTexture* shadow_map;
    SDL_GPUSampler* shadow_map_sampler;
    SDL_GPUTextureFormat depth_texture_array_format;

    // Bloom passes
    SDL_GPUSampler* bloom_sampler;
    std::array<SDL_GPUTexture*, QUALITY_SETTINGS.bloom_downsamples> bloom_textures;
    u32 get_bloom_texture_width(const u32 level);
    u32 get_bloom_texture_height(const u32 level);

private:
    SDL_GPUTexture* create_texture(const SDL_GPUTextureCreateInfo& info);

    void create_diffuse_texture();
    void create_depth_texture();
    void create_shadow_map();
    void create_bloom_textures();

    const Device& device;
};