#pragma once
#include "common.h"

class Texture
{
public:
    Texture(
        const std::string& filename,
        SDL_GPUDevice* device,
        SDL_GPUCopyPass* copy_pass,
        const std::string& root = TEXTURE_ROOT
    );
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(SDL_GPURenderPass* render_pass, SDL_GPUSampler* sampler);
    void generate_mipmaps(SDL_GPUCommandBuffer* command_buffer);

private:
    SDL_GPUDevice* device;
    SDL_GPUTexture* texture;
};