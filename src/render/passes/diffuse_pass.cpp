#include "render/passes/diffuse_pass.h"
#include "render/render_pass.h"

DiffusePass::DiffusePass(
    Device& device,
    PipelineFactory& pipeline_factory,
    TextureManager& texture_manager
) :
    device(device),
    pipeline_factory(pipeline_factory),
    texture_manager(texture_manager)
{}

void DiffusePass::execute(
    SDL_GPUCommandBuffer* command_buffer,
    const World& world,
    const std::unordered_map<Model::ID, Model*>& models,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::mat4& weapon_model,
    const FragmentUniforms& fragment_uniforms
)
{
    const RenderPass<1> render_pass(
        command_buffer,
        {{{
            .texture = texture_manager.diffuse_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = true,
            .cycle_resolve_texture = false
        }}},
        {
            .texture = texture_manager.depth_texture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = false,
            .clear_stencil = 0,
            .mip_level = 0,
            .layer = 0
        }
    );

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.diffuse_pipeline);

    const float width = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale;
    const float height = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale;
    render_pass.set_viewport(width, height);

    render_pass.push_view_projection(view, projection);

    SDL_PushGPUFragmentUniformData(
        command_buffer,
        0,
        &fragment_uniforms,
        sizeof(fragment_uniforms)
    );

    render_pass.bind_fragment_sampler(
        1,
        texture_manager.shadow_map,
        texture_manager.shadow_map_sampler
    );

    // Draw map
    render_pass.push_model_matrix(glm::mat4(1.0f));
    for (const auto& draw_call : world.map->draw_calls)
    {
        draw_call.texture->bind(render_pass, texture_manager.sampler);
        draw_call.mesh->bind(render_pass);
        draw_call.mesh->draw(render_pass);
    }

    // Draw torches
    models.at(Model::ID::TORCH)->mesh->bind(render_pass);
    models.at(Model::ID::TORCH)->texture->bind(render_pass, texture_manager.sampler);
    for (const auto& light : world.torchlights)
    {
        render_pass.push_model_matrix(light.get_model_matrix());
        models.at(Model::ID::TORCH)->mesh->draw(render_pass);
    }

    // Draw weapon
    render_pass.push_model_matrix(weapon_model);
    models.at(world.player->weapon.model)->texture->bind(render_pass, texture_manager.sampler);
    models.at(world.player->weapon.model)->mesh->bind(render_pass);
    models.at(world.player->weapon.model)->mesh->draw(render_pass);
}
