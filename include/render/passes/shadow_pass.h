#pragma once
#include "common.h"
#include "world.h"
#include "model.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"
#include "render/quad.h"

class ShadowPass
{
public:
    ShadowPass(PipelineFactory& pipeline_factory, TextureManager& texture_manager);

    void execute(
        SDL_GPUCommandBuffer* command_buffer,
        const World& world,
        const std::unordered_map<Model::ID, Model*>& models,
        const std::unordered_map<Decal::ID, Texture*>& sprites,
        const glm::mat4& light_matrix,
        const glm::mat4& weapon_model,
        const Quad& quad,
        u8 slot
    );

private:
    PipelineFactory& pipeline_factory;
    TextureManager& texture_manager;
};
