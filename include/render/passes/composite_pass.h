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

    struct PostProcessingSettings
    {
        float bloom_strength = 0.02f;
        float exposure = 250.0f;
        float gamma = 2.2f;
        float panini_strength = 0.0f;

        glm::vec4 get_uniform_buffer() const
        {
            return { bloom_strength, exposure, gamma, panini_strength };
        }
    } settings = {};

private:
    Device& device;

    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
    Quad& quad;
};
