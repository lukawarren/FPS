#pragma once
#include "common.h"
#include "render/device.h"
#include "render/texture_manager.h"

class PipelineFactory
{
public:
    PipelineFactory(Device& device, const TextureManager& texture_manager);
    ~PipelineFactory();

    void on_swapchain_format_change(SDL_GPUTextureFormat swapchain_format);

    SDL_GPUGraphicsPipeline* diffuse_pipeline;
    SDL_GPUGraphicsPipeline* depth_pipeline;
    SDL_GPUGraphicsPipeline* downsample_pipeline;
    SDL_GPUGraphicsPipeline* upsample_pipeline;
    SDL_GPUGraphicsPipeline* composite_pipeline;

private:
    SDL_GPUGraphicsPipeline* create_diffuse_pipeline(
        SDL_GPUShader* vs,
        SDL_GPUShader* fs,
        SDL_GPUTextureFormat colour_format,
        SDL_GPUTextureFormat depth_format
    );

    SDL_GPUGraphicsPipeline* create_depth_pipeline(
        SDL_GPUShader* vs,
        SDL_GPUShader* fs,
        SDL_GPUTextureFormat depth_format
    );

    SDL_GPUGraphicsPipeline* create_downsample_pipeline(
        SDL_GPUShader* vs,
        SDL_GPUShader* fs,
        SDL_GPUTextureFormat colour_format
    );

    SDL_GPUGraphicsPipeline* create_upsample_pipeline(
        SDL_GPUShader* vs,
        SDL_GPUShader* fs,
        SDL_GPUTextureFormat colour_format
    );

    SDL_GPUGraphicsPipeline* create_composite_pipeline(
        SDL_GPUShader* vs,
        SDL_GPUShader* fs,
        SDL_GPUTextureFormat colour_format
    );

    SDL_GPUGraphicsPipeline* check_pipeline(SDL_GPUGraphicsPipeline* pipeline) const;

    SDL_GPUShader* diffuse_vs;
    SDL_GPUShader* diffuse_fs;
    SDL_GPUShader* depth_vs;
    SDL_GPUShader* depth_fs;
    SDL_GPUShader* quad_vs;
    SDL_GPUShader* downsample_fs;
    SDL_GPUShader* upsample_fs;
    SDL_GPUShader* composite_fs;

    SDL_GPUTextureFormat depth_texture_array_format;

    SDL_GPUDevice* device;
};
