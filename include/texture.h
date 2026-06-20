#pragma once
#include "common.h"

class Texture
{
public:
    Texture(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(SDL_GPURenderPass* render_pass, SDL_GPUSampler* sampler);

private:
    SDL_GPUDevice* device;
    SDL_GPUTexture* texture;
};