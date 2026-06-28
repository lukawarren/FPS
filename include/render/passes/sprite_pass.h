#pragma once
#include "common.h"
#include "world.h"
#include "model.h"
#include "render/device.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"
#include "render/passes/diffuse_pass.h"
#include "render/quad.h"

class SpritePass
{
public:
    SpritePass(
        Device& device,
        PipelineFactory& pipeline_factory,
        TextureManager& texture_manager
    );

    void execute(
        SDL_GPUCommandBuffer* command_buffer,
        const World& world,
        const std::unordered_map<Decal::ID, Texture*>& sprites,
        const glm::mat4& view,
        const glm::mat4& projection,
        const Quad& quad,
        const DiffusePass::FragmentUniforms& fragment_uniforms
    );

private:
    Device& device;
    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
};
