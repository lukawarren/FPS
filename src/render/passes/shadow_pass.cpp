#include "render/passes/shadow_pass.h"
#include "render/render_pass.h"

ShadowPass::ShadowPass(
    PipelineFactory& pipeline_factory,
    TextureManager& texture_manager)
:
    pipeline_factory(pipeline_factory),
    texture_manager(texture_manager)
{}

void ShadowPass::execute(
    SDL_GPUCommandBuffer* command_buffer,
    const World& world,
    const std::unordered_map<Model::ID, std::pair<Mesh*, Texture*>>& models,
    const glm::mat4& light_matrix,
    const glm::mat4& weapon_model,
    const Quad& quad,
    u8 slot
)
{
    const RenderPass<0> render_pass(
        command_buffer,
        {
            .texture = texture_manager.shadow_map,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = (slot == 0),
            .clear_stencil = 0,
            .mip_level = 0,
            .layer = slot
        }
    );

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.depth_pipeline_texture_array);
    render_pass.set_viewport((float)QUALITY_SETTINGS.shadow_map_width, (float)QUALITY_SETTINGS.shadow_map_height);

    // Draw map
    render_pass.push_view_projection(light_matrix, glm::mat4(1.0f));
    render_pass.push_model_matrix(glm::mat4(1.0f));
    for (const auto& draw_call : world.map->draw_calls)
    {
        draw_call.texture->bind(render_pass, texture_manager.sampler);
        draw_call.mesh->bind(render_pass);
        draw_call.mesh->draw(render_pass);
    }

    // Draw entities - TODO: sort
    models.at(Model::ID::DOOR).second->bind(render_pass, texture_manager.sampler);
    for (const auto& e : world.entities)
    {
        glm::mat4 model = e->transform.matrix();

        if (e->model.has_value())
        {
            model *= e->model->transform.matrix();
            render_pass.push_model_matrix(model);
            models.at(e->model->id).first->bind(render_pass);
            models.at(e->model->id).first->draw(render_pass);
        }
    }

    (void)models;
}
