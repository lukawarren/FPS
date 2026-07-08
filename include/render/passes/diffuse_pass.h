#pragma once
#include "common.h"
#include "world.h"
#include "model.h"
#include "render/device.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"

class DiffusePass
{
public:
    struct FragmentUniforms
    {
        Spotlight::UniformBuffer spotlights[QUALITY_SETTINGS.max_spotlights];
    };

    DiffusePass(
        Device& device,
        PipelineFactory& pipeline_factory,
        TextureManager& texture_manager
    );

    void execute(
        SDL_GPUCommandBuffer* command_buffer,
        const World& world,
        const std::unordered_map<Model::ID, std::pair<Mesh*, Texture*>>& models,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& weapon_model,
        const FragmentUniforms& fragment_uniforms
    );

private:
    Device& device;
    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
};
