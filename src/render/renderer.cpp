#include "render/renderer.h"

Renderer::Renderer(
    const std::string& title,
    const u32 width,
    const u32 height,
    Audio& audio
) :
    device(title, width, height),
    texture_manager(device),
    pipeline_factory(device, texture_manager),
    quad(create_quad(device)),
    shadow_pass(pipeline_factory, texture_manager),
    depth_pass(device, pipeline_factory, texture_manager),
    diffuse_pass(device, pipeline_factory, texture_manager),
    sprite_pass(device, pipeline_factory, texture_manager),
    bloom_pass(device, pipeline_factory, texture_manager, *quad),
    composite_pass(device, pipeline_factory, texture_manager, *quad)
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    // Load quad
    Quad* quad = new Quad(device.device, copy_pass);

    // Load models
    for (size_t i = 0; i < Model::MODEL_NAMES.size(); i++)
    {
        models[(Model::ID)i] = new Model(
            Model::MODEL_NAMES[i],
            device.device,
            copy_pass
        );
    }

    // Load sprites
    for (size_t i = 0; i < Decal::SPRITE_NAMES.size(); i++)
    {
        sprites[(Decal::ID)i] = new Texture(
            std::string(Decal::SPRITE_NAMES[i].first) + ".png",
            device.device,
            copy_pass,
            SPRITE_ROOT
        );
    }

    // Load world
    world = new World("map3.map", device.window, device.device, copy_pass, audio);

    SDL_EndGPUCopyPass(copy_pass);

    for (const auto& draw_call : world->map->draw_calls)
        draw_call.texture->generate_mipmaps(command_buffer);

    for (const auto& m : models)
        m.second->texture->generate_mipmaps(command_buffer);

    for (const auto& s : sprites)
        s.second->generate_mipmaps(command_buffer);

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
}

Renderer::~Renderer()
{
    device.wait_for_idle();

    for (size_t i = 0; i < Model::MODEL_NAMES.size(); i++)
        delete models[(Model::ID)i];

    for (size_t i = 0; i < Decal::SPRITE_NAMES.size(); i++)
        delete sprites[(Decal::ID)i];

    delete world;
    delete quad;
}

bool Renderer::update()
{
    device.window->update();
    world->update(1.0f / 60.0f);
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

    // Collect matrices
    const glm::mat4 camera_view = world->camera.view_matrix();
    const glm::mat4 camera_projection = world->camera.projection_matrix(device.swapchain_width, device.swapchain_height);
    const glm::mat4 weapon_model = world->player->weapon.get_model_matrix(
        camera_view,
        world->player->head_bob_offset
    );

    // Collect lights
    const LightingState lighting = collect_lights();

    for (u32 i = 0; i < lighting.n_lights; i++)
        shadow_pass.execute(
            command_buffer,
            *world,
            models,
            sprites,
            lighting.matrices[i],
            weapon_model,
            *quad,
            (u8)i
        );

    depth_pass.execute(
        command_buffer,
        *world,
        models,
        camera_view,
        camera_projection,
        weapon_model
    );

    diffuse_pass.execute(
        command_buffer,
        *world,
        models,
        camera_view,
        camera_projection,
        weapon_model,
        lighting.fragment_uniforms
    );

    sprite_pass.execute(
        command_buffer,
        *world,
        sprites,
        camera_view,
        camera_projection,
        *quad,
        lighting.fragment_uniforms
    );

    bloom_pass.execute(command_buffer);

    composite_pass.execute(command_buffer, swapchain_texture.value());

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

Renderer::LightingState Renderer::collect_lights() const
{
    LightingState state = {};

    const bool flashlight_enabled = world->player->flashlight.enabled;
    const u32 max_world_lights = QUALITY_SETTINGS.max_spotlights - (flashlight_enabled ? 1 : 0);

    // Gather candidate world lights with their distance to the player
    std::vector<std::pair<float, Spotlight*>> candidates;
    candidates.reserve(world->spotlights.size());
    const glm::vec3 player_pos = world->player->position;

    for (auto& light : world->spotlights)
    {
        const float dist2 = glm::length(light.position - world->player->position);
        candidates.emplace_back(dist2, &light);
    }

    // Partial sort: nearest max_world_lights first
    const u32 n_world_lights = std::min(max_world_lights, (u32)candidates.size());
    std::partial_sort(
        candidates.begin(),
        candidates.begin() + n_world_lights,
        candidates.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; }
    );

    state.n_lights = n_world_lights + (flashlight_enabled ? 1 : 0);

    std::array<Spotlight*, QUALITY_SETTINGS.max_spotlights> spotlights;

    for (u32 i = 0; i < n_world_lights; i++)
    {
        spotlights[i] = candidates[i].second;
        state.matrices[i] = spotlights[i]->get_matrix();
    }

    if (flashlight_enabled)
    {
        spotlights[state.n_lights - 1] = &world->player->flashlight;
        state.matrices[state.n_lights - 1] = world->player->flashlight.get_matrix();
    }

    for (u32 i = 0; i < state.n_lights; i++)
        state.fragment_uniforms.spotlights[i] = spotlights[i]->get_uniform_buffer(
            state.matrices[i]
        );

    for (u32 i = state.n_lights; i < QUALITY_SETTINGS.max_spotlights; i++)
        state.fragment_uniforms.spotlights[i] = Spotlight::get_disabled_uniform_buffer();

    return state;
}

Quad* Renderer::create_quad(Device& device)
{
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    Quad* quad = new Quad(device.device, copy_pass);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    return quad;
}