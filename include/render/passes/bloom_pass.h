#pragma once
#include "common.h"
#include "render/device.h"
#include "render/quad.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"

class BloomPass
{
public:
    BloomPass(
        Device& device,
        PipelineFactory& pipeline_factory,
        TextureManager& texture_manager,
        Quad& quad
    );

    void execute(SDL_GPUCommandBuffer* command_buffer);

private:
    void downsample(SDL_GPUCommandBuffer* command_buffer);
    void upsample(SDL_GPUCommandBuffer* command_buffer);

    Device& device;
    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
    Quad& quad;
};
