#include "render/renderer.h"
#include "render/quad.h"
#include "transform.h"

constexpr glm::vec3 zenith_colour = { 0.4f, 0.54f, 0.71f };
constexpr glm::vec3 horizon_colour = { 0.26f, 0.57f, 0.91f };

Renderer::Renderer(const std::string& window_title, const int width, const int height) :
    window(window_title, width, height),
    dummy_cubemap(Texture::Type::Cubemap, 2, 2),
    quad(quad_vertices, quad_indices)
{
    // Setup GL state
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Init ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Texture units
    diffuse_shader.bind();
    diffuse_shader.set_uniform("diffuse", 0);
    diffuse_shader.set_uniform("directional_light.depth", 1);

    // Constant uniforms
    sky_shader.bind();
    sky_shader.set_uniform("zenith_colour", zenith_colour);
    sky_shader.set_uniform("horizon_colour", horizon_colour);

}

void Renderer::load_world(const World& world)
{
    // Point lights
    point_light_framebuffers.resize(world.point_lights.size());
    for (size_t i = 0; i < world.point_lights.size(); ++i)
        point_light_framebuffers[i] = new Framebuffer(true);

    // Directional light
    diffuse_shader.bind();
    if (world.directional_light.has_value())
    {
        diffuse_shader.set_uniform("directional_light.position", world.directional_light->position);
        diffuse_shader.set_uniform("directional_light.colour", world.directional_light->colour);
        directional_light_framebuffer.emplace();

        min_world_bounds = world.map->get_min_bounds();
        max_world_bounds = world.map->get_max_bounds();

        diffuse_shader.set_uniform("directional_light.matrix", directional_light_framebuffer->get_matrix(
            min_world_bounds,
            max_world_bounds,
            world.directional_light->position
        ));
    }
    else
    {
        diffuse_shader.set_uniform("directional_light.position", glm::vec3(0.0f));
        diffuse_shader.set_uniform("directional_light.colour", glm::vec3(0.0f));
    }

    // Load models
    for (const auto& entity : world.entities)
    {
        const char* mesh_path = entity.get_mesh();
        const char* texture_path = entity.get_texture();

        if (meshes.count(mesh_path) == 0)
            meshes[mesh_path] = new Mesh(mesh_path);

        if (textures.count(texture_path) == 0)
            textures[texture_path] = new Texture(texture_path);
    }

    // Environment
    diffuse_shader.set_uniform("ambient", horizon_colour * world.ambient_lighting);
    diffuse_shader.set_uniform("min_shadow", world.min_shadow);

    render_point_light_pass(world, false);
    render_directional_light_pass(world, false);
}

bool Renderer::should_render()
{
    // Update window
    if (!window.update()) return false;

    // Begin ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void Renderer::render(const World& world)
{
    // Get camera matrices
    const glm::mat4 projection_matrix = world.camera.projection_matrix(
        window.framebuffer_width,
        window.framebuffer_height
    );
    const glm::mat4 view_matrix = world.camera.view_matrix();

    render_point_light_pass(world, true);
    render_directional_light_pass(world, true);

    if (world.has_sky)
        render_sky_pass(glm::inverse(projection_matrix * view_matrix));
    else
        glClear(GL_COLOR_BUFFER_BIT);

    render_forward_pass(world, view_matrix, projection_matrix);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::render_point_light_pass(const World& world, const bool render_only_dynamic)
{
    // Possible early exit to avoid state changes
    if (render_only_dynamic)
    {
        bool all_static = true;
        for (const auto& light : world.point_lights)
        {
            if (light.dynamic)
            {
                all_static = false;
                break;
            }
        }
        if (all_static) return;
    }

    assert(world.point_lights.size() == point_light_framebuffers.size());
    point_light_shader.bind();

    for (size_t i = 0; i < world.point_lights.size(); ++i)
    {
        if (render_only_dynamic && !world.point_lights[i].dynamic) continue;

        Framebuffer* framebuffer = point_light_framebuffers[i];
        framebuffer->bind();

        const auto matrices = framebuffer->get_matrices(
            world.point_lights[i].position,
            world.point_lights[i].distance
        );

        // Set uniforms
        point_light_shader.set_uniform("light_position", world.point_lights[i].position);
        point_light_shader.set_uniform("far_plane", world.point_lights[i].distance);
        point_light_shader.set_uniform("matrices[0]", matrices[0]);
        point_light_shader.set_uniform("matrices[1]", matrices[1]);
        point_light_shader.set_uniform("matrices[2]", matrices[2]);
        point_light_shader.set_uniform("matrices[3]", matrices[3]);
        point_light_shader.set_uniform("matrices[4]", matrices[4]);
        point_light_shader.set_uniform("matrices[5]", matrices[5]);
        point_light_shader.set_uniform("model", glm::mat4(1.0f));

        // Render map
        glClear(GL_DEPTH_BUFFER_BIT);
        for (const auto& draw_call : world.map->draw_calls)
        {
            draw_call.mesh->bind();
            draw_call.mesh->draw();
        }

        // Render entities
        for (const auto& entity : world.entities)
        {
            point_light_shader.set_uniform("model", entity.transform.matrix());
            meshes[entity.get_mesh()]->bind();
            meshes[entity.get_mesh()]->draw();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render_directional_light_pass(const World& world, const bool render_only_dynamic)
{
    if (!world.directional_light.has_value()) return;
    if (!world.directional_light->dynamic && render_only_dynamic) return;

    directional_light_shader.bind();
    directional_light_framebuffer->bind();
    const glm::mat4 matrix = directional_light_framebuffer->get_matrix(
        min_world_bounds,
        max_world_bounds,
        world.directional_light->position
    );

    // Render map
    glClear(GL_DEPTH_BUFFER_BIT);
    directional_light_shader.set_uniform("matrix", matrix);
    for (const auto& draw_call : world.map->draw_calls)
    {
        draw_call.mesh->bind();
        draw_call.mesh->draw();
    }

    // Render entities
    for (const auto& entity : world.entities)
    {
        directional_light_shader.set_uniform("matrix", matrix * entity.transform.matrix());
        meshes[entity.get_mesh()]->bind();
        meshes[entity.get_mesh()]->draw();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render_forward_pass(
    const World& world,
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix
)
{
    // Begin forward pass
    glViewport(0, 0, window.framebuffer_width, window.framebuffer_height);
    glClear(GL_DEPTH_BUFFER_BIT);
    diffuse_shader.bind();
    diffuse_shader.set_uniform("view_projection", projection_matrix * view_matrix);
    diffuse_shader.set_uniform("n_point_lights", std::min(
        (int)world.point_lights.size(),
        max_point_lights)
    );

    // Get nearest N lights
    std::vector<size_t> light_indices;
    light_indices.resize(world.point_lights.size());
    for (size_t i = 0; i < world.point_lights.size(); ++i)
        light_indices[i] = i;

    if (world.point_lights.size() > max_point_lights)
    {
        std::partial_sort(
            light_indices.begin(),
            light_indices.begin() + max_point_lights,
            light_indices.end(),
            [&](size_t a, size_t b)
            {
                const glm::vec3 pos_a = world.point_lights[a].position;
                const glm::vec3 pos_b = world.point_lights[b].position;
                return  glm::length2(pos_a - world.camera.position) <
                        glm::length2(pos_b - world.camera.position);
            }
        );
    }

    // Point lights
    for (size_t i = 0; i < max_point_lights; ++i)
    {
        std::string s = std::to_string(i);
        if (i < world.point_lights.size())
        {
            const PointLight& light = world.point_lights[light_indices[i]];
            diffuse_shader.set_uniform("point_lights[" + s + "].position", light.position);
            diffuse_shader.set_uniform("point_lights[" + s + "].colour", light.colour);
            diffuse_shader.set_uniform("point_lights[" + s + "].far_plane", light.distance);
            diffuse_shader.set_uniform("point_lights[" + s + "].depth", 2 + (int)i);
            point_light_framebuffers[i]->texture->bind(2 + i);
        }
        else
        {
            // Must set texture to cubemap, but 0 isn't
            diffuse_shader.set_uniform("point_lights[" + s + "].depth", 2);
        }
    }

    // Must have at least one valid cubemap for them all the point to
    if (world.point_lights.empty())
        dummy_cubemap.bind(2);

    // Directional light
    if (world.directional_light.has_value())
        directional_light_framebuffer->texture->bind(1);

    // Render map
    diffuse_shader.set_uniform("model", glm::mat4(1.0f));
    diffuse_shader.set_uniform("normal", glm::mat3(1.0f));
    for (const auto& draw_call : world.map->draw_calls)
    {
        draw_call.texture->bind();
        draw_call.mesh->bind();
        draw_call.mesh->draw();
    }

    // Render entities
    for (const auto& entity : world.entities)
    {
        const glm::mat4 model = entity.transform.matrix();
        diffuse_shader.set_uniform("model", model);
        diffuse_shader.set_uniform("normal", glm::mat3(glm::transpose(glm::inverse(model))));
        textures[entity.get_texture()]->bind();
        meshes[entity.get_mesh()]->bind();
        meshes[entity.get_mesh()]->draw();
    }
}

void Renderer::render_sky_pass(const glm::mat4& inverse_view)
{
    sky_shader.bind();
    sky_shader.set_uniform("screen_size", glm::vec2 { window.framebuffer_width, window.framebuffer_height });
    sky_shader.set_uniform("inverse_view", inverse_view);
    quad.bind();
    quad.draw();
}

Renderer::~Renderer()
{
    for (Framebuffer* fb : point_light_framebuffers)
        delete fb;

    for (const auto& [_, texture] : textures)
        delete texture;

    for (const auto& [_, mesh] : meshes)
        delete mesh;
}
