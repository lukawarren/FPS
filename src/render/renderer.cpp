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
    world = new World("map4.map", device.device, copy_pass, *device.window, audio);

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

    init_imgui();
    last_time = SDL_GetTicksNS();

    dbg("TODO: don't use uniform buffer for lights");
}

Renderer::~Renderer()
{
    device.wait_for_idle();

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    for (size_t i = 0; i < Model::MODEL_NAMES.size(); i++)
        delete models[(Model::ID)i];

    for (size_t i = 0; i < Decal::SPRITE_NAMES.size(); i++)
        delete sprites[(Decal::ID)i];

    delete world;
    delete quad;
}

bool Renderer::update()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    device.window->update();

    u64 time = SDL_GetTicksNS();
    if (time - last_time == 0) time++;
    const float delta = float(time - last_time) / 1000000000.0f;
    last_time = time;

    world->update(delta);

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
        texture_manager.on_swapchain_format_change();
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

    debug_renderer.execute(
        camera_view,
        camera_projection,
        (float)device.swapchain_width,
        (float)device.swapchain_height
    );

    // ImGui will crash on un-maximising as uses old dimensions?
    if (!device.did_swapchain_format_change())
    {
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture.value();
        target_info.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
        target_info.load_op = SDL_GPU_LOADOP_LOAD;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
        SDL_EndGPURenderPass(render_pass);
    }
    else ImGui::Render();

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void Renderer::init_imgui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsLight();
    ImGui::GetStyle().FontSizeBase = 25.0f;

    ImGui_ImplSDL3_InitForSDLGPU(device.window->get_window());
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = device.device;
    init_info.ColorTargetFormat = device.swapchain_format;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);
}

Renderer::LightingState Renderer::collect_lights() const
{
    LightingState state = {};

    const bool flashlight_enabled = world->player->flashlight.enabled;
    const u32 max_world_lights = QUALITY_SETTINGS.max_spotlights - (flashlight_enabled ? 1 : 0);

    // Gather candidate world lights with their distance to the player
    std::vector<std::pair<float, Spotlight*>> candidates;
    candidates.reserve(world->torchlights.size());
    const glm::vec3 player_pos = world->camera.position;

    for (auto& light : world->torchlights)
    {
        const float dist2 = glm::length(light.position - world->camera.position);
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