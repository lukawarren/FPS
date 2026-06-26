#include "render/renderer.h"

Renderer::Renderer(const std::string& title, const u32 width, const u32 height) :
    device(title, width, height),
    texture_manager(device),
    pipeline_factory(device, texture_manager)
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    world = new World("map2.map", device.window, device.device, copy_pass);
    quad = new Quad(device.device, copy_pass);
    SDL_EndGPUCopyPass(copy_pass);
    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.texture->generate_mipmaps(command_buffer);
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

#if 0
    if (SDL_WindowSupportsGPUPresentMode(device.device, device.window->get_window(), SDL_GPU_PRESENTMODE_IMMEDIATE))
        SDL_SetGPUSwapchainParameters(
            device.device,
            device.window->get_window(),
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            SDL_GPU_PRESENTMODE_IMMEDIATE
        );
#endif

    dbg("TODO: don't use uniform buffer for lights");
    dbg("TODO: fix AO");
}

Renderer::~Renderer()
{
    device.wait_for_idle();
    delete world;
    delete quad;
}

bool Renderer::update()
{
    device.window->update();
    world->player.update(world->map->world, world->camera, 1.0f / 60.0f);
    world->player.update_camera(world->camera);
    return !device.window->should_close();
}

void Renderer::render()
{
    SDL_GPUCommandBuffer* command_buffer = device.acquire_command_buffer();
    std::optional<SDL_GPUTexture*> swapchain_texture = device.get_swapchain_texture(command_buffer);
    if (!swapchain_texture.has_value())
    {
        SDL_CancelGPUCommandBuffer(command_buffer);
        return;
    }

    if (device.did_swapchain_format_change())
    {
        texture_manager.on_swapchain_format_change(device);
        pipeline_factory.on_swapchain_format_change(device.swapchain_format);
    }

    const glm::mat4 camera_projection = world->camera.projection_matrix(device.swapchain_width, device.swapchain_height);
    const glm::mat4 camera_view = world->camera.view_matrix();

    const u32 n_lights = world->player.flashlight.enabled
            ? std::min(QUALITY_SETTINGS.max_spotlights, (u32)world->spotlights.size() + 1)
            : std::min(QUALITY_SETTINGS.max_spotlights, (u32)world->spotlights.size());

    std::array<Spotlight*, QUALITY_SETTINGS.max_spotlights> spotlights;
    std::array<glm::mat4, QUALITY_SETTINGS.max_spotlights> matrices;
    for (u32 i = 0; i < n_lights - (world->player.flashlight.enabled ? 1 : 0); i++)
    {
        spotlights[i] = &world->spotlights[i];
        matrices[i] = spotlights[i]->get_matrix();
    }

    if (world->player.flashlight.enabled)
    {
        spotlights[n_lights - 1] = &world->player.flashlight;
        matrices[n_lights - 1] = world->player.flashlight.get_matrix();
    }

    // Set diffuse uniforms
    diffuse_shader_uniforms_vertex.view = camera_view;
    diffuse_shader_uniforms_vertex.projection = camera_projection;

    for (u32 i = 0; i < n_lights; i++)
        diffuse_shader_uniforms_fragment.spotlights[i] = spotlights[i]->get_uniform_buffer(matrices[i]);
    for (u32 i = n_lights; i < QUALITY_SETTINGS.max_spotlights; i++)
        diffuse_shader_uniforms_fragment.spotlights[i] = Spotlight::get_disabled_uniform_buffer();

    // Set SSAO uniforms
    ssao_shader_uniforms_vertex.aspect_ratio = (float)device.swapchain_width / (float)device.swapchain_height;
    ssao_shader_uniforms_vertex.tan_half_fov = std::tan(world->camera.fov / 2.0f);
    ssao_shader_uniforms_fragment.projection = camera_projection;

    for (u32 i = 0; i < n_lights; i++)
        shadow_pass(command_buffer, matrices[i], i);

    depth_pass(command_buffer, camera_view, camera_projection);
    ssao_pass(command_buffer);
    ssao_blur_pass(command_buffer);
    diffuse_pass(command_buffer);
    downsample_pass(command_buffer);
    upsample_pass(command_buffer);
    composite_pass(command_buffer, swapchain_texture.value());

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void Renderer::shadow_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& light_matrix, const u8 slot)
{
    SDL_GPURenderPass* shadow_pass = SDL_BeginGPURenderPass(
        command_buffer,
        NULL,
        0,
        &(SDL_GPUDepthStencilTargetInfo) {
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

    SDL_BindGPUGraphicsPipeline(shadow_pass, pipeline_factory.depth_pipeline_texture_array);

    SDL_SetGPUViewport(shadow_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)QUALITY_SETTINGS.shadow_map_width,
        .h = (float)QUALITY_SETTINGS.shadow_map_height,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    glm::mat4 m[2] = { light_matrix, glm::mat4(1.0f) };
    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        (void*)glm::value_ptr(m[0]),
        sizeof(float) * 32
    );

    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.mesh->bind(shadow_pass);
        draw_call.mesh->draw(shadow_pass);
    }

    SDL_EndGPURenderPass(shadow_pass);
}

void Renderer::depth_pass(SDL_GPUCommandBuffer* command_buffer, const glm::mat4& view, const glm::mat4& projection)
{
    SDL_GPURenderPass* depth_pass = SDL_BeginGPURenderPass(
        command_buffer,
        NULL,
        0,
        &(SDL_GPUDepthStencilTargetInfo) {
            .texture = texture_manager.depth_texture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = true,
            .clear_stencil = 0,
            .mip_level = 0,
            .layer = 0
        }
    );

    SDL_BindGPUGraphicsPipeline(depth_pass, pipeline_factory.depth_pipeline);

    SDL_SetGPUViewport(depth_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale,
        .h = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    glm::mat4 m[2] = { view, projection };
    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        (void*)glm::value_ptr(m[0]),
        sizeof(float) * 32
    );

    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.mesh->bind(depth_pass);
        draw_call.mesh->draw(depth_pass);
    }

    SDL_EndGPURenderPass(depth_pass);
}

void Renderer::diffuse_pass(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPURenderPass* diffuse_pass = SDL_BeginGPURenderPass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
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
        },
        1,
        &(SDL_GPUDepthStencilTargetInfo) {
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

    SDL_BindGPUGraphicsPipeline(diffuse_pass, pipeline_factory.diffuse_pipeline);

    SDL_SetGPUViewport(diffuse_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale,
        .h = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &diffuse_shader_uniforms_vertex,
        sizeof(diffuse_shader_uniforms_vertex)
    );

    SDL_PushGPUFragmentUniformData(
        command_buffer,
        0,
        &diffuse_shader_uniforms_fragment,
        sizeof(diffuse_shader_uniforms_fragment)
    );

    SDL_BindGPUFragmentSamplers(
        diffuse_pass,
        1,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = texture_manager.shadow_map_sampler,
            .texture = texture_manager.shadow_map
        },
        1
    );

    std::array<SDL_GPUTextureSamplerBinding, 2> bindings =
    {
        SDL_GPUTextureSamplerBinding
        {
            .sampler = texture_manager.shadow_map_sampler,
            .texture = texture_manager.shadow_map
        },
        SDL_GPUTextureSamplerBinding
        {
            .sampler = texture_manager.bloom_sampler,
            .texture = texture_manager.ssao_blur_texture
        }
    };

    SDL_BindGPUFragmentSamplers(
        diffuse_pass,
        1,
        &bindings[0],
        (u32)bindings.size()
    );

    for (const auto& draw_call : world->map->draw_calls)
    {
        draw_call.texture->bind(diffuse_pass, texture_manager.sampler);
        draw_call.mesh->bind(diffuse_pass);
        draw_call.mesh->draw(diffuse_pass);
    }

    SDL_EndGPURenderPass(diffuse_pass);
}

void Renderer::ssao_pass(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPURenderPass* ssao_pass = SDL_BeginGPURenderPass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = texture_manager.ssao_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = true,
            .cycle_resolve_texture = false
        },
        1,
        NULL
    );

    SDL_BindGPUGraphicsPipeline(ssao_pass, pipeline_factory.ssao_pipeline);

    SDL_SetGPUViewport(ssao_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale / 2.0f,
        .h = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale / 2.0f,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    SDL_PushGPUVertexUniformData(
        command_buffer,
        0,
        &ssao_shader_uniforms_vertex,
        sizeof(ssao_shader_uniforms_vertex)
    );

    SDL_PushGPUFragmentUniformData(
        command_buffer,
        0,
        &ssao_shader_uniforms_fragment,
        sizeof(ssao_shader_uniforms_fragment)
    );

    SDL_BindGPUFragmentSamplers(
        ssao_pass,
        0,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = texture_manager.bloom_sampler,
            .texture = texture_manager.depth_texture
        },
        1
    );

    quad->bind(ssao_pass);
    quad->draw(ssao_pass);
    SDL_EndGPURenderPass(ssao_pass);
}

void Renderer::ssao_blur_pass(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPURenderPass* ssao_blur_pass = SDL_BeginGPURenderPass(
        command_buffer,
        &(SDL_GPUColorTargetInfo) {
            .texture = texture_manager.ssao_blur_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .store_op = SDL_GPU_STOREOP_STORE,
            .resolve_texture = NULL,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = true,
            .cycle_resolve_texture = false
        },
        1,
        NULL
    );

    SDL_BindGPUGraphicsPipeline(ssao_blur_pass, pipeline_factory.ssao_blur_pipeline);

    SDL_SetGPUViewport(ssao_blur_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)device.swapchain_width / QUALITY_SETTINGS.inverse_render_scale / 2.0f,
        .h = (float)device.swapchain_height / QUALITY_SETTINGS.inverse_render_scale / 2.0f,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    SDL_BindGPUFragmentSamplers(
        ssao_blur_pass,
        0,
        &(SDL_GPUTextureSamplerBinding) {
            .sampler = texture_manager.bloom_sampler,
            .texture = texture_manager.ssao_texture
        },
        1
    );

    quad->bind(ssao_blur_pass);
    quad->draw(ssao_blur_pass);
    SDL_EndGPURenderPass(ssao_blur_pass);
}

void Renderer::downsample_pass(SDL_GPUCommandBuffer* command_buffer)
{
    for (u32 level = 0; level < QUALITY_SETTINGS.bloom_downsamples; level++)
    {
        SDL_GPURenderPass* downsample_pass = SDL_BeginGPURenderPass(
            command_buffer,
            &(SDL_GPUColorTargetInfo) {
                .texture = texture_manager.bloom_textures[level],
                .mip_level = 0,
                .layer_or_depth_plane = 0,
                .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                .load_op = SDL_GPU_LOADOP_DONT_CARE,
                .store_op = SDL_GPU_STOREOP_STORE,
                .resolve_texture = NULL,
                .resolve_mip_level = 0,
                .resolve_layer = 0,
                .cycle = true,
                .cycle_resolve_texture = false
            },
            1,
            NULL
        );

        SDL_BindGPUGraphicsPipeline(downsample_pass, pipeline_factory.downsample_pipeline);

        SDL_SetGPUViewport(downsample_pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = (float)texture_manager.get_bloom_texture_width(device, level),
            .h = (float)texture_manager.get_bloom_texture_height(device, level),
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });

        SDL_BindGPUFragmentSamplers(
            downsample_pass,
            0,
            &(SDL_GPUTextureSamplerBinding) {
                .sampler = texture_manager.bloom_sampler,
                .texture = (level == 0 ? texture_manager.diffuse_texture : texture_manager.bloom_textures[level - 1])
            },
            1
        );

        quad->bind(downsample_pass);
        quad->draw(downsample_pass);
        SDL_EndGPURenderPass(downsample_pass);
    }
}

void Renderer::upsample_pass(SDL_GPUCommandBuffer* command_buffer)
{
    for (u32 level = 0; level < QUALITY_SETTINGS.bloom_downsamples - 1; level++)
    {
        const u32 target_level = texture_manager.bloom_textures.size() - level - 2;
        const u32 source_level = texture_manager.bloom_textures.size() - level - 1;

        SDL_GPURenderPass* upsample_pass = SDL_BeginGPURenderPass(
            command_buffer,
            &(SDL_GPUColorTargetInfo) {
                .texture = texture_manager.bloom_textures[target_level],
                .mip_level = 0,
                .layer_or_depth_plane = 0,
                .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                .load_op = SDL_GPU_LOADOP_LOAD,
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

        SDL_BindGPUGraphicsPipeline(upsample_pass, pipeline_factory.upsample_pipeline);

        SDL_SetGPUViewport(upsample_pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = (float)texture_manager.get_bloom_texture_width(device, target_level),
            .h = (float)texture_manager.get_bloom_texture_height(device, target_level),
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });

        SDL_BindGPUFragmentSamplers(
            upsample_pass,
            0,
            &(SDL_GPUTextureSamplerBinding) {
                .sampler = texture_manager.bloom_sampler,
                .texture = texture_manager.bloom_textures[source_level]
            },
            1
        );

        quad->bind(upsample_pass);
        quad->draw(upsample_pass);
        SDL_EndGPURenderPass(upsample_pass);
    }
}

void Renderer::composite_pass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* swapchain_texture)
{
    SDL_GPURenderPass* composite_pass = SDL_BeginGPURenderPass(
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

    SDL_BindGPUGraphicsPipeline(composite_pass, pipeline_factory.composite_pipeline);

    SDL_SetGPUViewport(composite_pass, &(SDL_GPUViewport) {
        .x = 0.0f,
        .y = 0.0f,
        .w = (float)device.swapchain_width,
        .h = (float)device.swapchain_height,
        .min_depth = 0.0f,
        .max_depth = 1.0f
    });

    std::array<SDL_GPUTextureSamplerBinding, 2> bindings =
    {
        SDL_GPUTextureSamplerBinding
        {
            .sampler = texture_manager.sampler,
            .texture = texture_manager.diffuse_texture
        },
        SDL_GPUTextureSamplerBinding
        {
            .sampler = texture_manager.bloom_sampler,
            .texture = texture_manager.bloom_textures[0]
        }
    };

    SDL_BindGPUFragmentSamplers(
        composite_pass,
        0,
        &bindings[0],
        (u32)bindings.size()
    );

    quad->bind(composite_pass);
    quad->draw(composite_pass);
    SDL_EndGPURenderPass(composite_pass);
}