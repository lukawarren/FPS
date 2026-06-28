#pragma once
#include "common.h"
#include "render/device.h"
#include "render/quad.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"

class CompositePass
{
public:
    CompositePass(
        Device& device,
        PipelineFactory& pipeline_factory,
        TextureManager& texture_manager,
        Quad& quad
    );

    void execute(
        SDL_GPUCommandBuffer* command_buffer,
        SDL_GPUTexture* swapchain_texture
    );

private:
    Device& device;
    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
    Quad& quad;
};
