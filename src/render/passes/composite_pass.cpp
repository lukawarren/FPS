#include "render/passes/composite_pass.h"
#include "render/render_pass.h"

CompositePass::CompositePass(
    Device& device,
    PipelineFactory& pipeline_factory,
    TextureManager& texture_manager,
    Quad& quad
) :
    device(device),
    pipeline_factory(pipeline_factory),
    texture_manager(texture_manager),
    quad(quad)
{}

void CompositePass::execute(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture)
{
    const RenderPass render_pass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = swapchain_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = false,
            .cycle_resolve_texture = false
        },
        1,
        NULL
    );

    SDL_BindGPUGraphicsPipeline(render_pass, pipeline_factory.composite_pipeline);
    render_pass.set_viewport((float)device.swapchain_width, (float)device.swapchain_height);

    const std::array<SDL_GPUTextureSamplerBinding, 2> bindings =
    {
        SDL_GPUTextureSamplerBinding
        {
            .texture = texture_manager.diffuse_texture,
            .sampler = texture_manager.sampler
        },
        SDL_GPUTextureSamplerBinding
        {
            .texture = texture_manager.bloom_textures[0],
            .sampler = texture_manager.bloom_sampler
        }
    };

    SDL_BindGPUFragmentSamplers(render_pass, 0, &bindings[0], (u32)bindings.size());

    quad.bind(render_pass);
    quad.draw(render_pass);
}
